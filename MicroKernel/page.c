/*	page.c for ulios
	作者：孙亮
	功能：分页内存管理
	最后修改日期：2009-05-29
*/

#include "knldef.h"

/*初始化物理内存管理*/
long InitPMM()
{
	DWORD i;
	MEM_ARDS *CurArd;

	i = (((MemEnd - UPHYMEM_ADDR) + 0x0001FFFF) >> 17);	/*取得进程物理页管理表长度*/
	if ((pmmap = (DWORD*)kmalloc(i * sizeof(DWORD))) == NULL)	/*建立用户内存位图,以4字节为单位*/
		return ERROR_HAVENO_KMEM;
	memset32(pmmap, 0xFFFFFFFF, i);	/*先标记为已用*/
	PmmLen = (i << 5);	/*用户内存总页数*/
	PmpID = INVALID;
	RemmSiz = 0;
	for (CurArd = ards; CurArd->addr != INVALID; CurArd++)
		if (CurArd->type == ARDS_TYPE_RAM && CurArd->addr + CurArd->siz > UPHYMEM_ADDR)	/*内存区有效且包含了进程内存页面*/
		{
			DWORD fst, cou, end, tcu;	/*页面起始块,数量,循环结束值,临时数量*/

			if (CurArd->addr < UPHYMEM_ADDR)	/*地址转换为进程内存相对地址*/
			{
				fst = 0;
				cou = (CurArd->addr + CurArd->siz - UPHYMEM_ADDR) >> 12;
			}
			else
			{
				fst = (CurArd->addr + 0x00000FFF - UPHYMEM_ADDR) >> 12;	/*从字节地址高端的页面起始*/
				cou = CurArd->siz >> 12;
			}
			if (PmpID > fst)
				PmpID = fst;
			RemmSiz += (cou << 12);
			end = (fst + 0x0000001F) & 0xFFFFFFE0;
			tcu = end - fst;
			if (fst + cou < end)	/*32页边界内的小整块*/
			{
				cou = (fst + cou) & 0x0000001F;
				pmmap[fst >> 5] &= ((0xFFFFFFFF >> tcu) | (0xFFFFFFFF << cou));
				continue;
			}
			pmmap[fst >> 5] &= (0xFFFFFFFF >> tcu);	/*32页边界开始的零碎块*/
			fst = end;
			cou -= tcu;
			memset32(&pmmap[fst >> 5], 0, cou >> 5);	/*大整块*/
			fst += (cou & 0xFFFFFFE0);
			cou &= 0x0000001F;
			if (cou)	/*32页边界结束的零碎块*/
				pmmap[fst >> 5] &= (0xFFFFFFFF << cou);
		}
	return NO_ERROR;
}

/*刷新页表缓冲*/
static inline void RefreshTlb()
{
	register DWORD reg;
	__asm__ __volatile__("movl %%cr3, %0;movl %0, %%cr3": "=r"(reg));
}

/*分配物理页*/
DWORD AllocPage()
{
	DWORD pgid;

	if (PmpID >= PmmLen)	/*物理页不足*/
		return 0;
	pgid = PmpID;
	pmmap[pgid >> 5] |= (1ul << (pgid & 0x0000001F));	/*标志为已分配*/
	RemmSiz -= PAGE_SIZE;
	CurPmd->MemSiz += PAGE_SIZE;
	for (PmpID++; PmpID < PmmLen; PmpID = (PmpID + 32) & 0xFFFFFFE0)	/*寻找下一个未用页*/
	{
		DWORD dat;	/*数据缓存*/

		dat = pmmap[PmpID >> 5];
		if (dat != 0xFFFFFFFF)
		{
			for (;; PmpID++)
				if (!(dat & (1ul << (PmpID & 0x0000001F))))
					return (pgid << 12) + UPHYMEM_ADDR;	/*页号换算为物理地址*/
		}
	}
	return (pgid << 12) + UPHYMEM_ADDR;	/*页号换算为物理地址*/
}

/*回收物理页*/
void FreePage(DWORD pgaddr)
{
	DWORD pgid;

//	if (pgaddr < UPHYMEM_ADDR || pgaddr >= UPHYMEM_ADDR + (PmmLen << 12))	/*非法地址*/
//		return;
	pgid = (pgaddr - UPHYMEM_ADDR) >> 12;	/*物理地址换算为页号*/
	pmmap[pgid >> 5] &= (~(1ul << (pgid & 0x0000001F)));
	RemmSiz += PAGE_SIZE;
	CurPmd->MemSiz -= PAGE_SIZE;
	if (PmpID > pgid)
		PmpID = pgid;
}

/*映射物理地址*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	PROCESS_DESC *CurProc;
	void *LineAddr;
	DWORD fst, end, pdti;
	BOOL isRefreshTlb;

	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	siz = (PhyAddr + siz + 0x00000FFF) & 0xFFFFF000;	/*调整PhyAddr,siz到页边界,siz临时用做结束地址*/
	*((DWORD*)addr) = PhyAddr & 0x00000FFF;	/*记录地址参数的零头*/
	PhyAddr &= 0xFFFFF000;
	if (siz <= PhyAddr)	/*长度越界*/
		return ERROR_INVALID_MAPSIZE;
	if (PhyAddr < BOOTDAT_ADDR || (PhyAddr < UPHYMEM_ADDR + (PmmLen << 12) && siz > (DWORD)kdat))	/*与不允许被映射的区域有重合*/
		return ERROR_INVALID_MAPADDR;
	siz -= PhyAddr;	/*siz恢复为映射字节数*/
	CurProc = CurPmd;
	if ((LineAddr = LockAllocUBlk(CurProc, siz, BLK_PTID_SPECIAL)) == NULL)
		return ERROR_HAVENO_LINEADDR;
	isRefreshTlb = FALSE;
	pdti = CurProc->CurTmd->id.ProcID << 10;
	fst = ((DWORD)LineAddr >> 12);
	end = (((DWORD)LineAddr + siz) >> 12);
	lock(&CurProc->Page_l);
	while (fst < end)	/*清除残余页*/
	{
		if (pdt[pdti | (fst >> 10)] & PAGE_ATTR_P)
		{
			do
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = pt[fst]) & PAGE_ATTR_P)
				{
					LockFreePage(PgAddr);	/*释放物理页*/
					isRefreshTlb = TRUE;
				}
			}
			while (((++fst) & 0x3FF) && fst < end);
		}
		else	/*整个页目录表项空*/
			fst = (fst + 0x400) & 0xFFFFFC00;
	}
	for (fst = pdti | ((DWORD)LineAddr >> 22), end = pdti | (((DWORD)LineAddr + siz) >> 22); fst < end; fst++)	/*修改页目录表*/
	{
		DWORD PtAddr;	/*页表的物理地址*/

		if ((PtAddr = pdt[fst]) & PAGE_ATTR_P)	/*页表已存在*/
		{
			if ((PtAddr & (PAGE_ATTR_W | PAGE_ATTR_U)) != (PAGE_ATTR_W | PAGE_ATTR_U))
			{
				pdt[fst] |= PAGE_ATTR_W | PAGE_ATTR_U;	/*开启写,用户权限*/
				isRefreshTlb = TRUE;
			}
		}
		else	/*页表不存在*/
		{
			if ((PtAddr = LockAllocPage()) == 0)
			{
				ulock(&CurProc->Page_l);
				LockFreeUBlk(CurProc, LineAddr);
				return ERROR_HAVENO_PMEM;
			}
			pdt[fst] = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*开启写,用户权限*/
			memset32(&pt[(fst << 10) & 0x000FFC00], 0, 0x400);	/*清空页表*/
		}
	}
	PhyAddr |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U;
	for (fst = ((DWORD)LineAddr >> 12), end = (((DWORD)LineAddr + siz) >> 12); fst < end; fst++)	/*修改页表,映射地址*/
	{
		pt[fst] = PhyAddr;
		PhyAddr += PAGE_SIZE;
	}
	if (isRefreshTlb)	/*如果有清除映射操作,就要刷新TLB*/
		RefreshTlb();
	ulock(&CurProc->Page_l);
	*((DWORD*)addr) |= *((DWORD*)&LineAddr);
	return NO_ERROR;
}

/*映射用户地址*/
long MapUserAddr(void **addr, DWORD siz)
{
	PROCESS_DESC *CurProc;
	void *LineAddr;

	siz = (siz + 0x00000FFF) & 0xFFFFF000;	/*siz调整到页边界*/
	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	CurProc = CurPmd;
	if ((LineAddr = LockAllocUBlk(CurProc, siz, BLK_PTID_SPECIAL | BLK_PTID_FREEPG)) == NULL)	/*只申请空间即可*/
		return ERROR_HAVENO_LINEADDR;
	*addr = LineAddr;
	return NO_ERROR;
}

/*解除映射地址*/
void UnmapAddr(void *addr, DWORD siz, DWORD attr)
{
	PROCESS_DESC *CurProc;
	DWORD fst, end, pdti;

	CurProc = CurPmd;
	pdti = CurProc->CurTmd->id.ProcID << 10;
	fst = ((DWORD)addr >> 12);
	end = (((DWORD)addr + siz) >> 12);
	lock(&CurProc->Page_l);
	while (fst < end)	/*目录项对应的页表对应的每页分别回收*/
	{
		DWORD PtAddr;	/*页表的物理地址*/

		if ((PtAddr = pdt[pdti | (fst >> 10)]) & PAGE_ATTR_P)
		{
			DWORD tfst;

			tfst = fst;	/*记录开始释放的位置*/
			do	/*删除对应的全部非空页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = pt[fst]) & PAGE_ATTR_P)
				{
					if (attr & BLK_PTID_FREEPG)
						LockFreePage(PgAddr);	/*释放物理页*/
					pt[fst] = 0;
				}
			} while (((++fst) & 0x3FF) && fst < end);
			for (; tfst & 0x3FF; tfst--)	/*检查页表是否需要释放*/
				if (pt[tfst - 1] & PAGE_ATTR_P)
					goto skip;
			for (; fst & 0x3FF; fst++)
				if (pt[fst] & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pdt[pdti | (tfst >> 10)] = 0;
skip:		continue;
		}
		else	/*整个页目录表项空*/
			fst = (fst + 0x400) & 0xFFFFFC00;
	}
	RefreshTlb();
	ulock(&CurProc->Page_l);
}

/*映射地址块给别的进程,并发送映射消息*/
long MapProcAddr(void *ProcAddr, DWORD siz, THREAD_ID ptid, BOOL isReadonly)
{
	PROCESS_DESC *CurProc, *DstProc;
	void *addr, *LineAddr;
	DWORD fst, end, pdti;
	BOOL isRefreshTlb;

	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	siz = ((DWORD)ProcAddr + siz + 0x00000FFF) & 0xFFFFF000;	/*调整ProcAddr,siz到页边界,siz临时用做结束地址*/
	*((DWORD*)&addr) = (DWORD)ProcAddr & 0x00000FFF;	/*记录地址参数的零头*/
	*((DWORD*)&ProcAddr) &= 0xFFFFF000;
	if (siz <= (DWORD)ProcAddr)	/*长度越界*/
		return ERROR_INVALID_MAPSIZE;
	if (ProcAddr < UADDR_OFF || (void*)siz > SHRDLIB_OFF)	/*与不允许被映射的区域有重合*/
		return ERROR_INVALID_MAPADDR;
	siz -= (DWORD)ProcAddr;	/*siz恢复为映射字节数*/
	CurProc = CurPmd;
	if (ptid.ProcID >= PMT_LEN)
		return ERROR_WRONG_PROCID;
	if (ptid.ThedID >= TMT_LEN)
		return ERROR_WRONG_THEDID;

	if ((LineAddr = LockAllocUBlk(CurProc, siz, BLK_PTID_SPECIAL)) == NULL)
		return ERROR_HAVENO_LINEADDR;

	return NO_ERROR;
}

/*解除映射进程共享地址,并发送解除消息*/
long UnmapProcAddr(void *addr, DWORD siz, THREAD_ID ptid)
{
	return NO_ERROR;
}

/*向文件系统发出消息读取文件*/
long ReadFs(WORD file, void *buf, DWORD siz, DWORD seek)
{
	return NO_ERROR;
}

/*页故障处理程序*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	PROCESS_DESC *CurProc;
	EXEC_DESC *CurExec;
	void *LineAddr;
	DWORD pdti, pti;

	__asm__("movl %%cr2, %0": "=r"(LineAddr));
	sti();
	if (LineAddr < UADDR_OFF)	/*进程非法访问内核空间*/
	{
		DebugMsg("Process want access KNL ADDR!\n");
		DeleteThed();
	}
	CurProc = CurPmd;
	CurExec = CurProc->exec;
	pdti = (CurProc->CurTmd->id.ProcID << 10) | ((DWORD)LineAddr >> 22);
	pti = (DWORD)LineAddr >> 12;
	lock(&CurProc->Page_l);
	lock(&CurExec->Page_l);
	/*缺页故障处理*/
	if (!(ErrCode & PAGE_ATTR_P))
	{
		if (!(pdt[pdti] & PAGE_ATTR_P))	/*页表不存在*/
		{
			DWORD PtAddr;	/*页表的物理地址*/

			if (!((PtAddr = pdt0[pdti]) & PAGE_ATTR_P))	/*副本中不存在,申请新页表*/
			{
				void *fst, *end;

				if ((PtAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pdt[pdti] = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr;	/*修改页目录表,设为只读*/
				memset32(&pt[pti & 0xFFFFFC00], 0, 0x400);	/*清空页表*/
				fst = (void*)((DWORD)LineAddr & 0xFFC00000);	/*计算缺页地址所在页表的覆盖区*/
				end = (void*)(((DWORD)LineAddr + 0x003FFFFF) & 0xFFC00000);
				if ((CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < end && CurExec->CodeEnd > fst) ||	/*代码段与缺页表覆盖区有重合*/
					(CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < end && CurExec->DataEnd > fst))		/*数据段与缺页表覆盖区有重合*/
					pdt0[pdti] = pdt[pdti];
			}
			else	/*副本中存在*/
				pdt[pdti] = PtAddr;	/*映射副本页表*/
		}
		if (!(pt[pti] & PAGE_ATTR_P))	/*页不存在*/
		{
			DWORD PgAddr;	/*页的物理地址*/

			if (!(pdt0[pdti] & PAGE_ATTR_P))	/*副本页表不存在,申请新页*/
			{
				if ((PgAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pt[pti] = PAGE_ATTR_P | PAGE_ATTR_U | PgAddr;	/*修改页表,设为只读*/
			}
			else if (!((PgAddr = pt0[pti]) & PAGE_ATTR_P))	/*副本页不存在,申请新页*/
			{
				void *fst, *end;

				if ((PgAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pt[pti] = PAGE_ATTR_P | PAGE_ATTR_U | PgAddr;	/*修改页表,设为只读*/
				fst = (void*)((DWORD)LineAddr & 0xFFFFF000);	/*计算缺页地址所在页的覆盖区*/
				end = (void*)(((DWORD)LineAddr + 0x00000FFF) & 0xFFFFF000);
				memset32(fst, 0, 0x400);	/*清空页*/
				if (CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < end && CurExec->CodeEnd > fst)	/*代码段与缺页覆盖区有重合*/
				{
					void *buf;
					DWORD siz;

					buf = (fst < CurExec->CodeOff ? CurExec->CodeOff : fst);
					siz = (end > CurExec->CodeEnd ? CurExec->CodeEnd : end) - buf;
					if (ReadFs(CurExec->file, buf, siz, buf - CurExec->CodeOff + CurExec->CodeSeek) != NO_ERROR)	/*读取代码段*/
					{
						pt0[pti] = 0;
						ulock(&CurExec->Page_l);
						ulock(&CurProc->Page_l);
						DeleteThed();
					}
					pt0[pti] = pt[pti];
				}
				if (CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < end && CurExec->DataEnd > fst)	/*数据段与缺页覆盖区有重合*/
				{
					void *buf;
					DWORD siz;

					buf = (fst < CurExec->DataOff ? CurExec->DataOff : fst);
					siz = (end > CurExec->DataEnd ? CurExec->DataEnd : end) - buf;
					if (ReadFs(CurExec->file, buf, siz, buf - CurExec->DataOff + CurExec->DataSeek) != NO_ERROR)	/*读取数据段*/
					{
						pt0[pti] = 0;
						ulock(&CurExec->Page_l);
						ulock(&CurProc->Page_l);
						DeleteThed();
					}
					pt0[pti] = pt[pti];
				}
			}
			else	/*副本页存在*/
				pt[pti] = PgAddr;
		}
	}
	/*写保护故障处理*/
	if (ErrCode & PAGE_ATTR_W)
	{
		if (!(pdt[pdti] & PAGE_ATTR_W))	/*页表写保护*/
		{
			if (!(pdt0[pdti] & PAGE_ATTR_P))	/*副本中不存在,直接开启写权限*/
			{
				pdt[pdti] |= PAGE_ATTR_W;
				RefreshTlb();
			}
			else	/*副本中存在,申请新页表*/
			{
				DWORD PtAddr;	/*页表的物理地址*/

				if ((PtAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pdt[pdti] = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*修改页目录表,开启写权限*/
				RefreshTlb();
				memcpy32(&pt[pti & 0xFFFFFC00], &pt0[pti & 0xFFFFFC00], 0x400);	/*复制页表*/
			}
		}
		if (!(pt[pti] & PAGE_ATTR_W))	/*页写保护*/
		{
			if (!(pdt0[pdti] & PAGE_ATTR_P))	/*副本页表不存在,直接开启写权限*/
				pt[pti] |= PAGE_ATTR_W;
			else if (!(pt0[pti] & PAGE_ATTR_P))	/*副本页不存在,直接开启写权限*/
				pt[pti] |= PAGE_ATTR_W;
			else	/*副本页存在,申请复制页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pt[pti] = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PgAddr;	/*修改页表,开启写权限*/
				RefreshTlb();
				pdti = (pdti & 0xFFFFFC00) | PG0_ID;
				pdt[pdti] = pt0[pti];	/*设置临时映射*/
				memcpy32((void*)((DWORD)LineAddr & 0xFFFFF000), &pg0[(DWORD)LineAddr & 0x003FF000], 0x400);	/*写时复制*/
				pdt[pdti] = 0;	/*解除临时映射*/
			}
			RefreshTlb();
		}
	}
	ulock(&CurExec->Page_l);
	ulock(&CurProc->Page_l);
}

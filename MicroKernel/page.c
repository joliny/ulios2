/*	page.c for ulios
	作者：孙亮
	功能：分页内存管理
	最后修改日期：2009-05-29
*/

#include "knldef.h"

/*分配物理页*/
DWORD AllocPage()
{
	DWORD pgid;

	if (PmpID >= PmmLen)	/*物理页不足*/
		return 0;
	pgid = PmpID;
	pmmap[pgid >> 5] |= (1ul << (pgid & 0x0000001F));	/*标志为已分配*/
	RemmSiz -= PAGE_SIZE;
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

	pgid = (pgaddr - UPHYMEM_ADDR) >> 12;	/*物理地址换算为页号*/
	if (pgid >= PmmLen)	/*非法地址*/
		return;
	pmmap[pgid >> 5] &= (~(1ul << (pgid & 0x0000001F)));
	RemmSiz += PAGE_SIZE;
	if (PmpID > pgid)
		PmpID = pgid;
}

/*分配地址映射结构*/
MAPBLK_DESC *AllocMap()
{
	MAPBLK_DESC *map;

	cli();
	if (FstMap >= &mapmt[MAPMT_LEN])
	{
		sti();
		return NULL;
	}
	map = FstMap;
	map->siz = INVALID;
	do
		FstMap++;
	while (FstMap < &mapmt[MAPMT_LEN] && FstMap->siz);
	sti();
	return map;
}

/*释放地址映射结构*/
void FreeMap(MAPBLK_DESC *map)
{
	cli();
	map->siz = 0;
	if (FstMap > map)
		FstMap = map;
	sti();
}

/*填充空页表*/
long FillPt(PAGE_DESC *FstPg, PAGE_DESC *EndPg, DWORD attr)
{
	*((DWORD*)&FstPg) &= 0xFFFFF000;	/*调整地址到页表边界*/
	while (FstPg < EndPg)
	{
		if (!(pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P))	/*页表不存在*/
		{
			DWORD PtAddr;	/*页表的物理地址*/

			if ((PtAddr = LockAllocPage()) == 0)
				return ERROR_HAVENO_PMEM;
			pt[(DWORD)FstPg >> 12] = attr | PtAddr;
			memset32(FstPg, 0, 0x400);	/*清空页表*/
		}
		FstPg += 0x400;
	}
	return NO_ERROR;
}

/*填充页内容*/
long FillPage(EXEC_DESC *exec, void *addr, DWORD ErrCode)
{
	PAGE_DESC *CurPt, *CurPt0, *CurPg, *CurPg0;

	CurPg = &pt[(DWORD)addr >> 12];
	CurPg0 = &pt0[(DWORD)addr >> 12];
	CurPt = &pt[(DWORD)CurPg >> 12];
	CurPt0 = &pt[(DWORD)CurPg0 >> 12];
	if ((ErrCode & PAGE_ATTR_W) && (*CurPt & PAGE_ATTR_ROMAP))	/*页表为映射只读*/
		return ERROR_INVALID_MAPADDR;
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPt & PAGE_ATTR_P))	/*页表不存在*/
	{
		if (*CurPt0 & PAGE_ATTR_P)	/*副本中存在*/
			*CurPt = *CurPt0;	/*映射副本页表*/
		else	/*副本中不存在*/
		{
			DWORD PtAddr;	/*页表的物理地址*/
			void *fst, *end;

			if ((PtAddr = LockAllocPage()) == 0)	/*申请新页表*/
				return ERROR_HAVENO_PMEM;
			PtAddr |= PAGE_ATTR_P | PAGE_ATTR_U;	/*设为只读*/
			*CurPt |= PtAddr;	/*修改页目录表*/
			memset32((void*)((DWORD)CurPg & 0xFFFFF000), 0, 0x400);	/*清空页表*/
			fst = (void*)((DWORD)addr & 0xFFC00000);	/*计算缺页地址所在页表的覆盖区*/
			end = (void*)((DWORD)fst + PAGE_SIZE * 0x400);
			if ((exec->CodeOff < exec->CodeEnd && exec->CodeOff < end && exec->CodeEnd > fst) ||	/*代码段与缺页表覆盖区有重合*/
				(exec->DataOff < exec->DataEnd && exec->DataOff < end && exec->DataEnd > fst))		/*数据段与缺页表覆盖区有重合*/
				*CurPt0 = PtAddr;
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && !(*CurPt & PAGE_ATTR_W))	/*页表写保护*/
	{
		if (*CurPt0 & PAGE_ATTR_P)	/*副本页表存在*/
		{
			DWORD PtAddr;	/*页表的物理地址*/

			if ((PtAddr = LockAllocPage()) == 0)	/*申请新页表*/
				return ERROR_HAVENO_PMEM;
			*CurPt = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*修改页目录表,开启写权限*/
			RefreshTlb();
			memcpy32((void*)((DWORD)CurPg & 0xFFFFF000), (void*)((DWORD)CurPg0 & 0xFFFFF000), 0x400);	/*页表写时复制*/
		}
		else	/*副本页表不存在*/
		{
			*CurPt |= PAGE_ATTR_W;	/*直接开启写权限*/
			RefreshTlb();
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && (*CurPg & PAGE_ATTR_ROMAP))	/*页为映射只读*/
		return ERROR_INVALID_MAPADDR;
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPg & PAGE_ATTR_P))	/*页不存在*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*副本页表和副本页存在*/
			*CurPg = *CurPg0;	/*映射副本页*/
		else	/*副本页表或副本页不存在*/
		{
			DWORD PgAddr;	/*页的物理地址*/
			void *fst, *end;

			if ((PgAddr = LockAllocPage()) == 0)	/*申请新页*/
				return ERROR_HAVENO_PMEM;
			PgAddr |= PAGE_ATTR_P | PAGE_ATTR_U;	/*设为只读*/
			*CurPg |= PgAddr;	/*修改页表*/
			memset32((void*)((DWORD)addr & 0xFFFFF000), 0, 0x400);	/*清空页*/
			fst = (void*)((DWORD)addr & 0xFFFFF000);	/*计算缺页地址所在页的覆盖区*/
			end = (void*)((DWORD)fst + PAGE_SIZE);
			if (exec->CodeOff < exec->CodeEnd && exec->CodeOff < end && exec->CodeEnd > fst)	/*代码段与缺页覆盖区有重合*/
			{
				void *buf;
				DWORD siz;
				THREAD_ID ptid;
				DWORD data[MSG_DATA_LEN];

				buf = (fst < exec->CodeOff ? exec->CodeOff : fst);
				siz = (end > exec->CodeEnd ? exec->CodeEnd : end) - buf;
				data[0] = FS_API_READPAGE;	/*向文件系统发出消息读取可执行文件页*/
				data[1] = buf - exec->CodeOff + exec->CodeSeek;
				ptid = kpt[FS_KPORT];
				if ((data[0] = MapProcAddr(buf, siz, &ptid, TRUE, FALSE, data, FS_OUT_TIME)) != NO_ERROR)
					return data[0];
				if (data[2] != NO_ERROR)
					return data[2];
				*CurPg0 = PgAddr;
			}
			if (exec->DataOff < exec->DataEnd && exec->DataOff < end && exec->DataEnd > fst)	/*数据段与缺页覆盖区有重合*/
			{
				void *buf;
				DWORD siz;
				THREAD_ID ptid;
				DWORD data[MSG_DATA_LEN];

				buf = (fst < exec->DataOff ? exec->DataOff : fst);
				siz = (end > exec->DataEnd ? exec->DataEnd : end) - buf;
				data[0] = FS_API_READPAGE;	/*向文件系统发出消息读取可执行文件页*/
				data[1] = buf - exec->DataOff + exec->DataSeek;
				ptid = kpt[FS_KPORT];
				if ((data[0] = MapProcAddr(buf, siz, &ptid, TRUE, FALSE, data, FS_OUT_TIME)) != NO_ERROR)
					return data[0];
				if (data[2] != NO_ERROR)
					return data[2];
				*CurPg0 = PgAddr;
			}
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && !(*CurPg & PAGE_ATTR_W))	/*页写保护*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*副本页存在*/
		{
			DWORD PgAddr;	/*页的物理地址*/
			PAGE_DESC *Pg0Pt;

			if ((PgAddr = LockAllocPage()) == 0)	/*申请新页*/
				return ERROR_HAVENO_PMEM;
			*CurPg = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PgAddr;	/*修改页表,开启写权限*/
			RefreshTlb();
			Pg0Pt = (PAGE_DESC*)(((DWORD)CurPt & 0xFFFFF000) | (PG0_ID * sizeof(PAGE_DESC)));
			*Pg0Pt = *CurPt0;	/*设置临时映射*/
			memcpy32((void*)((DWORD)addr & 0xFFFFF000), &pg0[(DWORD)addr & 0x003FF000], 0x400);	/*页写时复制*/
			*Pg0Pt = 0;	/*解除临时映射*/
		}
		else	/*副本页表或副本页不存在*/
		{
			*CurPg |= PAGE_ATTR_W;	/*直接开启写权限*/
			RefreshTlb();
		}
	}
	return NO_ERROR;
}

/*清除页内容*/
void ClearPage(PAGE_DESC *FstPg, PAGE_DESC *EndPg, BOOL isFree)
{
	BOOL isRefreshTlb;

	isRefreshTlb = FALSE;
	while (FstPg < EndPg)	/*目录项对应的页表对应的每页分别回收*/
	{
		DWORD PtAddr;	/*页表的物理地址*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P)	/*页表存在*/
		{
			PAGE_DESC *TFstPg;

			TFstPg = FstPg;	/*记录开始释放的位置*/
			do	/*删除对应的全部非空页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*物理页存在*/
				{
					if (isFree)
						LockFreePage(PgAddr);	/*释放物理页*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
			}
			while (((DWORD)(++FstPg) & 0xFFF) && FstPg < EndPg);
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*检查页表是否需要释放*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg & 0xFFF; FstPg++)
				if (*FstPg & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pt[(DWORD)TFstPg >> 12] = 0;
skip:		continue;
		}
		else	/*整个页目录表项空*/
			FstPg = (PAGE_DESC*)(((DWORD)FstPg + 0x1000) & 0xFFFFF000);
	}
	if (isRefreshTlb)
		RefreshTlb();
}

/*清除页内容(不清除副本)*/
void ClearPageNoPt0(PAGE_DESC *FstPg, PAGE_DESC *EndPg)
{
	PAGE_DESC *FstPg0;
	BOOL isRefreshTlb;

	FstPg0 = FstPg + (PT0_ID - PT_ID) * 0x100000;
	isRefreshTlb = FALSE;
	while (FstPg < EndPg)	/*目录项对应的页表对应的每页分别回收*/
	{
		DWORD PtAddr;	/*页表的物理地址*/
		DWORD Pt0Addr;	/*副本页表的物理地址*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P && (PtAddr >> 12) != ((Pt0Addr = pt[(DWORD)FstPg0 >> 12]) >> 12))	/*页表存在,与副本不重合*/
		{
			PAGE_DESC *TFstPg;

			TFstPg = FstPg;	/*记录开始释放的位置*/
			do	/*删除对应的全部非空页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*物理页存在*/
				{
					if (!(Pt0Addr & PAGE_ATTR_P) || (PgAddr >> 12) != (*FstPg0 >> 12))	/*与副本不重合*/
						LockFreePage(PgAddr);	/*释放物理页*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
			}
			while (((DWORD)(++FstPg0, ++FstPg) & 0xFFF) && FstPg < EndPg);
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*检查页表是否需要释放*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg & 0xFFF; FstPg0++, FstPg++)
				if (*FstPg & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pt[(DWORD)TFstPg >> 12] = 0;
skip:		continue;
		}
		else	/*整个页目录表项空*/
		{
			FstPg = (PAGE_DESC*)(((DWORD)FstPg + 0x1000) & 0xFFFFF000);
			FstPg0 = (PAGE_DESC*)(((DWORD)FstPg0 + 0x1000) & 0xFFFFF000);
		}
	}
	if (isRefreshTlb)
		RefreshTlb();
}

/*映射进程取得地址映射结构,并锁定目标进程*/
static MAPBLK_DESC *GetMap(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

	cli();
	for (PreMap = NULL, CurMap = proc->map; CurMap; CurMap = (PreMap = CurMap)->nxt)
		if (CurMap->addr == addr)
		{
			map = CurMap;
			if (PreMap)	/*从映射进程的映射结构链表中删除*/
				PreMap->nxt = map->nxt;
			else
				proc->map = map->nxt;
			DstProc = pmt[map->ptid2.ProcID];
			for (PreMap = NULL, CurMap = DstProc->map2; CurMap; CurMap = (PreMap = CurMap)->nxt2)
				if (CurMap == map)
					break;
			if (PreMap)	/*从被映射进程的映射结构链表中删除*/
				PreMap->nxt2 = map->nxt2;
			else
				DstProc->map2 = map->nxt2;
			proc->MapCou--;
			DstProc->Map_l++;
			sti();
			return map;
		}
	sti();
	return NULL;
}

/*被映射进程取得地址映射结构,并锁定目标进程*/
static MAPBLK_DESC *GetMap2(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

	cli();
	for (PreMap = NULL, CurMap = proc->map2; CurMap; CurMap = (PreMap = CurMap)->nxt2)
		if (CurMap->addr2 == addr)
		{
			map = CurMap;
			if (PreMap)	/*从映射进程的映射结构链表中删除*/
				PreMap->nxt2 = map->nxt2;
			else
				proc->map2 = map->nxt2;
			DstProc = pmt[map->ptid.ProcID];
			for (PreMap = NULL, CurMap = DstProc->map; CurMap; CurMap = (PreMap = CurMap)->nxt)
				if (CurMap == map)
					break;
			if (PreMap)	/*从被映射进程的映射结构链表中删除*/
				PreMap->nxt = map->nxt;
			else
				DstProc->map = map->nxt;
			DstProc->MapCou--;
			DstProc->Map_l++;
			sti();
			return map;
		}
	sti();
	return NULL;
}

/*映射物理地址*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	PROCESS_DESC *CurProc;
	BLK_DESC *CurBlk;
	void *MapAddr;
	PAGE_DESC *FstPg, *EndPg;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return ERROR_NOT_DRIVER;	/*驱动进程特权API*/
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
	if ((CurBlk = LockAllocUBlk(CurProc, siz)) == NULL)
		return ERROR_HAVENO_LINEADDR;
	MapAddr = CurBlk->addr;
	FstPg = &pt[(DWORD)MapAddr >> 12];
	EndPg = &pt[((DWORD)MapAddr + siz) >> 12];
	lock(&CurProc->Page_l);
	ClearPage(FstPg, EndPg, TRUE);
	if (FillPt(FstPg, EndPg, PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U) != NO_ERROR)
	{
		ClearPage(FstPg, EndPg, TRUE);
		ulock(&CurProc->Page_l);
		LockFreeUBlk(CurProc, CurBlk);
		return ERROR_HAVENO_PMEM;
	}
	PhyAddr |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U;
	for (; FstPg < EndPg; FstPg++)	/*修改页表,映射地址*/
	{
		*FstPg = PhyAddr;
		PhyAddr += PAGE_SIZE;
	}
	ulock(&CurProc->Page_l);
	*((DWORD*)addr) |= *((DWORD*)&MapAddr);
	return NO_ERROR;
}

/*映射用户地址*/
long MapUserAddr(void **addr, DWORD siz)
{
	BLK_DESC *CurBlk;

	siz = (siz + 0x00000FFF) & 0xFFFFF000;	/*siz调整到页边界*/
	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	if ((CurBlk = LockAllocUBlk(CurPmd, siz)) == NULL)	/*只申请空间即可*/
		return ERROR_HAVENO_LINEADDR;
	*addr = CurBlk->addr;
	return NO_ERROR;
}

/*解除映射地址*/
long UnmapAddr(void *addr)
{
	PROCESS_DESC *CurProc;
	BLK_DESC *blk;

	CurProc = CurPmd;
	*((DWORD*)&addr) &= 0xFFFFF000;
	if ((blk = LockFindUBlk(CurProc, addr)) == NULL)
		return ERROR_INVALID_ADDR;
	lock(&CurProc->Page_l);
	ClearPage(&pt[(DWORD)addr >> 12], &pt[((DWORD)addr + blk->siz) >> 12], TRUE);
	ulock(&CurProc->Page_l);
	LockFreeUBlk(CurProc, blk);
	return NO_ERROR;
}

/*映射地址块给别的进程,并发送映射消息*/
long MapProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, BOOL isWrite, BOOL isChkExec, DWORD *argv, DWORD cs)
{
	PROCESS_DESC *CurProc, *DstProc;
	THREAD_DESC *CurThed, *DstThed;
	MAPBLK_DESC *map;
	void *MapAddr, *daddr;
	PAGE_DESC *FstPg, *EndPg, *FstPg2;
	DWORD dsiz;
	MESSAGE_DESC *msg;
	long res;

	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	dsiz = siz;
	siz = ((DWORD)addr + siz + 0x00000FFF) & 0xFFFFF000;	/*调整ProcAddr,siz到页边界,siz临时用做结束地址*/
	*((DWORD*)&daddr) = (DWORD)addr & 0x00000FFF;	/*记录地址参数的零头*/
	*((DWORD*)&addr) &= 0xFFFFF000;
	if (siz <= (DWORD)addr)	/*长度越界*/
		return ERROR_INVALID_MAPSIZE;
	if (addr < UADDR_OFF)	/*与不允许被映射的区域有重合*/
		return ERROR_INVALID_MAPADDR;
	if (ptid->ProcID >= PMT_LEN)
		return ERROR_WRONG_PROCID;
	if (ptid->ThedID >= TMT_LEN)
		return ERROR_WRONG_THEDID;
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (ptid->ProcID == CurThed->id.ProcID)	/*不允许进程自身映射*/
		return ERROR_WRONG_PROCID;
	if (isChkExec)
	{
		EXEC_DESC *CurExec;

		CurExec = CurProc->exec;	/*检查映射区域与代码段和数据段的重合*/
		lock(&CurProc->Page_l);
		lock(&CurExec->Page_l);
		if (CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < (void*)siz && CurExec->CodeEnd > addr)	/*代码段与映射区有重合*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(addr < CurExec->CodeOff ? CurExec->CodeOff : addr) & 0xFFFFF000);
			end = ((void*)siz > CurExec->CodeEnd ? CurExec->CodeEnd : (void*)siz);
			while (fst < end)	/*填充被映射的代码段*/
			{
				if ((res = FillPage(CurExec, fst, isWrite ? PAGE_ATTR_W : 0)) != NO_ERROR)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					return res;
				}
				fst += PAGE_SIZE;
			}
		}
		if (CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < (void*)siz && CurExec->DataEnd > addr)	/*数据段与映射区有重合*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(addr < CurExec->DataOff ? CurExec->DataOff : addr) & 0xFFFFF000);
			end = ((void*)siz > CurExec->DataEnd ? CurExec->DataEnd : (void*)siz);
			while (fst < end)	/*填充被映射的数据段*/
			{
				if ((res = FillPage(CurExec, fst, isWrite ? PAGE_ATTR_W : 0)) != NO_ERROR)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					return res;
				}
				fst += PAGE_SIZE;
			}
		}
		ulock(&CurExec->Page_l);
		ulock(&CurProc->Page_l);
	}
	cli();	/*要访问其他进程的信息,所以防止任务切换*/
	DstProc = pmt[ptid->ProcID];
	if (DstProc == NULL || (DstProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_PROCID;
	}
	DstThed = DstProc->tmt[ptid->ThedID];
	if (DstThed == NULL || (DstThed->attr & THED_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_THEDID;
	}
	if (DstProc->MapCou >= PROC_MAP_LEN)
	{
		sti();
		return ERROR_HAVENO_LINEADDR;
	}
	DstProc->Map_l++;
	sti();
	siz -= (DWORD)addr;	/*siz恢复为映射字节数*/
	if ((MapAddr = LockAllocUFData(DstProc, siz)) == NULL)	/*申请用户空间*/
	{
		clisub(&DstProc->Map_l);
		return ERROR_HAVENO_LINEADDR;
	}
	if ((map = AllocMap(DstProc, siz)) == NULL)	/*申请映射管理结构*/
	{
		LockFreeUFData(DstProc, MapAddr, siz);
		clisub(&DstProc->Map_l);
		return ERROR_HAVENO_LINEADDR;
	}
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + siz) >> 12];
	FstPg2 = &pt2[(DWORD)MapAddr >> 12];
	lock(&DstProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid->ProcID]);	/*映射关系进程的页表*/
	ClearPage(FstPg2, &pt2[((DWORD)MapAddr + siz) >> 12], TRUE);
	while (FstPg < EndPg)	/*修改页表,映射地址*/
	{
		DWORD PtAddr;	/*页表的物理地址*/

		if (((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_ROMAP) && isWrite)	/*源页表映射为只读*/
		{
			ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
			ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
			ulock(&DstProc->Page_l);
			FreeMap(map);
			LockFreeUFData(DstProc, MapAddr, siz);
			clisub(&DstProc->Map_l);
			return ERROR_INVALID_MAPADDR;
		}
		if (PtAddr & PAGE_ATTR_P)	/*源页表存在*/
		{
			do
			{
				DWORD PgAddr;	/*页的物理地址*/

				if (((PgAddr = *FstPg) & PAGE_ATTR_ROMAP) && isWrite)	/*源页映射为只读*/
				{
					ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
					ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
					ulock(&DstProc->Page_l);
					FreeMap(map);
					LockFreeUFData(DstProc, MapAddr, siz);
					clisub(&DstProc->Map_l);
					return ERROR_INVALID_MAPADDR;
				}
				if (PgAddr & PAGE_ATTR_P)	/*源页存在*/
				{
					PAGE_DESC *CurPt2;

					CurPt2 = &pt[(DWORD)FstPg2 >> 12];
					if (!(*CurPt2 & PAGE_ATTR_P))	/*目的页表不存在*/
					{
						DWORD PtAddr;	/*页表的物理地址*/

						if ((PtAddr = LockAllocPage()) == 0)	/*申请目的页表*/
						{
							ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
							ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
							ulock(&DstProc->Page_l);
							FreeMap(map);
							LockFreeUFData(DstProc, MapAddr, siz);
							clisub(&DstProc->Map_l);
							return ERROR_HAVENO_PMEM;
						}
						*CurPt2 = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr;	/*开启用户权限*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*清空页表*/
					}
					if (isWrite)
					{
						*CurPt2 |= PAGE_ATTR_W;	/*开启写权限*/
						*FstPg2 = PgAddr | PAGE_ATTR_W;
					}
					else
					{
						if (((DWORD)CurPt2 << 20) >= (DWORD)MapAddr && ((DWORD)(CurPt2 + 1) << 20) <= (DWORD)MapAddr + siz)	/*页表覆盖区全部位于映射空间内*/
							*CurPt2 |= PAGE_ATTR_ROMAP;	/*页表设置为映射只读*/
						*FstPg2 = (PgAddr & (~PAGE_ATTR_W)) | PAGE_ATTR_ROMAP;	/*关闭写权限,设置为映射只读*/
					}
				}
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
		}
		else	/*源页目录表项空*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
	ulock(&DstProc->Page_l);
	map->addr = (void*)(*((DWORD*)&MapAddr) | *((DWORD*)&daddr));	/*设置映射结构*/
	map->addr2 = (void*)(*((DWORD*)&addr) | *((DWORD*)&daddr));
	map->siz = siz;
	map->ptid = *ptid;
	map->ptid2 = CurThed->id;
	cli();
	map->nxt = DstProc->map;
	map->nxt2 = CurProc->map2;
	CurProc->map2 = DstProc->map = map;
	DstProc->MapCou++;
	DstProc->Map_l--;
	sti();	/*到此映射完成*/
	if ((msg = AllocMsg()) == NULL)	/*申请消息结构*/
		return ERROR_HAVENO_MSGDESC;
	msg->ptid = *ptid;	/*设置消息*/
	msg->data[0] = isWrite ? (MSG_ATTR_MAP | TRUE) : MSG_ATTR_MAP;
	msg->data[1] = dsiz;
	msg->data[2] = *((DWORD*)&MapAddr) | *((DWORD*)&daddr);
	memcpy32(msg->data + 3, argv, MSG_DATA_LEN - 3);
	if (!isChkExec)
		CurProc->PageReadAddr = map->addr2;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*发送映射消息*/
	{
		if (!isChkExec)
			CurProc->PageReadAddr = NULL;
		FreeMsg(msg);
		return res;
	}
	if (cs)	/*等待返回消息*/
	{
		res = RecvProcMsg(&msg, *ptid, cs);
		if (!isChkExec)
			CurProc->PageReadAddr = NULL;
		if (res != NO_ERROR)
			return res;
		*ptid = msg->ptid;
		memcpy32(argv, msg->data, MSG_DATA_LEN);
		FreeMsg(msg);
	}
	return NO_ERROR;
}

/*解除映射进程共享地址,并发送解除消息*/
long UnmapProcAddr(void *addr, const DWORD *argv)
{
	PROCESS_DESC *CurProc, *DstProc;
	MAPBLK_DESC *map;
	PAGE_DESC *FstPg, *EndPg, *FstPg2;
	THREAD_ID ptid;
	void *addr2;
	MESSAGE_DESC *msg;
	long res;

	CurProc = CurPmd;
	if ((map = GetMap(CurProc, addr)) == NULL)
		return ERROR_INVALID_ADDR;
	ptid = map->ptid2;
	addr2 = map->addr2;
	DstProc = pmt[ptid.ProcID];
	res = NO_ERROR;
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + map->siz) >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	if (DstProc->PageReadAddr == NULL || DstProc->PageReadAddr != addr2)
		lock(&DstProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*映射关系进程的页表*/
	while (FstPg < EndPg)	/*目录项对应的页表对应的每页分别回收*/
	{
		DWORD PtAddr;	/*页表的物理地址*/

		if ((PtAddr = pt[(DWORD)FstPg2 >> 12]) & PAGE_ATTR_P)	/*页表存在*/
		{
			PAGE_DESC *TFstPg;
			BOOL isSkip;

			TFstPg = FstPg2;	/*记录开始释放的位置*/
			isSkip = FALSE;
			do	/*删除对应的全部非空页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = *FstPg2) & PAGE_ATTR_P)	/*物理页存在*/
				{
					if ((pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P) && (PgAddr >> 12) == (*FstPg >> 12))	/*重合页不被释放*/
						isSkip = TRUE;
					else
					{
						LockFreePage(PgAddr);	/*释放物理页*/
						*FstPg2 &= PAGE_ATTR_AVL;
					}
				}
			}
			while (((DWORD)(++FstPg, ++FstPg2) & 0xFFF) && FstPg < EndPg);
			if (isSkip)
				goto skip;
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*检查页表是否需要释放*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg2 & 0xFFF; FstPg++, FstPg2++)
				if (*FstPg2 & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pt[(DWORD)TFstPg >> 12] &= PAGE_ATTR_AVL;
skip:		continue;
		}
		else	/*整个页目录表项空*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg2 & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	FstPg = &pt[(DWORD)addr >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	while (FstPg < EndPg)	/*修改页表,映射地址*/
	{
		if (pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P)	/*源页表存在*/
		{
			do
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*源页存在*/
				{
					if (!(pt[(DWORD)FstPg2 >> 12] & PAGE_ATTR_P))	/*目的页表不存在*/
					{
						DWORD PtAddr;	/*页表的物理地址*/

						if ((PtAddr = LockAllocPage()) == 0)	/*申请目的页表*/
						{
							res = ERROR_HAVENO_PMEM;
							goto skip2;
						}
						pt[(DWORD)FstPg2 >> 12] |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*开启用户写权限*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*清空页表*/
					}
					if (!(*FstPg2 & PAGE_ATTR_P))	/*目的页不存在*/
						*FstPg2 |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | (PgAddr & 0xFFFFF000);	/*开启用户写权限*/
				}
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
		}
		else	/*源页目录表项空*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
skip2:
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
	if (DstProc->PageReadAddr == NULL || DstProc->PageReadAddr != addr2)
		ulock(&DstProc->Page_l);
	clisub(&DstProc->Map_l);	/*到此映射完成*/
	lock(&CurProc->Page_l);
	ClearPage(&pt[(DWORD)addr >> 12], EndPg, FALSE);	/*清除当前进程的映射*/
	ulock(&CurProc->Page_l);
	LockFreeUFData(CurProc, (void*)((DWORD)addr & 0xFFFFF000), map->siz);
	FreeMap(map);
	if ((msg = AllocMsg()) == NULL)	/*申请消息结构*/
		return ERROR_HAVENO_MSGDESC;
	msg->ptid = ptid;	/*设置消息*/
	msg->data[0] = MSG_ATTR_UNMAP;
	msg->data[1] = (DWORD)addr2;
	memcpy32(msg->data + 2, argv, MSG_DATA_LEN - 2);
	if (res != NO_ERROR)
		msg->data[2] = res;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*发送撤销映射消息*/
	{
		FreeMsg(msg);
		return res;
	}
	return NO_ERROR;
}

/*取消映射进程共享地址,并发送取消消息*/
long CnlmapProcAddr(void *addr, const DWORD *argv)
{
	PROCESS_DESC *CurProc, *DstProc;
	MAPBLK_DESC *map;
	PAGE_DESC *FstPg, *EndPg, *FstPg2;
	THREAD_ID ptid;
	void *addr2;
	MESSAGE_DESC *msg;
	long res;

	CurProc = CurPmd;
	if ((map = GetMap2(CurProc, addr)) == NULL)
		return ERROR_INVALID_ADDR;
	ptid = map->ptid;
	addr2 = map->addr;
	DstProc = pmt[ptid.ProcID];
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + map->siz) >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	lock(&DstProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*映射关系进程的页表*/
	while (FstPg < EndPg)	/*目录项对应的页表对应的每页分别回收*/
	{
		DWORD PtAddr;	/*页表的物理地址*/

		if ((PtAddr = pt[(DWORD)FstPg2 >> 12]) & PAGE_ATTR_P)	/*页表存在*/
		{
			PAGE_DESC *TFstPg;

			TFstPg = FstPg2;	/*记录开始释放的位置*/
			do	/*删除对应的全部非空页*/
			{
				DWORD PgAddr;	/*页的物理地址*/

				if ((PgAddr = *FstPg2) & PAGE_ATTR_P)	/*物理页存在*/
				{
					if (!(pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P) || (PgAddr >> 12) != (*FstPg >> 12))	/*释放非重合页*/
						LockFreePage(PgAddr);	/*释放物理页*/
					*FstPg2 = 0;
				}
			}
			while (((DWORD)(++FstPg, ++FstPg2) & 0xFFF) && FstPg < EndPg);
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*检查页表是否需要释放*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg2 & 0xFFF; FstPg++, FstPg2++)
				if (*FstPg2 & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*释放页表*/
			pt[(DWORD)TFstPg >> 12] = 0;
skip:		continue;
		}
		else	/*整个页目录表项空*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg2 & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*解除关系进程页表的映射*/
	ulock(&DstProc->Page_l);
	clisub(&DstProc->Map_l);	/*到此解除映射完成*/
	LockFreeUFData(DstProc, (void*)((DWORD)addr2 & 0xFFFFF000), map->siz);
	FreeMap(map);
	if ((msg = AllocMsg()) == NULL)	/*申请消息结构*/
		return ERROR_HAVENO_MSGDESC;
	msg->ptid = ptid;	/*设置消息*/
	msg->data[0] = MSG_ATTR_CNLMAP;
	msg->data[1] = (DWORD)addr2;
	memcpy32(msg->data + 2, argv, MSG_DATA_LEN - 2);
	if ((res = SendMsg(msg)) != NO_ERROR)	/*发送取消映射消息*/
	{
		FreeMsg(msg);
		return res;
	}
	return NO_ERROR;
}

/*清除进程的映射队列*/
void FreeAllMap()
{
	PROCESS_DESC *CurProc;
	MAPBLK_DESC *CurMap, *map;
	DWORD data[MSG_DATA_LEN - 2];

	data[0] = MSG_ATTR_PROCEXIT;
	CurProc = CurPmd;
	while (CurProc->Map_l)	/*等待其它进程映射完成*/
		schedul();
	CurMap = CurProc->map;
	while (CurMap)
	{
		map = CurMap->nxt;
		UnmapProcAddr(CurMap->addr, data);
		CurMap = map;
	}
	CurMap = CurProc->map2;
	while (CurMap)
	{
		map = CurMap->nxt2;
		CnlmapProcAddr(CurMap->addr2, data);
		CurMap = map;
	}
}

/*页故障处理程序*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *CurExec;
	void *addr;
	long res;

	/*进入中断处理程序以前中断已经关闭*/
	addr = GetPageFaultAddr();
	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	CurThed->attr &= (~THED_ATTR_APPS);	/*进入系统调用态*/
	if (addr < UADDR_OFF)	/*进程非法访问内核空间*/
	{
		MESSAGE_DESC *msg;

		if ((msg = AllocMsg()) != NULL)	/*通知报告服务器异常消息*/
		{
			msg->ptid = kpt[REP_KPORT];
			msg->data[0] = MSG_ATTR_EXCEP;
			msg->data[1] = ERROR_INVALID_ADDR;
			msg->data[2] = (DWORD)addr;
			msg->data[3] = eip;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		ThedExit(ERROR_PROC_EXCEP);
	}
	CurExec = CurProc->exec;
	lock(&CurProc->Page_l);
	lock(&CurExec->Page_l);
	res = FillPage(CurExec, addr, ErrCode);
	ulock(&CurExec->Page_l);
	ulock(&CurProc->Page_l);
	if (res != NO_ERROR)
	{
		MESSAGE_DESC *msg;

		if ((msg = AllocMsg()) != NULL)	/*通知报告服务器异常消息*/
		{
			msg->ptid = kpt[REP_KPORT];
			msg->data[0] = MSG_ATTR_EXCEP;
			msg->data[1] = res;
			msg->data[2] = (DWORD)addr;
			msg->data[3] = eip;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		ThedExit(res);
	}
	CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
}

/*	page.c for ulios
	���ߣ�����
	���ܣ���ҳ�ڴ����
	����޸����ڣ�2009-05-29
*/

#include "knldef.h"

/*��ʼ�������ڴ����*/
long InitPMM()
{
	DWORD i;
	MEM_ARDS *CurArd;

	i = (((MemEnd - UPHYMEM_ADDR) + 0x0001FFFF) >> 17);	/*ȡ�ý�������ҳ�������*/
	if ((pmmap = (DWORD*)kmalloc(i * sizeof(DWORD))) == NULL)	/*�����û��ڴ�λͼ,��4�ֽ�Ϊ��λ*/
		return ERROR_HAVENO_KMEM;
	memset32(pmmap, 0xFFFFFFFF, i);	/*�ȱ��Ϊ����*/
	PmmLen = (i << 5);	/*�û��ڴ���ҳ��*/
	PmpID = INVALID;
	RemmSiz = 0;
	for (CurArd = ards; CurArd->addr != INVALID; CurArd++)
		if (CurArd->type == ARDS_TYPE_RAM && CurArd->addr + CurArd->siz > UPHYMEM_ADDR)	/*�ڴ�����Ч�Ұ����˽����ڴ�ҳ��*/
		{
			DWORD fst, cou, end, tcu;	/*ҳ����ʼ��,����,ѭ������ֵ,��ʱ����*/

			if (CurArd->addr < UPHYMEM_ADDR)	/*��ַת��Ϊ�����ڴ���Ե�ַ*/
			{
				fst = 0;
				cou = (CurArd->addr + CurArd->siz - UPHYMEM_ADDR) >> 12;
			}
			else
			{
				fst = (CurArd->addr + 0x00000FFF - UPHYMEM_ADDR) >> 12;	/*���ֽڵ�ַ�߶˵�ҳ����ʼ*/
				cou = CurArd->siz >> 12;
			}
			if (PmpID > fst)
				PmpID = fst;
			RemmSiz += (cou << 12);
			end = (fst + 0x0000001F) & 0xFFFFFFE0;
			tcu = end - fst;
			if (fst + cou < end)	/*32ҳ�߽��ڵ�С����*/
			{
				cou = (fst + cou) & 0x0000001F;
				pmmap[fst >> 5] &= ((0xFFFFFFFF >> tcu) | (0xFFFFFFFF << cou));
				continue;
			}
			pmmap[fst >> 5] &= (0xFFFFFFFF >> tcu);	/*32ҳ�߽翪ʼ�������*/
			fst = end;
			cou -= tcu;
			memset32(&pmmap[fst >> 5], 0, cou >> 5);	/*������*/
			fst += (cou & 0xFFFFFFE0);
			cou &= 0x0000001F;
			if (cou)	/*32ҳ�߽�����������*/
				pmmap[fst >> 5] &= (0xFFFFFFFF << cou);
		}
	return NO_ERROR;
}

/*ˢ��ҳ����*/
static inline void RefreshTlb()
{
	register DWORD reg;
	__asm__ __volatile__("movl %%cr3, %0;movl %0, %%cr3": "=r"(reg));
}

/*��������ҳ*/
DWORD AllocPage()
{
	DWORD pgid;

	if (PmpID >= PmmLen)	/*����ҳ����*/
		return 0;
	pgid = PmpID;
	pmmap[pgid >> 5] |= (1ul << (pgid & 0x0000001F));	/*��־Ϊ�ѷ���*/
	RemmSiz -= PAGE_SIZE;
	CurPmd->MemSiz += PAGE_SIZE;
	for (PmpID++; PmpID < PmmLen; PmpID = (PmpID + 32) & 0xFFFFFFE0)	/*Ѱ����һ��δ��ҳ*/
	{
		DWORD dat;	/*���ݻ���*/

		dat = pmmap[PmpID >> 5];
		if (dat != 0xFFFFFFFF)
		{
			for (;; PmpID++)
				if (!(dat & (1ul << (PmpID & 0x0000001F))))
					return (pgid << 12) + UPHYMEM_ADDR;	/*ҳ�Ż���Ϊ�����ַ*/
		}
	}
	return (pgid << 12) + UPHYMEM_ADDR;	/*ҳ�Ż���Ϊ�����ַ*/
}

/*��������ҳ*/
void FreePage(DWORD pgaddr)
{
	DWORD pgid;

//	if (pgaddr < UPHYMEM_ADDR || pgaddr >= UPHYMEM_ADDR + (PmmLen << 12))	/*�Ƿ���ַ*/
//		return;
	pgid = (pgaddr - UPHYMEM_ADDR) >> 12;	/*�����ַ����Ϊҳ��*/
	pmmap[pgid >> 5] &= (~(1ul << (pgid & 0x0000001F)));
	RemmSiz += PAGE_SIZE;
	CurPmd->MemSiz -= PAGE_SIZE;
	if (PmpID > pgid)
		PmpID = pgid;
}

/*ӳ�������ַ*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	PROCESS_DESC *CurProc;
	void *LineAddr;
	DWORD fst, end, pdti;
	BOOL isRefreshTlb;

	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	siz = (PhyAddr + siz + 0x00000FFF) & 0xFFFFF000;	/*����PhyAddr,siz��ҳ�߽�,siz��ʱ����������ַ*/
	*((DWORD*)addr) = PhyAddr & 0x00000FFF;	/*��¼��ַ��������ͷ*/
	PhyAddr &= 0xFFFFF000;
	if (siz <= PhyAddr)	/*����Խ��*/
		return ERROR_INVALID_MAPSIZE;
	if (PhyAddr < BOOTDAT_ADDR || (PhyAddr < UPHYMEM_ADDR + (PmmLen << 12) && siz > (DWORD)kdat))	/*�벻����ӳ����������غ�*/
		return ERROR_INVALID_MAPADDR;
	siz -= PhyAddr;	/*siz�ָ�Ϊӳ���ֽ���*/
	CurProc = CurPmd;
	if ((LineAddr = LockAllocUBlk(CurProc, siz, BLK_PTID_SPECIAL)) == NULL)
		return ERROR_HAVENO_LINEADDR;
	isRefreshTlb = FALSE;
	pdti = CurProc->CurTmd->id.ProcID << 10;
	fst = ((DWORD)LineAddr >> 12);
	end = (((DWORD)LineAddr + siz) >> 12);
	lock(&CurProc->Page_l);
	while (fst < end)	/*�������ҳ*/
	{
		if (pdt[pdti | (fst >> 10)] & PAGE_ATTR_P)
		{
			do
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = pt[fst]) & PAGE_ATTR_P)
				{
					LockFreePage(PgAddr);	/*�ͷ�����ҳ*/
					isRefreshTlb = TRUE;
				}
			}
			while (((++fst) & 0x3FF) && fst < end);
		}
		else	/*����ҳĿ¼�����*/
			fst = (fst + 0x400) & 0xFFFFFC00;
	}
	for (fst = pdti | ((DWORD)LineAddr >> 22), end = pdti | (((DWORD)LineAddr + siz) >> 22); fst < end; fst++)	/*�޸�ҳĿ¼��*/
	{
		DWORD PtAddr;	/*ҳ��������ַ*/

		if ((PtAddr = pdt[fst]) & PAGE_ATTR_P)	/*ҳ���Ѵ���*/
		{
			if ((PtAddr & (PAGE_ATTR_W | PAGE_ATTR_U)) != (PAGE_ATTR_W | PAGE_ATTR_U))
			{
				pdt[fst] |= PAGE_ATTR_W | PAGE_ATTR_U;	/*����д,�û�Ȩ��*/
				isRefreshTlb = TRUE;
			}
		}
		else	/*ҳ������*/
		{
			if ((PtAddr = LockAllocPage()) == 0)
			{
				ulock(&CurProc->Page_l);
				LockFreeUBlk(CurProc, LineAddr);
				return ERROR_HAVENO_PMEM;
			}
			pdt[fst] = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*����д,�û�Ȩ��*/
			memset32(&pt[(fst << 10) & 0x000FFC00], 0, 0x400);	/*���ҳ��*/
		}
	}
	PhyAddr |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U;
	for (fst = ((DWORD)LineAddr >> 12), end = (((DWORD)LineAddr + siz) >> 12); fst < end; fst++)	/*�޸�ҳ��,ӳ���ַ*/
	{
		pt[fst] = PhyAddr;
		PhyAddr += PAGE_SIZE;
	}
	if (isRefreshTlb)	/*��������ӳ�����,��Ҫˢ��TLB*/
		RefreshTlb();
	ulock(&CurProc->Page_l);
	*((DWORD*)addr) |= *((DWORD*)&LineAddr);
	return NO_ERROR;
}

/*ӳ���û���ַ*/
long MapUserAddr(void **addr, DWORD siz)
{
	PROCESS_DESC *CurProc;
	void *LineAddr;

	siz = (siz + 0x00000FFF) & 0xFFFFF000;	/*siz������ҳ�߽�*/
	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	CurProc = CurPmd;
	if ((LineAddr = LockAllocUBlk(CurProc, siz, BLK_PTID_SPECIAL | BLK_PTID_FREEPG)) == NULL)	/*ֻ����ռ伴��*/
		return ERROR_HAVENO_LINEADDR;
	*addr = LineAddr;
	return NO_ERROR;
}

/*���ӳ���ַ*/
void UnmapAddr(void *addr, DWORD siz, DWORD attr)
{
	PROCESS_DESC *CurProc;
	DWORD fst, end, pdti;

	CurProc = CurPmd;
	pdti = CurProc->CurTmd->id.ProcID << 10;
	fst = ((DWORD)addr >> 12);
	end = (((DWORD)addr + siz) >> 12);
	lock(&CurProc->Page_l);
	while (fst < end)	/*Ŀ¼���Ӧ��ҳ���Ӧ��ÿҳ�ֱ����*/
	{
		DWORD PtAddr;	/*ҳ��������ַ*/

		if ((PtAddr = pdt[pdti | (fst >> 10)]) & PAGE_ATTR_P)
		{
			DWORD tfst;

			tfst = fst;	/*��¼��ʼ�ͷŵ�λ��*/
			do	/*ɾ����Ӧ��ȫ���ǿ�ҳ*/
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = pt[fst]) & PAGE_ATTR_P)
				{
					if (attr & BLK_PTID_FREEPG)
						LockFreePage(PgAddr);	/*�ͷ�����ҳ*/
					pt[fst] = 0;
				}
			} while (((++fst) & 0x3FF) && fst < end);
			for (; tfst & 0x3FF; tfst--)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (pt[tfst - 1] & PAGE_ATTR_P)
					goto skip;
			for (; fst & 0x3FF; fst++)
				if (pt[fst] & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pdt[pdti | (tfst >> 10)] = 0;
skip:		continue;
		}
		else	/*����ҳĿ¼�����*/
			fst = (fst + 0x400) & 0xFFFFFC00;
	}
	RefreshTlb();
	ulock(&CurProc->Page_l);
}

/*ӳ���ַ�����Ľ���,������ӳ����Ϣ*/
long MapProcAddr(void *ProcAddr, DWORD siz, THREAD_ID ptid, BOOL isReadonly)
{
	PROCESS_DESC *CurProc, *DstProc;
	void *addr, *LineAddr;
	DWORD fst, end, pdti;
	BOOL isRefreshTlb;

	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	siz = ((DWORD)ProcAddr + siz + 0x00000FFF) & 0xFFFFF000;	/*����ProcAddr,siz��ҳ�߽�,siz��ʱ����������ַ*/
	*((DWORD*)&addr) = (DWORD)ProcAddr & 0x00000FFF;	/*��¼��ַ��������ͷ*/
	*((DWORD*)&ProcAddr) &= 0xFFFFF000;
	if (siz <= (DWORD)ProcAddr)	/*����Խ��*/
		return ERROR_INVALID_MAPSIZE;
	if (ProcAddr < UADDR_OFF || (void*)siz > SHRDLIB_OFF)	/*�벻����ӳ����������غ�*/
		return ERROR_INVALID_MAPADDR;
	siz -= (DWORD)ProcAddr;	/*siz�ָ�Ϊӳ���ֽ���*/
	CurProc = CurPmd;
	if (ptid.ProcID >= PMT_LEN)
		return ERROR_WRONG_PROCID;
	if (ptid.ThedID >= TMT_LEN)
		return ERROR_WRONG_THEDID;

	if ((LineAddr = LockAllocUBlk(CurProc, siz, BLK_PTID_SPECIAL)) == NULL)
		return ERROR_HAVENO_LINEADDR;

	return NO_ERROR;
}

/*���ӳ����̹����ַ,�����ͽ����Ϣ*/
long UnmapProcAddr(void *addr, DWORD siz, THREAD_ID ptid)
{
	return NO_ERROR;
}

/*���ļ�ϵͳ������Ϣ��ȡ�ļ�*/
long ReadFs(WORD file, void *buf, DWORD siz, DWORD seek)
{
	return NO_ERROR;
}

/*ҳ���ϴ������*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	PROCESS_DESC *CurProc;
	EXEC_DESC *CurExec;
	void *LineAddr;
	DWORD pdti, pti;

	__asm__("movl %%cr2, %0": "=r"(LineAddr));
	sti();
	if (LineAddr < UADDR_OFF)	/*���̷Ƿ������ں˿ռ�*/
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
	/*ȱҳ���ϴ���*/
	if (!(ErrCode & PAGE_ATTR_P))
	{
		if (!(pdt[pdti] & PAGE_ATTR_P))	/*ҳ������*/
		{
			DWORD PtAddr;	/*ҳ��������ַ*/

			if (!((PtAddr = pdt0[pdti]) & PAGE_ATTR_P))	/*�����в�����,������ҳ��*/
			{
				void *fst, *end;

				if ((PtAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pdt[pdti] = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr;	/*�޸�ҳĿ¼��,��Ϊֻ��*/
				memset32(&pt[pti & 0xFFFFFC00], 0, 0x400);	/*���ҳ��*/
				fst = (void*)((DWORD)LineAddr & 0xFFC00000);	/*����ȱҳ��ַ����ҳ��ĸ�����*/
				end = (void*)(((DWORD)LineAddr + 0x003FFFFF) & 0xFFC00000);
				if ((CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < end && CurExec->CodeEnd > fst) ||	/*�������ȱҳ���������غ�*/
					(CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < end && CurExec->DataEnd > fst))		/*���ݶ���ȱҳ���������غ�*/
					pdt0[pdti] = pdt[pdti];
			}
			else	/*�����д���*/
				pdt[pdti] = PtAddr;	/*ӳ�丱��ҳ��*/
		}
		if (!(pt[pti] & PAGE_ATTR_P))	/*ҳ������*/
		{
			DWORD PgAddr;	/*ҳ�������ַ*/

			if (!(pdt0[pdti] & PAGE_ATTR_P))	/*����ҳ������,������ҳ*/
			{
				if ((PgAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pt[pti] = PAGE_ATTR_P | PAGE_ATTR_U | PgAddr;	/*�޸�ҳ��,��Ϊֻ��*/
			}
			else if (!((PgAddr = pt0[pti]) & PAGE_ATTR_P))	/*����ҳ������,������ҳ*/
			{
				void *fst, *end;

				if ((PgAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pt[pti] = PAGE_ATTR_P | PAGE_ATTR_U | PgAddr;	/*�޸�ҳ��,��Ϊֻ��*/
				fst = (void*)((DWORD)LineAddr & 0xFFFFF000);	/*����ȱҳ��ַ����ҳ�ĸ�����*/
				end = (void*)(((DWORD)LineAddr + 0x00000FFF) & 0xFFFFF000);
				memset32(fst, 0, 0x400);	/*���ҳ*/
				if (CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < end && CurExec->CodeEnd > fst)	/*�������ȱҳ���������غ�*/
				{
					void *buf;
					DWORD siz;

					buf = (fst < CurExec->CodeOff ? CurExec->CodeOff : fst);
					siz = (end > CurExec->CodeEnd ? CurExec->CodeEnd : end) - buf;
					if (ReadFs(CurExec->file, buf, siz, buf - CurExec->CodeOff + CurExec->CodeSeek) != NO_ERROR)	/*��ȡ�����*/
					{
						pt0[pti] = 0;
						ulock(&CurExec->Page_l);
						ulock(&CurProc->Page_l);
						DeleteThed();
					}
					pt0[pti] = pt[pti];
				}
				if (CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < end && CurExec->DataEnd > fst)	/*���ݶ���ȱҳ���������غ�*/
				{
					void *buf;
					DWORD siz;

					buf = (fst < CurExec->DataOff ? CurExec->DataOff : fst);
					siz = (end > CurExec->DataEnd ? CurExec->DataEnd : end) - buf;
					if (ReadFs(CurExec->file, buf, siz, buf - CurExec->DataOff + CurExec->DataSeek) != NO_ERROR)	/*��ȡ���ݶ�*/
					{
						pt0[pti] = 0;
						ulock(&CurExec->Page_l);
						ulock(&CurProc->Page_l);
						DeleteThed();
					}
					pt0[pti] = pt[pti];
				}
			}
			else	/*����ҳ����*/
				pt[pti] = PgAddr;
		}
	}
	/*д�������ϴ���*/
	if (ErrCode & PAGE_ATTR_W)
	{
		if (!(pdt[pdti] & PAGE_ATTR_W))	/*ҳ��д����*/
		{
			if (!(pdt0[pdti] & PAGE_ATTR_P))	/*�����в�����,ֱ�ӿ���дȨ��*/
			{
				pdt[pdti] |= PAGE_ATTR_W;
				RefreshTlb();
			}
			else	/*�����д���,������ҳ��*/
			{
				DWORD PtAddr;	/*ҳ��������ַ*/

				if ((PtAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pdt[pdti] = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*�޸�ҳĿ¼��,����дȨ��*/
				RefreshTlb();
				memcpy32(&pt[pti & 0xFFFFFC00], &pt0[pti & 0xFFFFFC00], 0x400);	/*����ҳ��*/
			}
		}
		if (!(pt[pti] & PAGE_ATTR_W))	/*ҳд����*/
		{
			if (!(pdt0[pdti] & PAGE_ATTR_P))	/*����ҳ������,ֱ�ӿ���дȨ��*/
				pt[pti] |= PAGE_ATTR_W;
			else if (!(pt0[pti] & PAGE_ATTR_P))	/*����ҳ������,ֱ�ӿ���дȨ��*/
				pt[pti] |= PAGE_ATTR_W;
			else	/*����ҳ����,���븴��ҳ*/
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = LockAllocPage()) == 0)
				{
					ulock(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					DeleteThed();
				}
				pt[pti] = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PgAddr;	/*�޸�ҳ��,����дȨ��*/
				RefreshTlb();
				pdti = (pdti & 0xFFFFFC00) | PG0_ID;
				pdt[pdti] = pt0[pti];	/*������ʱӳ��*/
				memcpy32((void*)((DWORD)LineAddr & 0xFFFFF000), &pg0[(DWORD)LineAddr & 0x003FF000], 0x400);	/*дʱ����*/
				pdt[pdti] = 0;	/*�����ʱӳ��*/
			}
			RefreshTlb();
		}
	}
	ulock(&CurExec->Page_l);
	ulock(&CurProc->Page_l);
}

/*	page.c for ulios
	���ߣ�����
	���ܣ���ҳ�ڴ����
	����޸����ڣ�2009-05-29
*/

#include "knldef.h"

/*��������ҳ*/
DWORD AllocPage()
{
	DWORD pgid;

	if (PmpID >= PmmLen)	/*����ҳ����*/
		return 0;
	pgid = PmpID;
	pmmap[pgid >> 5] |= (1ul << (pgid & 0x0000001F));	/*��־Ϊ�ѷ���*/
	RemmSiz -= PAGE_SIZE;
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

	pgid = (pgaddr - UPHYMEM_ADDR) >> 12;	/*�����ַ����Ϊҳ��*/
	if (pgid >= PmmLen)	/*�Ƿ���ַ*/
		return;
	pmmap[pgid >> 5] &= (~(1ul << (pgid & 0x0000001F)));
	RemmSiz += PAGE_SIZE;
	if (PmpID > pgid)
		PmpID = pgid;
}

/*�����ַӳ��ṹ*/
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

/*�ͷŵ�ַӳ��ṹ*/
void FreeMap(MAPBLK_DESC *map)
{
	cli();
	map->siz = 0;
	if (FstMap > map)
		FstMap = map;
	sti();
}

/*����ҳ��*/
long FillPt(PAGE_DESC *FstPg, PAGE_DESC *EndPg, DWORD attr)
{
	*((DWORD*)&FstPg) &= 0xFFFFF000;	/*������ַ��ҳ��߽�*/
	while (FstPg < EndPg)
	{
		if (!(pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P))	/*ҳ������*/
		{
			DWORD PtAddr;	/*ҳ��������ַ*/

			if ((PtAddr = LockAllocPage()) == 0)
				return ERROR_HAVENO_PMEM;
			pt[(DWORD)FstPg >> 12] = attr | PtAddr;
			memset32(FstPg, 0, 0x400);	/*���ҳ��*/
		}
		FstPg += 0x400;
	}
	return NO_ERROR;
}

/*���ҳ����*/
long FillPage(EXEC_DESC *exec, void *addr, DWORD ErrCode)
{
	PAGE_DESC *CurPt, *CurPt0, *CurPg, *CurPg0;

	CurPg = &pt[(DWORD)addr >> 12];
	CurPg0 = &pt0[(DWORD)addr >> 12];
	CurPt = &pt[(DWORD)CurPg >> 12];
	CurPt0 = &pt[(DWORD)CurPg0 >> 12];
	if ((ErrCode & PAGE_ATTR_W) && (*CurPt & PAGE_ATTR_ROMAP))	/*ҳ��Ϊӳ��ֻ��*/
		return ERROR_INVALID_MAPADDR;
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPt & PAGE_ATTR_P))	/*ҳ������*/
	{
		if (*CurPt0 & PAGE_ATTR_P)	/*�����д���*/
			*CurPt = *CurPt0;	/*ӳ�丱��ҳ��*/
		else	/*�����в�����*/
		{
			DWORD PtAddr;	/*ҳ��������ַ*/
			void *fst, *end;

			if ((PtAddr = LockAllocPage()) == 0)	/*������ҳ��*/
				return ERROR_HAVENO_PMEM;
			PtAddr |= PAGE_ATTR_P | PAGE_ATTR_U;	/*��Ϊֻ��*/
			*CurPt |= PtAddr;	/*�޸�ҳĿ¼��*/
			memset32((void*)((DWORD)CurPg & 0xFFFFF000), 0, 0x400);	/*���ҳ��*/
			fst = (void*)((DWORD)addr & 0xFFC00000);	/*����ȱҳ��ַ����ҳ��ĸ�����*/
			end = (void*)((DWORD)fst + PAGE_SIZE * 0x400);
			if ((exec->CodeOff < exec->CodeEnd && exec->CodeOff < end && exec->CodeEnd > fst) ||	/*�������ȱҳ���������غ�*/
				(exec->DataOff < exec->DataEnd && exec->DataOff < end && exec->DataEnd > fst))		/*���ݶ���ȱҳ���������غ�*/
				*CurPt0 = PtAddr;
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && !(*CurPt & PAGE_ATTR_W))	/*ҳ��д����*/
	{
		if (*CurPt0 & PAGE_ATTR_P)	/*����ҳ�����*/
		{
			DWORD PtAddr;	/*ҳ��������ַ*/

			if ((PtAddr = LockAllocPage()) == 0)	/*������ҳ��*/
				return ERROR_HAVENO_PMEM;
			*CurPt = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*�޸�ҳĿ¼��,����дȨ��*/
			RefreshTlb();
			memcpy32((void*)((DWORD)CurPg & 0xFFFFF000), (void*)((DWORD)CurPg0 & 0xFFFFF000), 0x400);	/*ҳ��дʱ����*/
		}
		else	/*����ҳ������*/
		{
			*CurPt |= PAGE_ATTR_W;	/*ֱ�ӿ���дȨ��*/
			RefreshTlb();
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && (*CurPg & PAGE_ATTR_ROMAP))	/*ҳΪӳ��ֻ��*/
		return ERROR_INVALID_MAPADDR;
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPg & PAGE_ATTR_P))	/*ҳ������*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*����ҳ��͸���ҳ����*/
			*CurPg = *CurPg0;	/*ӳ�丱��ҳ*/
		else	/*����ҳ��򸱱�ҳ������*/
		{
			DWORD PgAddr;	/*ҳ�������ַ*/
			void *fst, *end;

			if ((PgAddr = LockAllocPage()) == 0)	/*������ҳ*/
				return ERROR_HAVENO_PMEM;
			PgAddr |= PAGE_ATTR_P | PAGE_ATTR_U;	/*��Ϊֻ��*/
			*CurPg |= PgAddr;	/*�޸�ҳ��*/
			memset32((void*)((DWORD)addr & 0xFFFFF000), 0, 0x400);	/*���ҳ*/
			fst = (void*)((DWORD)addr & 0xFFFFF000);	/*����ȱҳ��ַ����ҳ�ĸ�����*/
			end = (void*)((DWORD)fst + PAGE_SIZE);
			if (exec->CodeOff < exec->CodeEnd && exec->CodeOff < end && exec->CodeEnd > fst)	/*�������ȱҳ���������غ�*/
			{
				void *buf;
				DWORD siz;
				THREAD_ID ptid;
				DWORD data[MSG_DATA_LEN];

				buf = (fst < exec->CodeOff ? exec->CodeOff : fst);
				siz = (end > exec->CodeEnd ? exec->CodeEnd : end) - buf;
				data[0] = FS_API_READPAGE;	/*���ļ�ϵͳ������Ϣ��ȡ��ִ���ļ�ҳ*/
				data[1] = buf - exec->CodeOff + exec->CodeSeek;
				ptid = kpt[FS_KPORT];
				if ((data[0] = MapProcAddr(buf, siz, &ptid, TRUE, FALSE, data, FS_OUT_TIME)) != NO_ERROR)
					return data[0];
				if (data[2] != NO_ERROR)
					return data[2];
				*CurPg0 = PgAddr;
			}
			if (exec->DataOff < exec->DataEnd && exec->DataOff < end && exec->DataEnd > fst)	/*���ݶ���ȱҳ���������غ�*/
			{
				void *buf;
				DWORD siz;
				THREAD_ID ptid;
				DWORD data[MSG_DATA_LEN];

				buf = (fst < exec->DataOff ? exec->DataOff : fst);
				siz = (end > exec->DataEnd ? exec->DataEnd : end) - buf;
				data[0] = FS_API_READPAGE;	/*���ļ�ϵͳ������Ϣ��ȡ��ִ���ļ�ҳ*/
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
	if ((ErrCode & PAGE_ATTR_W) && !(*CurPg & PAGE_ATTR_W))	/*ҳд����*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*����ҳ����*/
		{
			DWORD PgAddr;	/*ҳ�������ַ*/
			PAGE_DESC *Pg0Pt;

			if ((PgAddr = LockAllocPage()) == 0)	/*������ҳ*/
				return ERROR_HAVENO_PMEM;
			*CurPg = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PgAddr;	/*�޸�ҳ��,����дȨ��*/
			RefreshTlb();
			Pg0Pt = (PAGE_DESC*)(((DWORD)CurPt & 0xFFFFF000) | (PG0_ID * sizeof(PAGE_DESC)));
			*Pg0Pt = *CurPt0;	/*������ʱӳ��*/
			memcpy32((void*)((DWORD)addr & 0xFFFFF000), &pg0[(DWORD)addr & 0x003FF000], 0x400);	/*ҳдʱ����*/
			*Pg0Pt = 0;	/*�����ʱӳ��*/
		}
		else	/*����ҳ��򸱱�ҳ������*/
		{
			*CurPg |= PAGE_ATTR_W;	/*ֱ�ӿ���дȨ��*/
			RefreshTlb();
		}
	}
	return NO_ERROR;
}

/*���ҳ����*/
void ClearPage(PAGE_DESC *FstPg, PAGE_DESC *EndPg, BOOL isFree)
{
	BOOL isRefreshTlb;

	isRefreshTlb = FALSE;
	while (FstPg < EndPg)	/*Ŀ¼���Ӧ��ҳ���Ӧ��ÿҳ�ֱ����*/
	{
		DWORD PtAddr;	/*ҳ��������ַ*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P)	/*ҳ�����*/
		{
			PAGE_DESC *TFstPg;

			TFstPg = FstPg;	/*��¼��ʼ�ͷŵ�λ��*/
			do	/*ɾ����Ӧ��ȫ���ǿ�ҳ*/
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*����ҳ����*/
				{
					if (isFree)
						LockFreePage(PgAddr);	/*�ͷ�����ҳ*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
			}
			while (((DWORD)(++FstPg) & 0xFFF) && FstPg < EndPg);
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg & 0xFFF; FstPg++)
				if (*FstPg & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pt[(DWORD)TFstPg >> 12] = 0;
skip:		continue;
		}
		else	/*����ҳĿ¼�����*/
			FstPg = (PAGE_DESC*)(((DWORD)FstPg + 0x1000) & 0xFFFFF000);
	}
	if (isRefreshTlb)
		RefreshTlb();
}

/*���ҳ����(���������)*/
void ClearPageNoPt0(PAGE_DESC *FstPg, PAGE_DESC *EndPg)
{
	PAGE_DESC *FstPg0;
	BOOL isRefreshTlb;

	FstPg0 = FstPg + (PT0_ID - PT_ID) * 0x100000;
	isRefreshTlb = FALSE;
	while (FstPg < EndPg)	/*Ŀ¼���Ӧ��ҳ���Ӧ��ÿҳ�ֱ����*/
	{
		DWORD PtAddr;	/*ҳ��������ַ*/
		DWORD Pt0Addr;	/*����ҳ��������ַ*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P && (PtAddr >> 12) != ((Pt0Addr = pt[(DWORD)FstPg0 >> 12]) >> 12))	/*ҳ�����,�븱�����غ�*/
		{
			PAGE_DESC *TFstPg;

			TFstPg = FstPg;	/*��¼��ʼ�ͷŵ�λ��*/
			do	/*ɾ����Ӧ��ȫ���ǿ�ҳ*/
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*����ҳ����*/
				{
					if (!(Pt0Addr & PAGE_ATTR_P) || (PgAddr >> 12) != (*FstPg0 >> 12))	/*�븱�����غ�*/
						LockFreePage(PgAddr);	/*�ͷ�����ҳ*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
			}
			while (((DWORD)(++FstPg0, ++FstPg) & 0xFFF) && FstPg < EndPg);
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg & 0xFFF; FstPg0++, FstPg++)
				if (*FstPg & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pt[(DWORD)TFstPg >> 12] = 0;
skip:		continue;
		}
		else	/*����ҳĿ¼�����*/
		{
			FstPg = (PAGE_DESC*)(((DWORD)FstPg + 0x1000) & 0xFFFFF000);
			FstPg0 = (PAGE_DESC*)(((DWORD)FstPg0 + 0x1000) & 0xFFFFF000);
		}
	}
	if (isRefreshTlb)
		RefreshTlb();
}

/*ӳ�����ȡ�õ�ַӳ��ṹ,������Ŀ�����*/
static MAPBLK_DESC *GetMap(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

	cli();
	for (PreMap = NULL, CurMap = proc->map; CurMap; CurMap = (PreMap = CurMap)->nxt)
		if (CurMap->addr == addr)
		{
			map = CurMap;
			if (PreMap)	/*��ӳ����̵�ӳ��ṹ������ɾ��*/
				PreMap->nxt = map->nxt;
			else
				proc->map = map->nxt;
			DstProc = pmt[map->ptid2.ProcID];
			for (PreMap = NULL, CurMap = DstProc->map2; CurMap; CurMap = (PreMap = CurMap)->nxt2)
				if (CurMap == map)
					break;
			if (PreMap)	/*�ӱ�ӳ����̵�ӳ��ṹ������ɾ��*/
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

/*��ӳ�����ȡ�õ�ַӳ��ṹ,������Ŀ�����*/
static MAPBLK_DESC *GetMap2(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

	cli();
	for (PreMap = NULL, CurMap = proc->map2; CurMap; CurMap = (PreMap = CurMap)->nxt2)
		if (CurMap->addr2 == addr)
		{
			map = CurMap;
			if (PreMap)	/*��ӳ����̵�ӳ��ṹ������ɾ��*/
				PreMap->nxt2 = map->nxt2;
			else
				proc->map2 = map->nxt2;
			DstProc = pmt[map->ptid.ProcID];
			for (PreMap = NULL, CurMap = DstProc->map; CurMap; CurMap = (PreMap = CurMap)->nxt)
				if (CurMap == map)
					break;
			if (PreMap)	/*�ӱ�ӳ����̵�ӳ��ṹ������ɾ��*/
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

/*ӳ�������ַ*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	PROCESS_DESC *CurProc;
	BLK_DESC *CurBlk;
	void *MapAddr;
	PAGE_DESC *FstPg, *EndPg;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return ERROR_NOT_DRIVER;	/*����������ȨAPI*/
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
	for (; FstPg < EndPg; FstPg++)	/*�޸�ҳ��,ӳ���ַ*/
	{
		*FstPg = PhyAddr;
		PhyAddr += PAGE_SIZE;
	}
	ulock(&CurProc->Page_l);
	*((DWORD*)addr) |= *((DWORD*)&MapAddr);
	return NO_ERROR;
}

/*ӳ���û���ַ*/
long MapUserAddr(void **addr, DWORD siz)
{
	BLK_DESC *CurBlk;

	siz = (siz + 0x00000FFF) & 0xFFFFF000;	/*siz������ҳ�߽�*/
	if (siz == 0)
		return ERROR_INVALID_MAPSIZE;
	if ((CurBlk = LockAllocUBlk(CurPmd, siz)) == NULL)	/*ֻ����ռ伴��*/
		return ERROR_HAVENO_LINEADDR;
	*addr = CurBlk->addr;
	return NO_ERROR;
}

/*���ӳ���ַ*/
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

/*ӳ���ַ�����Ľ���,������ӳ����Ϣ*/
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
	siz = ((DWORD)addr + siz + 0x00000FFF) & 0xFFFFF000;	/*����ProcAddr,siz��ҳ�߽�,siz��ʱ����������ַ*/
	*((DWORD*)&daddr) = (DWORD)addr & 0x00000FFF;	/*��¼��ַ��������ͷ*/
	*((DWORD*)&addr) &= 0xFFFFF000;
	if (siz <= (DWORD)addr)	/*����Խ��*/
		return ERROR_INVALID_MAPSIZE;
	if (addr < UADDR_OFF)	/*�벻����ӳ����������غ�*/
		return ERROR_INVALID_MAPADDR;
	if (ptid->ProcID >= PMT_LEN)
		return ERROR_WRONG_PROCID;
	if (ptid->ThedID >= TMT_LEN)
		return ERROR_WRONG_THEDID;
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (ptid->ProcID == CurThed->id.ProcID)	/*�������������ӳ��*/
		return ERROR_WRONG_PROCID;
	if (isChkExec)
	{
		EXEC_DESC *CurExec;

		CurExec = CurProc->exec;	/*���ӳ�����������κ����ݶε��غ�*/
		lock(&CurProc->Page_l);
		lock(&CurExec->Page_l);
		if (CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < (void*)siz && CurExec->CodeEnd > addr)	/*�������ӳ�������غ�*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(addr < CurExec->CodeOff ? CurExec->CodeOff : addr) & 0xFFFFF000);
			end = ((void*)siz > CurExec->CodeEnd ? CurExec->CodeEnd : (void*)siz);
			while (fst < end)	/*��䱻ӳ��Ĵ����*/
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
		if (CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < (void*)siz && CurExec->DataEnd > addr)	/*���ݶ���ӳ�������غ�*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(addr < CurExec->DataOff ? CurExec->DataOff : addr) & 0xFFFFF000);
			end = ((void*)siz > CurExec->DataEnd ? CurExec->DataEnd : (void*)siz);
			while (fst < end)	/*��䱻ӳ������ݶ�*/
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
	cli();	/*Ҫ�����������̵���Ϣ,���Է�ֹ�����л�*/
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
	siz -= (DWORD)addr;	/*siz�ָ�Ϊӳ���ֽ���*/
	if ((MapAddr = LockAllocUFData(DstProc, siz)) == NULL)	/*�����û��ռ�*/
	{
		clisub(&DstProc->Map_l);
		return ERROR_HAVENO_LINEADDR;
	}
	if ((map = AllocMap(DstProc, siz)) == NULL)	/*����ӳ�����ṹ*/
	{
		LockFreeUFData(DstProc, MapAddr, siz);
		clisub(&DstProc->Map_l);
		return ERROR_HAVENO_LINEADDR;
	}
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + siz) >> 12];
	FstPg2 = &pt2[(DWORD)MapAddr >> 12];
	lock(&DstProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid->ProcID]);	/*ӳ���ϵ���̵�ҳ��*/
	ClearPage(FstPg2, &pt2[((DWORD)MapAddr + siz) >> 12], TRUE);
	while (FstPg < EndPg)	/*�޸�ҳ��,ӳ���ַ*/
	{
		DWORD PtAddr;	/*ҳ��������ַ*/

		if (((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_ROMAP) && isWrite)	/*Դҳ��ӳ��Ϊֻ��*/
		{
			ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
			ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
			ulock(&DstProc->Page_l);
			FreeMap(map);
			LockFreeUFData(DstProc, MapAddr, siz);
			clisub(&DstProc->Map_l);
			return ERROR_INVALID_MAPADDR;
		}
		if (PtAddr & PAGE_ATTR_P)	/*Դҳ�����*/
		{
			do
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if (((PgAddr = *FstPg) & PAGE_ATTR_ROMAP) && isWrite)	/*Դҳӳ��Ϊֻ��*/
				{
					ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
					ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
					ulock(&DstProc->Page_l);
					FreeMap(map);
					LockFreeUFData(DstProc, MapAddr, siz);
					clisub(&DstProc->Map_l);
					return ERROR_INVALID_MAPADDR;
				}
				if (PgAddr & PAGE_ATTR_P)	/*Դҳ����*/
				{
					PAGE_DESC *CurPt2;

					CurPt2 = &pt[(DWORD)FstPg2 >> 12];
					if (!(*CurPt2 & PAGE_ATTR_P))	/*Ŀ��ҳ������*/
					{
						DWORD PtAddr;	/*ҳ��������ַ*/

						if ((PtAddr = LockAllocPage()) == 0)	/*����Ŀ��ҳ��*/
						{
							ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
							ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
							ulock(&DstProc->Page_l);
							FreeMap(map);
							LockFreeUFData(DstProc, MapAddr, siz);
							clisub(&DstProc->Map_l);
							return ERROR_HAVENO_PMEM;
						}
						*CurPt2 = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr;	/*�����û�Ȩ��*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*���ҳ��*/
					}
					if (isWrite)
					{
						*CurPt2 |= PAGE_ATTR_W;	/*����дȨ��*/
						*FstPg2 = PgAddr | PAGE_ATTR_W;
					}
					else
					{
						if (((DWORD)CurPt2 << 20) >= (DWORD)MapAddr && ((DWORD)(CurPt2 + 1) << 20) <= (DWORD)MapAddr + siz)	/*ҳ������ȫ��λ��ӳ��ռ���*/
							*CurPt2 |= PAGE_ATTR_ROMAP;	/*ҳ������Ϊӳ��ֻ��*/
						*FstPg2 = (PgAddr & (~PAGE_ATTR_W)) | PAGE_ATTR_ROMAP;	/*�ر�дȨ��,����Ϊӳ��ֻ��*/
					}
				}
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
		}
		else	/*ԴҳĿ¼�����*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
	ulock(&DstProc->Page_l);
	map->addr = (void*)(*((DWORD*)&MapAddr) | *((DWORD*)&daddr));	/*����ӳ��ṹ*/
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
	sti();	/*����ӳ�����*/
	if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
		return ERROR_HAVENO_MSGDESC;
	msg->ptid = *ptid;	/*������Ϣ*/
	msg->data[0] = isWrite ? (MSG_ATTR_MAP | TRUE) : MSG_ATTR_MAP;
	msg->data[1] = dsiz;
	msg->data[2] = *((DWORD*)&MapAddr) | *((DWORD*)&daddr);
	memcpy32(msg->data + 3, argv, MSG_DATA_LEN - 3);
	if (!isChkExec)
		CurProc->PageReadAddr = map->addr2;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*����ӳ����Ϣ*/
	{
		if (!isChkExec)
			CurProc->PageReadAddr = NULL;
		FreeMsg(msg);
		return res;
	}
	if (cs)	/*�ȴ�������Ϣ*/
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

/*���ӳ����̹����ַ,�����ͽ����Ϣ*/
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
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*ӳ���ϵ���̵�ҳ��*/
	while (FstPg < EndPg)	/*Ŀ¼���Ӧ��ҳ���Ӧ��ÿҳ�ֱ����*/
	{
		DWORD PtAddr;	/*ҳ��������ַ*/

		if ((PtAddr = pt[(DWORD)FstPg2 >> 12]) & PAGE_ATTR_P)	/*ҳ�����*/
		{
			PAGE_DESC *TFstPg;
			BOOL isSkip;

			TFstPg = FstPg2;	/*��¼��ʼ�ͷŵ�λ��*/
			isSkip = FALSE;
			do	/*ɾ����Ӧ��ȫ���ǿ�ҳ*/
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = *FstPg2) & PAGE_ATTR_P)	/*����ҳ����*/
				{
					if ((pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P) && (PgAddr >> 12) == (*FstPg >> 12))	/*�غ�ҳ�����ͷ�*/
						isSkip = TRUE;
					else
					{
						LockFreePage(PgAddr);	/*�ͷ�����ҳ*/
						*FstPg2 &= PAGE_ATTR_AVL;
					}
				}
			}
			while (((DWORD)(++FstPg, ++FstPg2) & 0xFFF) && FstPg < EndPg);
			if (isSkip)
				goto skip;
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg2 & 0xFFF; FstPg++, FstPg2++)
				if (*FstPg2 & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pt[(DWORD)TFstPg >> 12] &= PAGE_ATTR_AVL;
skip:		continue;
		}
		else	/*����ҳĿ¼�����*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg2 & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	FstPg = &pt[(DWORD)addr >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	while (FstPg < EndPg)	/*�޸�ҳ��,ӳ���ַ*/
	{
		if (pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P)	/*Դҳ�����*/
		{
			do
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*Դҳ����*/
				{
					if (!(pt[(DWORD)FstPg2 >> 12] & PAGE_ATTR_P))	/*Ŀ��ҳ������*/
					{
						DWORD PtAddr;	/*ҳ��������ַ*/

						if ((PtAddr = LockAllocPage()) == 0)	/*����Ŀ��ҳ��*/
						{
							res = ERROR_HAVENO_PMEM;
							goto skip2;
						}
						pt[(DWORD)FstPg2 >> 12] |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr;	/*�����û�дȨ��*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*���ҳ��*/
					}
					if (!(*FstPg2 & PAGE_ATTR_P))	/*Ŀ��ҳ������*/
						*FstPg2 |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | (PgAddr & 0xFFFFF000);	/*�����û�дȨ��*/
				}
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
		}
		else	/*ԴҳĿ¼�����*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
skip2:
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
	if (DstProc->PageReadAddr == NULL || DstProc->PageReadAddr != addr2)
		ulock(&DstProc->Page_l);
	clisub(&DstProc->Map_l);	/*����ӳ�����*/
	lock(&CurProc->Page_l);
	ClearPage(&pt[(DWORD)addr >> 12], EndPg, FALSE);	/*�����ǰ���̵�ӳ��*/
	ulock(&CurProc->Page_l);
	LockFreeUFData(CurProc, (void*)((DWORD)addr & 0xFFFFF000), map->siz);
	FreeMap(map);
	if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
		return ERROR_HAVENO_MSGDESC;
	msg->ptid = ptid;	/*������Ϣ*/
	msg->data[0] = MSG_ATTR_UNMAP;
	msg->data[1] = (DWORD)addr2;
	memcpy32(msg->data + 2, argv, MSG_DATA_LEN - 2);
	if (res != NO_ERROR)
		msg->data[2] = res;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*���ͳ���ӳ����Ϣ*/
	{
		FreeMsg(msg);
		return res;
	}
	return NO_ERROR;
}

/*ȡ��ӳ����̹����ַ,������ȡ����Ϣ*/
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
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*ӳ���ϵ���̵�ҳ��*/
	while (FstPg < EndPg)	/*Ŀ¼���Ӧ��ҳ���Ӧ��ÿҳ�ֱ����*/
	{
		DWORD PtAddr;	/*ҳ��������ַ*/

		if ((PtAddr = pt[(DWORD)FstPg2 >> 12]) & PAGE_ATTR_P)	/*ҳ�����*/
		{
			PAGE_DESC *TFstPg;

			TFstPg = FstPg2;	/*��¼��ʼ�ͷŵ�λ��*/
			do	/*ɾ����Ӧ��ȫ���ǿ�ҳ*/
			{
				DWORD PgAddr;	/*ҳ�������ַ*/

				if ((PgAddr = *FstPg2) & PAGE_ATTR_P)	/*����ҳ����*/
				{
					if (!(pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P) || (PgAddr >> 12) != (*FstPg >> 12))	/*�ͷŷ��غ�ҳ*/
						LockFreePage(PgAddr);	/*�ͷ�����ҳ*/
					*FstPg2 = 0;
				}
			}
			while (((DWORD)(++FstPg, ++FstPg2) & 0xFFF) && FstPg < EndPg);
			for (; (DWORD)TFstPg & 0xFFF; TFstPg--)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(TFstPg - 1) & PAGE_ATTR_P)
					goto skip;
			for (; (DWORD)FstPg2 & 0xFFF; FstPg++, FstPg2++)
				if (*FstPg2 & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pt[(DWORD)TFstPg >> 12] = 0;
skip:		continue;
		}
		else	/*����ҳĿ¼�����*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg2 & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
	ulock(&DstProc->Page_l);
	clisub(&DstProc->Map_l);	/*���˽��ӳ�����*/
	LockFreeUFData(DstProc, (void*)((DWORD)addr2 & 0xFFFFF000), map->siz);
	FreeMap(map);
	if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
		return ERROR_HAVENO_MSGDESC;
	msg->ptid = ptid;	/*������Ϣ*/
	msg->data[0] = MSG_ATTR_CNLMAP;
	msg->data[1] = (DWORD)addr2;
	memcpy32(msg->data + 2, argv, MSG_DATA_LEN - 2);
	if ((res = SendMsg(msg)) != NO_ERROR)	/*����ȡ��ӳ����Ϣ*/
	{
		FreeMsg(msg);
		return res;
	}
	return NO_ERROR;
}

/*������̵�ӳ�����*/
void FreeAllMap()
{
	PROCESS_DESC *CurProc;
	MAPBLK_DESC *CurMap, *map;
	DWORD data[MSG_DATA_LEN - 2];

	data[0] = MSG_ATTR_PROCEXIT;
	CurProc = CurPmd;
	while (CurProc->Map_l)	/*�ȴ���������ӳ�����*/
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

/*ҳ���ϴ������*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *CurExec;
	void *addr;
	long res;

	/*�����жϴ��������ǰ�ж��Ѿ��ر�*/
	addr = GetPageFaultAddr();
	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	CurThed->attr &= (~THED_ATTR_APPS);	/*����ϵͳ����̬*/
	if (addr < UADDR_OFF)	/*���̷Ƿ������ں˿ռ�*/
	{
		MESSAGE_DESC *msg;

		if ((msg = AllocMsg()) != NULL)	/*֪ͨ����������쳣��Ϣ*/
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

		if ((msg = AllocMsg()) != NULL)	/*֪ͨ����������쳣��Ϣ*/
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
	CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
}

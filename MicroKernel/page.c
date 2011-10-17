/*	page.c for ulios
	���ߣ�����
	���ܣ���ҳ�ڴ����
	����޸����ڣ�2009-05-29
*/

#include "knldef.h"

/*�����ַӳ��ṹ*/
MAPBLK_DESC *AllocMap()
{
	MAPBLK_DESC *map;

	cli();
	if (FstMap == NULL)
	{
		sti();
		return NULL;
	}
	FstMap = (map = FstMap)->nxt;
	sti();
	return map;
}

/*�ͷŵ�ַӳ��ṹ*/
void FreeMap(MAPBLK_DESC *map)
{
	cli();
	map->nxt = FstMap;
	FstMap = map;
	sti();
}

/*ӳ�����ȡ�õ�ַӳ��ṹ,������Ŀ�����*/
static MAPBLK_DESC *GetMap(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

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
			return map;
		}
	return NULL;
}

/*��ӳ�����ȡ�õ�ַӳ��ṹ,������Ŀ�����*/
static MAPBLK_DESC *GetMap2(PROCESS_DESC *proc, void *addr)
{
	PROCESS_DESC *DstProc;
	MAPBLK_DESC *PreMap, *CurMap, *map;

	for (PreMap = NULL, CurMap = proc->map2; CurMap; CurMap = (PreMap = CurMap)->nxt2)
		if (CurMap->addr2 == addr)
		{
			map = CurMap;
			if (PreMap)	/*�ӱ�ӳ����̵�ӳ��ṹ������ɾ��*/
				PreMap->nxt2 = map->nxt2;
			else
				proc->map2 = map->nxt2;
			DstProc = pmt[map->ptid.ProcID];
			for (PreMap = NULL, CurMap = DstProc->map; CurMap; CurMap = (PreMap = CurMap)->nxt)
				if (CurMap == map)
					break;
			if (PreMap)	/*��ӳ����̵�ӳ��ṹ������ɾ��*/
				PreMap->nxt = map->nxt;
			else
				DstProc->map = map->nxt;
			return map;
		}
	return NULL;
}

/*����ַ���Ƿ���ӳ����*/
static MAPBLK_DESC *CheckInMap(MAPBLK_DESC *map, void *fst, void *end)
{
	while (map)
	{
		if ((DWORD)end > ((DWORD)map->addr & 0xFFFFF000) && (DWORD)fst < ((DWORD)map->addr & 0xFFFFF000) + (map->siz & 0xFFFFF000))
			return map;
		map = map->nxt;
	}
	return NULL;
}

/*����ַ���Ƿ��ѱ�ӳ��*/
static MAPBLK_DESC *CheckInMap2(MAPBLK_DESC *map2, void *fst, void *end)
{
	while (map2)
	{
		if ((DWORD)end > ((DWORD)map2->addr2 & 0xFFFFF000) && (DWORD)fst < ((DWORD)map2->addr2 & 0xFFFFF000) + (map2->siz & 0xFFFFF000))
			return map2;
		map2 = map2->nxt2;
	}
	return NULL;
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

/*���������ҳ��ַ*/
long FillConAddr(PAGE_DESC *FstPg, PAGE_DESC *EndPg, DWORD PhyAddr, DWORD attr)
{
	PAGE_DESC *TstPg;
	BOOL isRefreshTlb;

	for (TstPg = (PAGE_DESC*)(*((DWORD*)&FstPg) & 0xFFFFF000); TstPg < EndPg; TstPg += 0x400)	/*������ַ��ҳ��߽�*/
	{
		if (!(pt[(DWORD)TstPg >> 12] & PAGE_ATTR_P))	/*ҳ������*/
		{
			DWORD PtAddr;	/*ҳ��������ַ*/

			if ((PtAddr = LockAllocPage()) == 0)
				return KERR_OUT_OF_PHYMEM;
			pt[(DWORD)TstPg >> 12] = attr | PtAddr;
			memset32(TstPg, 0, 0x400);	/*���ҳ��*/
		}
	}
	isRefreshTlb = FALSE;
	PhyAddr |= attr;
	for (; FstPg < EndPg; FstPg++)	/*�޸�ҳ��,ӳ���ַ*/
	{
		DWORD PgAddr;	/*ҳ�������ַ*/

		if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*����ҳ����*/
		{
			LockFreePage(PgAddr);	/*�ͷ�����ҳ*/
			isRefreshTlb = TRUE;
		}
		*FstPg = PhyAddr;
		PhyAddr += PAGE_SIZE;
	}
	if (isRefreshTlb)
		RefreshTlb();
	return NO_ERROR;
}

/*���ҳ����*/
long FillPage(EXEC_DESC *exec, void *addr, DWORD ErrCode)
{
	PAGE_DESC *CurPt, *CurPt0, *CurPg, *CurPg0;

	if (ErrCode & PAGE_ATTR_W)	/*����Ƿ�Ҫдֻ����ӳ����*/
	{
		MAPBLK_DESC *TmpMap;

		cli();
		TmpMap = CheckInMap(CurPmd->map, (void*)((DWORD)addr & 0xFFFFF000), (void*)(((DWORD)addr + PAGE_SIZE) & 0xFFFFF000));
		sti();
		if (TmpMap && !(TmpMap->siz & PAGE_ATTR_W))
			return KERR_WRITE_RDONLY_ADDR;
	}
	CurPg = &pt[(DWORD)addr >> 12];
	CurPg0 = &pt0[(DWORD)addr >> 12];
	CurPt = &pt[(DWORD)CurPg >> 12];
	CurPt0 = &pt[(DWORD)CurPg0 >> 12];
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPt & PAGE_ATTR_P))	/*ҳ������*/
	{
		if (*CurPt0 & PAGE_ATTR_P)	/*�����д���*/
			*CurPt = *CurPt0;	/*ӳ�丱��ҳ��*/
		else	/*�����в�����*/
		{
			DWORD PtAddr;	/*ҳ��������ַ*/
			void *fst, *end;

			if ((PtAddr = LockAllocPage()) == 0)	/*������ҳ��*/
				return KERR_OUT_OF_PHYMEM;
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
				return KERR_OUT_OF_PHYMEM;
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
	if (!(ErrCode & PAGE_ATTR_P) && !(*CurPg & PAGE_ATTR_P))	/*ҳ������*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*����ҳ��͸���ҳ����*/
			*CurPg = *CurPg0;	/*ӳ�丱��ҳ*/
		else	/*����ҳ��򸱱�ҳ������*/
		{
			DWORD PgAddr;	/*ҳ�������ַ*/
			void *fst, *end;

			if ((PgAddr = LockAllocPage()) == 0)	/*������ҳ*/
				return KERR_OUT_OF_PHYMEM;
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
				data[MSG_API_ID] = FS_API_READPAGE;	/*���ļ�ϵͳ������Ϣ��ȡ��ִ���ļ�ҳ*/
				data[3] = buf - exec->CodeOff + exec->CodeSeek;
				ptid = kpt[FS_KPORT];
				if ((data[MSG_API_ID] = MapProcAddr(buf, siz, &ptid, TRUE, FALSE, data, FS_OUT_TIME)) != NO_ERROR)
					return data[MSG_API_ID];
				if (data[MSG_RES_ID] != NO_ERROR)
					return data[MSG_RES_ID];
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
				data[MSG_API_ID] = FS_API_READPAGE;	/*���ļ�ϵͳ������Ϣ��ȡ��ִ���ļ�ҳ*/
				data[3] = buf - exec->DataOff + exec->DataSeek;
				ptid = kpt[FS_KPORT];
				if ((data[MSG_API_ID] = MapProcAddr(buf, siz, &ptid, TRUE, FALSE, data, FS_OUT_TIME)) != NO_ERROR)
					return data[MSG_API_ID];
				if (data[MSG_RES_ID] != NO_ERROR)
					return data[MSG_RES_ID];
				*CurPg0 = PgAddr;
			}
		}
	}
	if ((ErrCode & PAGE_ATTR_W) && !(*CurPg & PAGE_ATTR_W))	/*ҳд����*/
	{
		if ((*CurPt0 & PAGE_ATTR_P) && (*CurPg0 & PAGE_ATTR_P))	/*����ҳ����*/
		{
			DWORD PgAddr;	/*ҳ�������ַ*/

			if ((PgAddr = LockAllocPage()) == 0)	/*������ҳ*/
				return KERR_OUT_OF_PHYMEM;
			*CurPg = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PgAddr;	/*�޸�ҳ��,����дȨ��*/
			RefreshTlb();
			pt[(PT_ID << 10) | PG0_ID] = *CurPt0;	/*������ʱӳ��*/
			memcpy32((void*)((DWORD)addr & 0xFFFFF000), &pg0[(DWORD)addr & 0x003FF000], 0x400);	/*ҳдʱ����*/
			pt[(PT_ID << 10) | PG0_ID] = 0;	/*�����ʱӳ��*/
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
			PAGE_DESC *TstPg;

			TstPg = FstPg;	/*��¼��ʼ�ͷŵ�λ��*/
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
			while ((DWORD)TstPg & 0xFFF)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(--TstPg) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg & 0xFFF)
				if (*(FstPg++) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pt[(DWORD)TstPg >> 12] = 0;
skip:		continue;
		}
		else	/*����ҳ���*/
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
			PAGE_DESC *TstPg;

			TstPg = FstPg;	/*��¼��ʼ�ͷŵ�λ��*/
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
			while ((DWORD)TstPg & 0xFFF)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(--TstPg) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg & 0xFFF)
				if ((FstPg0++, *(FstPg++)) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pt[(DWORD)TstPg >> 12] = 0;
skip:		continue;
		}
		else	/*����ҳ���*/
		{
			FstPg = (PAGE_DESC*)(((DWORD)FstPg + 0x1000) & 0xFFFFF000);
			FstPg0 = (PAGE_DESC*)(((DWORD)FstPg0 + 0x1000) & 0xFFFFF000);
		}
	}
	if (isRefreshTlb)
		RefreshTlb();
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
		return KERR_NO_DRIVER_PRIVILEGE;	/*����������ȨAPI*/
	if (siz == 0)
		return KERR_MAPSIZE_IS_ZERO;
	siz = (PhyAddr + siz + 0x00000FFF) & 0xFFFFF000;	/*����PhyAddr,siz��ҳ�߽�,siz��ʱ����������ַ*/
	*((DWORD*)addr) = PhyAddr & 0x00000FFF;	/*��¼��ַ��������ͷ*/
	PhyAddr &= 0xFFFFF000;
	if (siz <= PhyAddr)	/*����Խ��*/
		return KERR_MAPSIZE_TOO_LONG;
	if (PhyAddr < BOOTDAT_ADDR || (PhyAddr < UPHYMEM_ADDR + (PmmLen << 12) && siz > (DWORD)kdat))	/*�벻����ӳ����������غ�*/
		return KERR_ILLEGAL_PHYADDR_MAPED;
	siz -= PhyAddr;	/*siz�ָ�Ϊӳ���ֽ���*/
	if ((CurBlk = LockAllocUBlk(CurProc, siz)) == NULL)
		return KERR_OUT_OF_LINEADDR;
	MapAddr = CurBlk->addr;
	cli();
	if (CheckInMap2(CurProc->map2, MapAddr, MapAddr + siz))	/*��������Ƿ��ѱ�ӳ��*/
	{
		sti();
		LockFreeUBlk(CurProc, CurBlk);
		return KERR_PAGE_ALREADY_MAPED;
	}
	lock(&CurProc->Page_l);
	FstPg = &pt[(DWORD)MapAddr >> 12];
	EndPg = &pt[((DWORD)MapAddr + siz) >> 12];
	if (FillConAddr(FstPg, EndPg, PhyAddr, PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U) != NO_ERROR)
	{
		ClearPage(FstPg, EndPg, TRUE);
		ulock(&CurProc->Page_l);
		LockFreeUBlk(CurProc, CurBlk);
		return KERR_OUT_OF_PHYMEM;
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
		return KERR_MAPSIZE_IS_ZERO;
	if ((CurBlk = LockAllocUBlk(CurPmd, siz)) == NULL)	/*ֻ����ռ伴��*/
		return KERR_OUT_OF_LINEADDR;
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
		return KERR_ADDRARGS_NOT_FOUND;
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
	void *MapAddr, *AlgAddr;
	PAGE_DESC *FstPg, *FstPg2, *EndPg;
	DWORD AlgSize;
	MESSAGE_DESC *msg;
	long res;

	if (siz == 0)
		return KERR_MAPSIZE_IS_ZERO;
	AlgAddr = (void*)((DWORD)addr & 0xFFFFF000);
	AlgSize = ((DWORD)addr + siz + 0x00000FFF) & 0xFFFFF000;	/*����ProcAddr,siz��ҳ�߽�,AlgSiz��ʱ����������ַ*/
	if (AlgSize <= (DWORD)AlgAddr)	/*����Խ��*/
		return KERR_MAPSIZE_TOO_LONG;
	if (AlgAddr < UADDR_OFF)	/*�벻����ӳ����������غ�*/
		return KERR_ACCESS_ILLEGAL_ADDR;
	if (ptid->ProcID >= PMT_LEN)
		return KERR_INVALID_PROCID;
	if (ptid->ThedID >= TMT_LEN)
		return KERR_INVALID_THEDID;
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (ptid->ProcID == CurThed->id.ProcID)	/*�������������ӳ��*/
		return KERR_PROC_SELF_MAPED;
	if (isChkExec)
	{
		EXEC_DESC *CurExec;

		CurExec = CurProc->exec;	/*���ӳ�����������κ����ݶε��غ�*/
		lock(&CurProc->Page_l);
		lockw(&CurExec->Page_l);
		if (CurExec->CodeOff < CurExec->CodeEnd && CurExec->CodeOff < (void*)AlgSize && CurExec->CodeEnd > AlgAddr)	/*�������ӳ�������غ�*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(AlgAddr < CurExec->CodeOff ? CurExec->CodeOff : AlgAddr) & 0xFFFFF000);
			end = ((void*)AlgSize > CurExec->CodeEnd ? CurExec->CodeEnd : (void*)AlgSize);
			while (fst < end)	/*��䱻ӳ��Ĵ����*/
			{
				if ((res = FillPage(CurExec, fst, isWrite ? PAGE_ATTR_W : 0)) != NO_ERROR)
				{
					ulockw(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					return res;
				}
				fst += PAGE_SIZE;
			}
		}
		if (CurExec->DataOff < CurExec->DataEnd && CurExec->DataOff < (void*)AlgSize && CurExec->DataEnd > AlgAddr)	/*���ݶ���ӳ�������غ�*/
		{
			void *fst, *end;

			fst = (void*)((DWORD)(AlgAddr < CurExec->DataOff ? CurExec->DataOff : AlgAddr) & 0xFFFFF000);
			end = ((void*)AlgSize > CurExec->DataEnd ? CurExec->DataEnd : (void*)AlgSize);
			while (fst < end)	/*��䱻ӳ������ݶ�*/
			{
				if ((res = FillPage(CurExec, fst, isWrite ? PAGE_ATTR_W : 0)) != NO_ERROR)
				{
					ulockw(&CurExec->Page_l);
					ulock(&CurProc->Page_l);
					return res;
				}
				fst += PAGE_SIZE;
			}
		}
		ulockw(&CurExec->Page_l);
		ulock(&CurProc->Page_l);
	}
	cli();	/*Ҫ�����������̵���Ϣ,���Է�ֹ�����л�*/
	DstProc = pmt[ptid->ProcID];
	if (DstProc == NULL || (DstProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return KERR_PROC_NOT_EXIST;
	}
	DstThed = DstProc->tmt[ptid->ThedID];
	if (DstThed == NULL || (DstThed->attr & THED_ATTR_DEL))
	{
		sti();
		return KERR_THED_NOT_EXIST;
	}
	DstProc->MapCou++;
	sti();
	AlgSize -= (DWORD)AlgAddr;	/*AlgSize�ָ�Ϊӳ���ֽ���*/
	if ((MapAddr = LockAllocUFData(DstProc, AlgSize)) == NULL)	/*�����û��ռ�*/
	{
		clisub(&DstProc->MapCou);
		return KERR_OUT_OF_LINEADDR;
	}
	if ((map = AllocMap()) == NULL)	/*����ӳ�����ṹ*/
	{
		LockFreeUFData(DstProc, MapAddr, AlgSize);
		clisub(&DstProc->MapCou);
		return KERR_OUT_OF_LINEADDR;
	}
	map->addr = (void*)((DWORD)MapAddr | ((DWORD)addr & 0x00000FFF));	/*����ӳ��ṹ*/
	map->addr2 = addr;
	map->siz = AlgSize | (isWrite ? PAGE_ATTR_W : 0);
	map->ptid = *ptid;
	map->ptid2 = CurThed->id;
	cli();
	if (isWrite && CurProc->map)	/*����Ƿ�Ҫдֻ����ӳ����*/
	{
		MAPBLK_DESC *TmpMap;

		TmpMap = CurProc->map;
		for (;;)
		{
			if ((TmpMap = CheckInMap(TmpMap, AlgAddr, AlgAddr + AlgSize)) == NULL)
				break;
			if (!(TmpMap->siz & PAGE_ATTR_W))
			{
				FreeMap(map);
				LockFreeUFData(DstProc, MapAddr, AlgSize);
				clisub(&DstProc->MapCou);
				return KERR_WRITE_RDONLY_ADDR;
			}
			TmpMap = TmpMap->nxt;
		}
	}
	map->nxt = DstProc->map;
	map->nxt2 = CurProc->map2;
	CurProc->map2 = DstProc->map = map;
	if (isChkExec)
	{
		clilock(CurProc->Page_l || DstProc->Page_l);
		CurProc->Page_l = TRUE;
		DstProc->Page_l = TRUE;
		sti();
	}
	else
		lock(&DstProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid->ProcID]);	/*ӳ���ϵ���̵�ҳ��*/
	FstPg = &pt[(DWORD)AlgAddr >> 12];
	EndPg = &pt[((DWORD)AlgAddr + AlgSize) >> 12];
	FstPg2 = &pt2[(DWORD)MapAddr >> 12];
	while (FstPg < EndPg)	/*Դ��ַѭ��*/
	{
		if (pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P)	/*Դҳ�����*/
		{
			do
			{
				DWORD PgAddr;	/*Դҳ�������ַ*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*Դҳ����*/
				{
					PAGE_DESC *CurPt2;
					DWORD PgAddr2;	/*Ŀ��ҳ�������ַ*/

					CurPt2 = &pt[(DWORD)FstPg2 >> 12];
					if (!(*CurPt2 & PAGE_ATTR_P))	/*Ŀ��ҳ������*/
					{
						DWORD PtAddr2;	/*Ŀ��ҳ��������ַ*/

						if ((PtAddr2 = LockAllocPage()) == 0)	/*����Ŀ��ҳ��*/
						{
							ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
							ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
							ulock(&DstProc->Page_l);
							if (isChkExec)
								ulock(&CurProc->Page_l);
							cli();
							GetMap2(CurProc, addr);
							cli();
							FreeMap(map);
							LockFreeUFData(DstProc, MapAddr, AlgSize);
							clisub(&DstProc->MapCou);
							return KERR_OUT_OF_PHYMEM;
						}
						*CurPt2 = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr2;	/*�����û�Ȩ��*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*���ҳ��*/
					}
					else if ((PgAddr2 = *FstPg2) & PAGE_ATTR_P)	/*Ŀ��ҳ����*/
					{
						cli();
						if (CheckInMap2(DstProc->map2, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)))	/*�ѱ�ӳ��*/
						{
							sti();
							ClearPage(&pt2[(DWORD)MapAddr >> 12], FstPg2, FALSE);
							ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
							ulock(&DstProc->Page_l);
							if (isChkExec)
								ulock(&CurProc->Page_l);
							cli();
							GetMap2(CurProc, addr);
							sti();
							FreeMap(map);
							LockFreeUFData(DstProc, MapAddr, AlgSize);
							clisub(&DstProc->MapCou);
							return KERR_PAGE_ALREADY_MAPED;
						}
						else
							LockFreePage(PgAddr2);	/*�ͷ�Ŀ��ҳ*/
					}
					if (isWrite)
					{
						*CurPt2 |= PAGE_ATTR_W;	/*����ҳ��дȨ��*/
						*FstPg2 = PAGE_ATTR_W | PgAddr;	/*����ҳдȨ��*/
					}
					else
					{
						if (((DWORD)CurPt2 << 20) >= (DWORD)MapAddr && ((DWORD)(CurPt2 + 1) << 20) <= (DWORD)MapAddr + AlgSize)	/*ҳ������ȫ��λ��ӳ��ռ���*/
							*CurPt2 &= (~PAGE_ATTR_W);	/*�ر�ҳ��дȨ��*/
						*FstPg2 = (~PAGE_ATTR_W) & PgAddr;	/*�ر�ҳдȨ��*/
					}
				}
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
		}
		else	/*Դҳ������*/
		{
			DWORD step;

			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
	ulock(&DstProc->Page_l);
	if (isChkExec)
		ulock(&CurProc->Page_l);
	clisub(&DstProc->MapCou);	/*����ӳ�����*/
	if (!isChkExec)
		lockset((volatile DWORD*)(&CurProc->PageReadAddr), (DWORD)map->addr2);
	if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
		return KERR_MSG_NOT_ENOUGH;
	msg->ptid = *ptid;	/*������Ϣ*/
	memcpy32(msg->data, argv, MSG_DATA_LEN);
	msg->data[MSG_ATTR_ID] = (argv[MSG_API_ID] & MSG_API_MASK) | (isWrite ? MSG_ATTR_RWMAP : MSG_ATTR_ROMAP);
	msg->data[MSG_ADDR_ID] = (DWORD)map->addr;
	msg->data[MSG_SIZE_ID] = siz;
	if ((res = SendMsg(msg)) != NO_ERROR)	/*����ӳ����Ϣ*/
	{
		FreeMsg(msg);
		return res;
	}
	if (cs)	/*�ȴ�������Ϣ*/
	{
		if ((res = RecvProcMsg(&msg, *ptid, cs)) != NO_ERROR)
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
	BOOL isRefreshTlb;
	MESSAGE_DESC *msg;
	long res;

	CurProc = CurPmd;
	cli();
	map = GetMap(CurProc, addr);
	if (map == NULL)
	{
		sti();
		return KERR_ADDRARGS_NOT_FOUND;
	}
	ptid = map->ptid2;
	addr2 = map->addr2;
	DstProc = pmt[ptid.ProcID];
	DstProc->MapCou++;
	if (DstProc->PageReadAddr != addr2)
	{
		clilock(DstProc->Page_l || CurProc->Page_l);
		DstProc->Page_l = TRUE;
		CurProc->Page_l = TRUE;
		sti();
	}
	else
		lock(&CurProc->Page_l);
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*ӳ���ϵ���̵�ҳ��*/
	res = NO_ERROR;
	isRefreshTlb = FALSE;
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + (map->siz & 0xFFFFF000)) >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	while (FstPg < EndPg)	/*Դ��ַѭ��*/
	{
		DWORD PtAddr;	/*Դҳ��������ַ*/

		if ((PtAddr = pt[(DWORD)FstPg >> 12]) & PAGE_ATTR_P)	/*Դҳ�����*/
		{
			PAGE_DESC *TstPg;

			TstPg = FstPg;	/*��¼��ʼ�ͷŵ�λ��*/
			do
			{
				DWORD PgAddr;	/*Դҳ�������ַ*/

				if ((PgAddr = *FstPg) & PAGE_ATTR_P)	/*Դҳ����*/
				{
					DWORD PgAddr2;	/*Ŀ��ҳ�������ַ*/

					if (!(pt[(DWORD)FstPg2 >> 12] & PAGE_ATTR_P))	/*Ŀ��ҳ������*/
					{
						DWORD PtAddr2;	/*Ŀ��ҳ��������ַ*/

						if ((PtAddr2 = LockAllocPage()) == 0)	/*����Ŀ��ҳ��*/
						{
							res = KERR_OUT_OF_PHYMEM;
							goto skip2;
						}
						pt[(DWORD)FstPg2 >> 12] |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | PtAddr2;	/*�����û�дȨ��*/
						memset32((void*)((DWORD)FstPg2 & 0xFFFFF000), 0, 0x400);	/*���ҳ��*/
					}
					else if ((PgAddr2 = *FstPg2) & PAGE_ATTR_P && (PgAddr2 >> 12) != (PgAddr >> 12))	/*Ŀ��ҳ����,����Դҳ���غ�*/
					{
						cli();
						if (CheckInMap(DstProc->map, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)) ||
							CheckInMap2(DstProc->map2, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)))	/*��ӳ����*/
						{
							sti();
							pt[(PT_ID << 10) | PG0_ID] = pt[(DWORD)FstPg2 >> 12];	/*������ʱӳ��*/
							memcpy32(&pg0[((DWORD)FstPg2 << 10) & 0x003FF000], (void*)((DWORD)FstPg << 10), 0x400);	/*����ҳ����*/
							pt[(PT_ID << 10) | PG0_ID] = 0;	/*�����ʱӳ��*/
							LockFreePage(PgAddr);	/*�ͷ�Դҳ*/
							PgAddr = PgAddr2;
						}
						else
							LockFreePage(PgAddr2);	/*�ͷ�Ŀ��ҳ*/
					}
					*FstPg2 |= PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_U | (PgAddr & 0xFFFFF000);	/*�����û�дȨ��*/
					*FstPg = 0;
					isRefreshTlb = TRUE;
				}
				else
					*FstPg = 0;
			}
			while (((DWORD)(++FstPg2, ++FstPg) & 0xFFF) && FstPg < EndPg);
			while ((DWORD)TstPg & 0xFFF)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(--TstPg) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg & 0xFFF)
				if (*(FstPg++) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr);	/*�ͷ�ҳ��*/
			pt[(DWORD)TstPg >> 12] = 0;
skip:		continue;
		}
		else	/*Դҳ������*/
		{
			DWORD step;

			pt[(DWORD)FstPg >> 12] = 0;
			step = 0x1000 - ((DWORD)FstPg & 0xFFF);
			*((DWORD*)&FstPg) += step;
			*((DWORD*)&FstPg2) += step;
		}
	}
skip2:
	if (isRefreshTlb)
		RefreshTlb();
	ulock(&pt[(PT_ID << 10) | PT2_ID]);	/*�����ϵ����ҳ���ӳ��*/
	ulock(&CurProc->Page_l);
	if (DstProc->PageReadAddr != addr2)
		ulock(&DstProc->Page_l);
	clisub(&DstProc->MapCou);	/*����ӳ�����*/
	if (DstProc->PageReadAddr == addr2)
		DstProc->PageReadAddr = NULL;
	FreeMap(map);
	LockFreeUFData(CurProc, (void*)((DWORD)addr & 0xFFFFF000), map->siz & 0xFFFFF000);
	if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
		return KERR_MSG_NOT_ENOUGH;
	msg->ptid = ptid;	/*������Ϣ*/
	memcpy32(msg->data, argv, MSG_DATA_LEN);
	msg->data[MSG_ATTR_ID] = (argv[MSG_API_ID] & MSG_API_MASK) | MSG_ATTR_UNMAP;
	msg->data[MSG_ADDR_ID] = (DWORD)addr2;
	if (res != NO_ERROR)
		msg->data[MSG_RES_ID] = res;
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
	cli();
	map = GetMap2(CurProc, addr);
	if (map == NULL)
	{
		sti();
		return KERR_ADDRARGS_NOT_FOUND;
	}
	ptid = map->ptid;
	addr2 = map->addr;
	DstProc = pmt[ptid.ProcID];
	DstProc->MapCou++;
	clilock(CurProc->Page_l || DstProc->Page_l);
	CurProc->Page_l = TRUE;
	DstProc->Page_l = TRUE;
	sti();
	lockset(&pt[(PT_ID << 10) | PT2_ID], pddt[ptid.ProcID]);	/*ӳ���ϵ���̵�ҳ��*/
	FstPg = &pt[(DWORD)addr >> 12];
	EndPg = &pt[((DWORD)addr + (map->siz & 0xFFFFF000)) >> 12];
	FstPg2 = &pt2[(DWORD)addr2 >> 12];
	while (FstPg < EndPg)	/*Դ��ַѭ��*/
	{
		DWORD PtAddr2;	/*Ŀ��ҳ��������ַ*/

		if ((PtAddr2 = pt[(DWORD)FstPg2 >> 12]) & PAGE_ATTR_P)	/*ҳ�����*/
		{
			PAGE_DESC *TstPg2;

			TstPg2 = FstPg2;	/*��¼��ʼ�ͷŵ�λ��*/
			do
			{
				DWORD PgAddr2;	/*Ŀ��ҳ�������ַ*/

				if ((PgAddr2 = *FstPg2) & PAGE_ATTR_P)	/*����ҳ����*/
				{
					cli();
					if ((!(pt[(DWORD)FstPg >> 12] & PAGE_ATTR_P) || (PgAddr2 >> 12) != (*FstPg >> 12)) &&
						!CheckInMap2(DstProc->map2, (void*)((DWORD)FstPg2 << 10), (void*)((DWORD)(FstPg2 + 1) << 10)))	/*(Դҳ�����ڻ���Դҳ���غ�)��û��ӳ��*/
						LockFreePage(PgAddr2);	/*�ͷ�����ҳ*/
					sti();
					*FstPg2 = 0;
				}
			}
			while (((DWORD)(++FstPg, ++FstPg2) & 0xFFF) && FstPg < EndPg);
			while ((DWORD)TstPg2 & 0xFFF)	/*���ҳ���Ƿ���Ҫ�ͷ�*/
				if (*(--TstPg2) & PAGE_ATTR_P)
					goto skip;
			while ((DWORD)FstPg2 & 0xFFF)
				if ((FstPg++, *(FstPg2++)) & PAGE_ATTR_P)
					goto skip;
			LockFreePage(PtAddr2);	/*�ͷ�ҳ��*/
			pt[(DWORD)TstPg2 >> 12] = 0;
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
	ulock(&CurProc->Page_l);
	clisub(&DstProc->MapCou);	/*���˽��ӳ�����*/
	FreeMap(map);
	LockFreeUFData(DstProc, (void*)((DWORD)addr2 & 0xFFFFF000), map->siz & 0xFFFFF000);
	if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
		return KERR_MSG_NOT_ENOUGH;
	msg->ptid = ptid;	/*������Ϣ*/
	memcpy32(msg->data, argv, MSG_DATA_LEN);
	msg->data[MSG_ATTR_ID] = (argv[MSG_API_ID] & MSG_API_MASK) | MSG_ATTR_CNLMAP;
	msg->data[MSG_ADDR_ID] = (DWORD)addr2;
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
	DWORD data[MSG_DATA_LEN];

	data[MSG_API_ID] = (MSG_ATTR_PROCEXIT >> 16);
	CurProc = CurPmd;
	while (CurProc->MapCou)	/*�ȴ���������ӳ�����*/
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
			msg->data[MSG_ATTR_ID] = MSG_ATTR_EXCEP;
			msg->data[MSG_ADDR_ID] = (DWORD)addr;
			msg->data[MSG_SIZE_ID] = eip;
			msg->data[MSG_RES_ID] = KERR_ACCESS_ILLEGAL_ADDR;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		ThedExit(KERR_ACCESS_ILLEGAL_ADDR);
	}
	CurExec = CurProc->exec;
	lock(&CurProc->Page_l);
	lockw(&CurExec->Page_l);
	res = FillPage(CurExec, addr, ErrCode);
	ulockw(&CurExec->Page_l);
	ulock(&CurProc->Page_l);
	if (res != NO_ERROR)
	{
		MESSAGE_DESC *msg;

		if ((msg = AllocMsg()) != NULL)	/*֪ͨ����������쳣��Ϣ*/
		{
			msg->ptid = kpt[REP_KPORT];
			msg->data[MSG_ATTR_ID] = MSG_ATTR_EXCEP;
			msg->data[MSG_ADDR_ID] = (DWORD)addr;
			msg->data[MSG_SIZE_ID] = eip;
			msg->data[MSG_RES_ID] = res;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		ThedExit(res);
	}
	CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
}

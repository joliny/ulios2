/*	task.c for ulios
	���ߣ�����
	���ܣ���������������̺��߳�
	����޸����ڣ�2009-06-25
*/

#include "knldef.h"

/*��ʼ�����̹����*/
void InitPMT()
{
	memset32(pmt, 0, PMT_LEN * sizeof(PROCESS_DESC*) / sizeof(DWORD));
	EndPmd = FstPmd = pmt;
	CurPmd = NULL;
	PmdCou = 0;
	clock = 0;
	SleepList = NULL;
	LastI387 = NULL;
}

/*��ʼ���ں˽���*/
void InitKnlProc()
{
	memset32(&KnlTss, 0, sizeof(TSS) / sizeof(DWORD));
	KnlTss.cr3 = (DWORD)kpdt;
	KnlTss.io = sizeof(TSS);
	SetSegDesc(&gdt[UCODE_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_E | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);	/*��ʼ���û�GDT��*/
	SetSegDesc(&gdt[UDATA_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_D_W | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);
	SetSegDesc(&gdt[TSS_SEL >> 3], (DWORD)&KnlTss, sizeof(TSS) - 1, DESC_ATTR_P | SYSSEG_ATTR_T_TSS);
	__asm__("ltr %%ax":: "a"(TSS_SEL));
}

/*�����л�*/
static void SwitchTS()
{
	TSS *CurTss;

	if (CurPmd)	/*�о�������*/
		CurTss = &CurPmd->CurTmd->tss;
	else	/*û�о�������,�л����ں�*/
		CurTss = &KnlTss;
	SetSegDesc(&gdt[TSS_SEL >> 3], (DWORD)CurTss, sizeof(TSS) - 1, DESC_ATTR_P | SYSSEG_ATTR_T_TSS);	/*���������TSS������*/
	__asm__("ljmp %0, $0":: "i"(TSS_SEL));
}

/*����ս���ID*/
static inline long AllocPid(PROCESS_DESC *proc)
{
	if (FstPmd >= &pmt[PMT_LEN])
		return ERROR_HAVENO_PROCID;	// û�пս���ID
	proc->tmt[0]->id.ProcID = FstPmd - pmt;
	*FstPmd = proc;
	do
		FstPmd++;
	while (FstPmd < &pmt[PMT_LEN] && *FstPmd);
	if (EndPmd < FstPmd)
		EndPmd = FstPmd;
	PmdCou++;
	return NO_ERROR;
}

/*�ͷſս���ID*/
static inline void FreePid(PROCESS_DESC **pmd)
{
	*pmd = NULL;
	if (FstPmd > pmd)
		FstPmd = pmd;
	while (EndPmd > pmt && *(EndPmd - 1) == NULL)
		EndPmd--;
	PmdCou--;
}

/*������߳�ID*/
static inline long AllocTid(PROCESS_DESC *proc, THREAD_DESC *thed)
{
	if (proc->FstTmd >= &proc->tmt[TMT_LEN])
		return ERROR_HAVENO_THEDID;	// û�п��߳�ID
	thed->id.ThedID = proc->FstTmd - proc->tmt;
	*proc->FstTmd = thed;
	do
		proc->FstTmd++;
	while (proc->FstTmd < &proc->tmt[TMT_LEN] && *proc->FstTmd);
	if (proc->EndTmd < proc->FstTmd)
		proc->EndTmd = proc->FstTmd;
	proc->TmdCou++;
	return NO_ERROR;
}

/*�ͷſ��߳�ID*/
static inline void FreeTid(PROCESS_DESC *proc, THREAD_DESC **tmd)
{
	*tmd = NULL;
	if (proc->FstTmd > tmd)
		proc->FstTmd = tmd;
	while (proc->EndTmd > proc->tmt && *(proc->EndTmd - 1) == NULL)
		proc->EndTmd--;
	proc->TmdCou--;
}

/*����ʱ�̲߳�����ʱ��������*/
static inline void InsertSleepList(THREAD_DESC *thed)
{
	THREAD_DESC *PreThed, *NxtThed;

	for (PreThed = NULL, NxtThed = SleepList; NxtThed; NxtThed = (PreThed = NxtThed)->nxt)
		if (thed->WakeupClock < NxtThed->WakeupClock)	/*�����������*/
			break;
	thed->pre = PreThed;
	if (PreThed)
		PreThed->nxt = thed;
	else
		SleepList = thed;
	thed->nxt = NxtThed;
	if (NxtThed)
		NxtThed->pre = thed;
}

/*����ʱ�̴߳���ʱ��������ͷ��ɾ��*/
static inline void DeleteSleepList(THREAD_DESC *thed)
{
	THREAD_DESC *PreThed, *NxtThed;

	PreThed = thed->pre;
	NxtThed = thed->nxt;
	if (PreThed)
		PreThed->nxt = NxtThed;
	else
		SleepList = NxtThed;
	if (NxtThed)
		NxtThed->pre = PreThed;
}

/*�̵߳���,����ǰ���ж�*/
void schedul()
{
	if (CurPmd)	/*�о�������*/
	{
		PROCESS_DESC *NxtProc;
		THREAD_DESC *NxtThed;

		NxtProc = CurPmd->nxt;	/*ѡ����һ������*/
		NxtThed = NxtProc->CurTmd->nxt;	/*ѡ����һ���߳�*/
		if (NxtProc != CurPmd || NxtThed != NxtProc->CurTmd)	/*�����ǰ�����л�����ǰ�����򲻱��л�*/
		{
			CurPmd = NxtProc;
			NxtProc->CurTmd = NxtThed;
			SwitchTS();
		}
	}
}

/*�����߳�*/
void wakeup(THREAD_DESC *thed)
{
	PROCESS_DESC *CurProc, *NewProc;
	THREAD_DESC *CurThed;

	CurProc = CurPmd;
	NewProc = pmt[thed->id.ProcID];
	CurThed = NewProc->CurTmd;
	if (thed->attr & THED_ATTR_WAITTIME)	/*����ʱ������ɾ��*/
		DeleteSleepList(thed);
	thed->attr &= (~(THED_ATTR_SLEEP | THED_ATTR_WAITTIME));	/*�������״̬,�����߳�*/
	if (CurThed)	/*�о����߳�,���뻷������*/
	{
		THREAD_DESC *NxtThed;

		NxtThed = CurThed->nxt;
		thed->pre = CurThed;
		thed->nxt = NxtThed;
		CurThed->nxt = thed;
		NxtThed->pre = thed;
	}
	else	/*������û�о����߳�,���Ȼ��ѽ���*/
	{
		thed->pre = thed;	/*�̷߳���ջ�������*/
		thed->nxt = thed;
		if (CurProc)	/*�о�������,���뻷������*/
		{
			PROCESS_DESC *NxtProc;

			NxtProc = CurProc->nxt;
			NewProc->pre = CurProc;
			NewProc->nxt = NxtProc;
			CurProc->nxt = NewProc;
			NxtProc->pre = NewProc;
		}
		else	/*û�о�������,���̷���ջ�������*/
		{
			NewProc->pre = NewProc;
			NewProc->nxt = NewProc;
		}
	}
	CurPmd = NewProc;
	NewProc->CurTmd = thed;
	SwitchTS();
}

/*�����߳�*/
void sleep(DWORD cs)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (CurThed->nxt == CurThed)	/*ֻʣ��ǰһ�������߳�,��Ҫ��������*/
	{
		if (CurProc->nxt == CurProc)	/*ֻʣ��ǰһ����������,�л����ں�����*/
			CurPmd = NULL;
		else	/*����һ����������,�ӻ���������ɾ��*/
		{
			PROCESS_DESC *PreProc, *NxtProc;

			PreProc = CurProc->pre;
			NxtProc = CurProc->nxt;
			PreProc->nxt = NxtProc;
			NxtProc->pre = PreProc;
			CurPmd = NxtProc;
		}
		CurProc->CurTmd = NULL;
	}
	else	/*����һ�������߳�,�ӻ���������ɾ��*/
	{
		THREAD_DESC *PreThed, *NxtThed;

		PreThed = CurThed->pre;
		NxtThed = CurThed->nxt;
		PreThed->nxt = NxtThed;
		NxtThed->pre = PreThed;
		CurProc->CurTmd = NxtThed;
	}
	CurThed->attr |= THED_ATTR_SLEEP;	/*�����߳�*/
	if (cs != INVALID)	/*����ʱ������*/
	{
		CurThed->WakeupClock = clock + cs;
		CurThed->attr |= THED_ATTR_WAITTIME;
		InsertSleepList(CurThed);
	}
	SwitchTS();
	if (CurThed->attr & THED_ATTR_KILLED)	/*�̱߳�ɱ��*/
	{
		CurThed->attr &= (~THED_ATTR_KILLED);
		sti();
		ThedExit();
	}
}

/*�����߳�*/
long CreateThed(const DWORD *argv, THREAD_ID *ptid)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed, *NewThed, *NxtThed;

	CurProc = CurPmd;
	if ((NewThed = (THREAD_DESC*)LockKmalloc(sizeof(THREAD_DESC))) == NULL)
		return ERROR_HAVENO_KMEM;
	CurThed = CurProc->CurTmd;
	memset32(NewThed, 0, sizeof(THREAD_DESC) / sizeof(DWORD));
	NewThed->par = CurThed->id.ThedID;
	NewThed->id.ProcID = CurThed->id.ProcID;
	*(DWORD*)(&NewThed->WaitId) = INVALID;
	NewThed->tss.cr3 = CurThed->tss.cr3;
	NewThed->tss.eip = ThedStart;
	NewThed->tss.esp = (DWORD)NewThed + sizeof(THREAD_DESC);
	NewThed->tss.cs = KCODE_SEL;
	NewThed->tss.gs = NewThed->tss.fs = NewThed->tss.ds = NewThed->tss.ss = NewThed->tss.es = KDATA_SEL;
	NewThed->tss.io = sizeof(TSS);
	memcpy32(NewThed->kstk, argv, 3);	/*���Ʋ���*/
	cli();
	if (CurProc->attr & PROC_ATTR_DEL)	/*���ڱ�ɾ���Ľ��̲������߳�*/
	{
		sti();
		LockKfree(NewThed, sizeof(THREAD_DESC));
		return NO_ERROR;
	}
	if (AllocTid(CurProc, NewThed) != NO_ERROR)
	{
		sti();
		LockKfree(NewThed, sizeof(THREAD_DESC));
		return ERROR_HAVENO_THEDID;
	}
	NxtThed = CurThed->nxt;	/*���뵱ǰ�̺߳�*/
	NewThed->pre = CurThed;
	NewThed->nxt = NxtThed;
	CurThed->nxt = NewThed;
	NxtThed->pre = NewThed;
	sti();
	*ptid = NewThed->id;
	return NO_ERROR;
}

/*ɾ���߳�*/
void DeleteThed()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	THREAD_DESC **Thedi;
	DWORD ptid;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	clilock(Kmalloc_l || AllocPage_l);
	ptid = CurThed->id.ThedID;
	kfree(CurThed, sizeof(THREAD_DESC));
	FreeTid(CurProc, &CurProc->tmt[ptid]);
	for (Thedi = CurProc->tmt; Thedi < CurProc->EndTmd; Thedi++)	/*�������̵߳ĸ��߳�ID*/
		if (*Thedi && (*Thedi)->par == ptid)
			(*Thedi)->par = CurThed->par;
	if (CurThed->nxt == CurThed)	/*ֻʣ��ǰһ�������߳�,��Ҫ��������*/
	{
		if (CurProc->TmdCou == 0)	/*ֻʣ��ǰһ���߳�,���������Դ*/
		{
			PROCESS_DESC **Proci;

			FreePage(pt[(PT_ID << 10) | PT_ID]);
			ptid = CurThed->id.ProcID;
			kfree(CurProc, sizeof(PROCESS_DESC));
			FreePid(&pmt[ptid]);
			for (Proci = pmt; Proci < EndPmd; Proci++)	/*�����ӽ��̵ĸ�����ID*/
				if (*Proci && (*Proci)->par == ptid)
					(*Proci)->par = CurProc->par;
		}
		if (CurProc->nxt == CurProc)	/*ֻʣ��ǰһ����������,�л����ں�����*/
			CurPmd = NULL;
		else	/*����һ����������,�ӻ���������ɾ��*/
		{
			PROCESS_DESC *PreProc, *NxtProc;

			PreProc = CurProc->pre;
			NxtProc = CurProc->nxt;
			PreProc->nxt = NxtProc;
			NxtProc->pre = PreProc;
			CurPmd = NxtProc;
		}
		CurProc->CurTmd = NULL;
	}
	else	/*����һ�������߳�,�ӻ���������ɾ��*/
	{
		THREAD_DESC *PreThed, *NxtThed;

		PreThed = CurThed->pre;
		NxtThed = CurThed->nxt;
		PreThed->nxt = NxtThed;
		NxtThed->pre = PreThed;
		CurProc->CurTmd = NxtThed;
	}
	SwitchTS();
}

/*ɱ���߳�*/
long KillThed(WORD ThedID)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	THREAD_DESC *DstThed;

	if (ThedID >= TMT_LEN)
		return ERROR_WRONG_THEDID;
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	cli();
	DstThed = CurProc->tmt[ThedID];
	if (DstThed == NULL || DstThed == CurThed || (DstThed->attr & THED_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_THEDID;
	}
	DstThed->attr |= THED_ATTR_KILLED;	/*���Ϊ��ɱ��*/
	if (DstThed->attr & THED_ATTR_SLEEP)	/*�߳�����,���Ȼ����߳�*/
		wakeup(DstThed);
	sti();
	return NO_ERROR;
}

/*��������*/
long CreateProc(const DWORD *argv, THREAD_ID *ptid)
{
	PROCESS_DESC *CurProc, *NewProc;
	THREAD_DESC *NewThed;
	DWORD NewPdt, pdti;

	CurProc = CurPmd;
	if (CurProc && (CurProc->attr & PROC_ATTR_APPS) && (argv[0] & EXEC_ARGV_DRIVER))	/*Ӧ�ý�����Ȩ������������*/
		return ERROR_NOT_DRIVER;
	if ((NewProc = (PROCESS_DESC*)LockKmalloc(sizeof(PROCESS_DESC))) == NULL)
		return ERROR_HAVENO_KMEM;
	if ((NewThed = (THREAD_DESC*)LockKmalloc(sizeof(THREAD_DESC))) == NULL)
	{
		LockKfree(NewProc, sizeof(PROCESS_DESC));
		return ERROR_HAVENO_KMEM;
	}
	if ((NewPdt = LockAllocPage()) == 0)
	{
		LockKfree(NewThed, sizeof(THREAD_DESC));
		LockKfree(NewProc, sizeof(PROCESS_DESC));
		return ERROR_HAVENO_PMEM;
	}
	memset32(NewProc, 0, sizeof(PROCESS_DESC) / sizeof(DWORD));
	NewProc->par = CurProc ? CurProc->CurTmd->id.ProcID : INVALID;
	NewProc->MemSiz = PAGE_SIZE;	/*ҳĿ¼��ռ��*/
	NewProc->tmt[0] = NewThed;
	NewProc->EndTmd = NewProc->FstTmd = &NewProc->tmt[1];
	NewProc->CurTmd = NewThed;
	NewProc->TmdCou = 1;
	memset32(NewThed, 0, sizeof(THREAD_DESC) / sizeof(DWORD));
	NewThed->pre = NewThed;
	NewThed->nxt = NewThed;
	NewThed->par = INVALID;
	*(DWORD*)(&NewThed->WaitId) = INVALID;
	NewThed->tss.cr3 = NewPdt;
	NewThed->tss.eip = ProcStart;
	NewThed->tss.esp = (DWORD)NewThed + sizeof(THREAD_DESC);
	NewThed->tss.cs = KCODE_SEL;
	NewThed->tss.gs = NewThed->tss.fs = NewThed->tss.ds = NewThed->tss.ss = NewThed->tss.es = KDATA_SEL;
	NewThed->tss.io = sizeof(TSS);
	if (argv[0] & EXEC_ARGV_BASESRV)
		memcpy32(NewThed->kstk, argv, 3);	/*���Ʋ���*/
	else
	{
		memcpy32(NewThed->kstk, argv, 2);	/*���Ʋ���*/
		if (argv[2])
			strcpy((BYTE*)&NewThed->kstk[2], (const BYTE*)argv[2]);
	}
	cli();
	if (AllocPid(NewProc) != NO_ERROR)
	{
		sti();
		LockFreePage(NewPdt);
		LockKfree(NewThed, sizeof(THREAD_DESC));
		LockKfree(NewProc, sizeof(PROCESS_DESC));
		return ERROR_HAVENO_PROCID;
	}
	pdti = NewThed->id.ProcID;
	NewPdt |= PAGE_ATTR_P;
	pddt[pdti] = NewPdt;	/*ӳ���½��̵�ҳĿ¼��*/
	pdti <<= 10;
	memset32(&pdt[pdti], 0, 0x400);
	memcpy32(&pdt[pdti], kpdt, 4);	/*����ҳĿ¼��*/
	pdt[pdti | PT_ID] = NewPdt;	/*ӳ���½��̵�ҳ��*/
	if (CurProc)	/*�о�������,���뵱ǰ���̺�*/
	{
		PROCESS_DESC *NxtProc;

		NxtProc = CurProc->nxt;
		NewProc->pre = CurProc;
		NewProc->nxt = NxtProc;
		CurProc->nxt = NewProc;
		NxtProc->pre = NewProc;
	}
	else	/*û�о�������,���̷���ջ�������*/
	{
		NewProc->pre = NewProc;
		NewProc->nxt = NewProc;
		CurPmd = NewProc;
		SwitchTS();
	}
	sti();
	*ptid = NewThed->id;
	return NO_ERROR;
}

/*ɾ������*/
void DeleteProc()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC **Thedi;
	THREAD_DESC *CurThed;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	cli();
	CurProc->attr |= PROC_ATTR_DEL;	/*���Ϊ���ڱ�ɾ��*/
	for (Thedi = CurProc->tmt; Thedi < CurProc->EndTmd; Thedi++)
	{
		THREAD_DESC *DstThed;

		DstThed = *Thedi;
		if (DstThed && DstThed != CurThed && !(DstThed->attr & THED_ATTR_DEL))
		{
			DstThed->attr |= THED_ATTR_KILLED;	/*���Ϊ��ɱ��*/
			if (DstThed->attr & THED_ATTR_SLEEP)	/*�߳�����,���Ȼ����߳�*/
				wakeup(DstThed);
		}
	}
	sti();
	ThedExit();
}

/*ɱ������*/
long KillProc(WORD ProcID)
{
	PROCESS_DESC *CurProc;
	PROCESS_DESC *DstProc;
	THREAD_DESC **Thedi;

	if (ProcID >= PMT_LEN)
		return ERROR_WRONG_PROCID;
	CurProc = CurPmd;
	cli();
	DstProc = pmt[ProcID];
	if (DstProc == NULL || DstProc == CurProc || (DstProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_PROCID;
	}
	if (!(DstProc->attr & PROC_ATTR_APPS) && (CurProc->attr & PROC_ATTR_APPS))	/*Ӧ�ý�����Ȩɱ����������*/
	{
		sti();
		return ERROR_NOT_DRIVER;
	}
	DstProc->attr |= PROC_ATTR_DEL;	/*���Ϊ���ڱ�ɾ��*/
	for (Thedi = DstProc->tmt; Thedi < DstProc->EndTmd; Thedi++)
	{
		THREAD_DESC *DstThed;

		DstThed = *Thedi;
		if (DstThed && !(DstThed->attr & THED_ATTR_DEL))
		{
			DstThed->attr |= THED_ATTR_KILLED;	/*���Ϊ��ɱ��*/
			if (DstThed->attr & THED_ATTR_SLEEP)	/*�߳�����,���Ȼ����߳�*/
				wakeup(DstThed);
		}
	}
	sti();
	return NO_ERROR;
}

/*�����û���ַ��*/
BLK_DESC *AllocUBlk(PROCESS_DESC *proc, DWORD siz)
{
	BLK_DESC *blk;

	if (proc->FstUBlk >= &proc->ublkt[UBLKT_LEN])
		return NULL;
	blk = proc->FstUBlk;
	if ((blk->addr = alloc(proc->ufdmt, siz)) == NULL)
		return NULL;
	blk->siz = siz;
	do
		proc->FstUBlk++;
	while (proc->FstUBlk < &proc->ublkt[UBLKT_LEN] && proc->FstUBlk->siz);
	if (proc->EndUBlk < proc->FstUBlk)
		proc->EndUBlk = proc->FstUBlk;
	return blk;
}

/*�����û���ַ��*/
BLK_DESC *FindUBlk(PROCESS_DESC *proc, void *addr)
{
	BLK_DESC *blk;

	for (blk = proc->ublkt; blk < proc->EndUBlk; blk++)
		if (blk->addr == addr)
			return blk;
	return NULL;
}

/*�����û���ַ��*/
void FreeUBlk(PROCESS_DESC *proc, BLK_DESC *blk)
{
	free(proc->ufdmt, blk->addr, blk->siz);
	blk->siz = 0;
	if (proc->FstUBlk > blk)
		proc->FstUBlk = blk;
	while (proc->EndUBlk > proc->ublkt && (proc->EndUBlk - 1)->siz == 0)
		proc->EndUBlk--;
}

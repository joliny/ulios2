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
static long AllocPid(PROCESS_DESC *proc)
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
static void FreePid(PROCESS_DESC **pmd)
{
	*pmd = NULL;
	if (FstPmd > pmd)
		FstPmd = pmd;
	while (EndPmd > pmt && *(EndPmd - 1) == NULL)
		EndPmd--;
	PmdCou--;
}

/*������߳�ID*/
static long AllocTid(PROCESS_DESC *proc, THREAD_DESC *thed)
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
static void FreeTid(PROCESS_DESC *proc, THREAD_DESC **tmd)
{
	*tmd = NULL;
	if (proc->FstTmd > tmd)
		proc->FstTmd = tmd;
	while (proc->EndTmd > proc->tmt && *(proc->EndTmd - 1) == NULL)
		proc->EndTmd--;
	proc->TmdCou--;
}

/*����ʱ�̲߳�����ʱ��������*/
static void InsertSleepList(THREAD_DESC *thed)
{
	THREAD_DESC *PreThed, *NxtThed;

	for (PreThed = NxtThed = SleepList; NxtThed; NxtThed = (PreThed = NxtThed)->nxt)
		if (thed->WakeupClock < NxtThed->WakeupClock)	/*�����������*/
			break;
	if (PreThed != SleepList)
	{
		thed->pre = PreThed;
		PreThed->nxt = thed;
	}
	else
	{
		thed->pre = NULL;
		SleepList = thed;
	}
	thed->nxt = NxtThed;
	if (NxtThed)
		NxtThed->pre = thed;
}

/*����ʱ�̴߳���ʱ��������ͷ��ɾ��*/
static void DeleteSleepList(THREAD_DESC *thed)
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

/*��ʱcs����*/
void SleepCs(DWORD cs)
{
	if (cs)
	{
		THREAD_DESC *CurThed;

		CurThed = CurPmd->CurTmd;
		CurThed->WakeupClock = clock + cs;
		cli();
		sleep(TRUE);
		sti();
		if (CurThed->attr & THED_ATTR_DEL)	/*��ɱ�����߳������˳�*/
			DeleteThed();
	}
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

/*�����߳�,����ǰ���ж�,�̱߳�����Ч������,���������ں�����*/
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

/*�����߳�,����ǰ���ж�,�̱߳�����Ч�Ҿ���,�����������ں�����*/
void sleep(BOOL isWaitTime)
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
	if (isWaitTime)	/*����ʱ������*/
	{
		CurThed->attr |= THED_ATTR_WAITTIME;
		InsertSleepList(CurThed);
	}
	SwitchTS();
}

/*�����߳�*/
long CreateThed(const DWORD *args, THREAD_ID *ptid)
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
	NewThed->tss.cr3 = CurThed->tss.cr3;
	NewThed->tss.eip = ThedStart;
	NewThed->tss.esp = (DWORD)NewThed + sizeof(THREAD_DESC);
	NewThed->tss.cs = KCODE_SEL;
	NewThed->tss.gs = NewThed->tss.fs = NewThed->tss.ds = NewThed->tss.ss = NewThed->tss.es = KDATA_SEL;
	NewThed->tss.io = sizeof(TSS);
	memcpy32(NewThed->kstk, args, 2);	/*���Ʋ���*/
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
	if (ptid)
		*ptid = NewThed->id;
	sti();
	return NO_ERROR;
}

/*ɾ���߳�,������ǰ�߳�*/
void DeleteThed()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *CurExec;
	THREAD_DESC **Thedi;
	WORD CurTid;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	CurExec = CurProc->exec;
	CurThed->attr |= THED_ATTR_DEL;	/*���Ϊ���ڱ�ɾ��*/
	UnregAllIrq();
	UnregAllKnlPort();
	FreeAllMsg();
	LockFreeUFData(CurProc, CurThed->ustk, CurThed->UstkSiz);
	clilock(CurProc->Page_l || CurExec->Page_l || Kmalloc_l || AllocPage_l);
	CurTid = CurThed->id.ThedID;
	kfree(CurThed, sizeof(THREAD_DESC));
	FreeTid(CurProc, &CurProc->tmt[CurTid]);
	for (Thedi = CurProc->tmt; Thedi < CurProc->EndTmd; Thedi++)	/*�������̵߳ĸ��߳�ID*/
		if (*Thedi && (*Thedi)->par == CurTid)
			(*Thedi)->par = CurThed->par;
	if (CurThed->nxt == CurThed)	/*ֻʣ��ǰһ�������߳�,��Ҫ��������*/
	{
		if (CurProc->TmdCou == 0)	/*ֻʣ��ǰһ������,��Ҫɾ������*/
		{
			PROCESS_DESC **Proci;
			WORD CurPid;

			kfree(CurExec, sizeof(EXEC_DESC));
			FreeExid(&exmt[CurExec->id]);
			CurPid = CurThed->id.ProcID;
			pddt[CurPid] = 0;
			FreePage(CurThed->tss.cr3);
			kfree(CurProc, sizeof(PROCESS_DESC));
			FreePid(&pmt[CurPid]);
			for (Proci = pmt; Proci < EndPmd; Proci++)	/*�����ӽ��̵ĸ�����ID*/
				if (*Proci && (*Proci)->par == CurPid)
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

/*ɱ���߳�,����ɱ����ǰ�߳�*/
long KillThed(WORD ThedID)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *DstThed;

	CurProc = CurPmd;
	clilock(CurProc->Ufdmt_l || Kmalloc_l);
	DstThed = CurProc->tmt[ThedID];
	if (DstThed == NULL)
	{
		sti();
		return ERROR_WRONG_THEDID;
	}
	if (!(DstThed->attr & (THED_ATTR_SLEEP | THED_ATTR_APPS)))	/*ϵͳ����̬�ľ����̲߳���ֱ��ɾ��*/
	{
		DstThed->attr |= THED_ATTR_DEL;
		sti();
		return NO_ERROR;
	}
	sti();
	return NO_ERROR;
}

/*��������*/
long CreateProc(DWORD attr, const BYTE *args, THREAD_ID *ptid)
{
	PROCESS_DESC *CurProc, *NewProc;
	THREAD_DESC *NewThed;
	DWORD NewPdt, pdti;

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
	CurProc = CurPmd;
	memset32(NewProc, 0, sizeof(PROCESS_DESC) / sizeof(DWORD));
	NewProc->par = CurProc->CurTmd->id.ProcID;
	NewProc->MemSiz = PAGE_SIZE;	/*ҳĿ¼��ռ��*/
	NewProc->tmt[0] = NewThed;
	NewProc->EndTmd = NewProc->FstTmd = &NewProc->tmt[1];
	NewProc->CurTmd = NewThed;
	NewProc->TmdCou = 1;
	memset32(NewThed, 0, sizeof(THREAD_DESC) / sizeof(DWORD));
	NewThed->pre = NewThed;
	NewThed->nxt = NewThed;
	NewThed->par = INVWID;
	NewThed->tss.cr3 = NewPdt;
	NewThed->tss.eip = ProcStart;
	NewThed->tss.esp = (DWORD)NewThed + sizeof(THREAD_DESC);
	NewThed->tss.cs = KCODE_SEL;
	NewThed->tss.gs = NewThed->tss.fs = NewThed->tss.ds = NewThed->tss.ss = NewThed->tss.es = KDATA_SEL;
	NewThed->tss.io = sizeof(TSS);
	NewThed->kstk[0] = attr;	/*���Ʋ���*/
	if (attr & EXEC_ARGS_BASESRV)
		memcpy32(&NewThed->kstk[1], args, sizeof(PHYBLK_DESC) / sizeof(DWORD));
	else
		strncpy8(&NewThed->kstk[1], args, EXEC_ARGS_SIZ);
	cli();
	if (AllocPid(NewProc) != NO_ERROR)
	{
		sti();
		LockFreePage(NewPdt);
		LockKfree(NewThed, sizeof(THREAD_DESC));
		LockKfree(NewProc, sizeof(PROCESS_DESC));
		return ERROR_HAVENO_PROCID;
	}
	CurProc->MemSiz -= PAGE_SIZE;	/*��ȥAllocPage�ж���ڵ�ǰ���̵�һҳ�ڴ�*/
	pdti = NewThed->id.ProcID;
	pddt[pdti] = PAGE_ATTR_P | PAGE_ATTR_W | PAGE_ATTR_A | PAGE_ATTR_D | PAGE_ATTR_G | NewPdt;	/*ӳ���½��̵�ҳĿ¼��*/
	pdti <<= 10;
	memset32(&pdt[pdti], 0, 0x400);
	memcpy32(&pdt[pdti], kpdt, 4);	/*����ҳĿ¼��*/
	pdt[pdti | PT_ID] = PAGE_ATTR_P | NewPdt;	/*ӳ���½��̵�ҳ��*/
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
	if (ptid)
		*ptid = NewThed->id;
	sti();
	return NO_ERROR;
}

/*ɾ������,������ǰ����*/
void DeleteProc()
{
	WORD CurTid, ThedID;

	CurPmd->attr |= PROC_ATTR_DEL;
	CurTid = CurPmd->CurTmd->id.ThedID;
	for (ThedID = 0; ThedID < TMT_LEN; ThedID++)
		if (ThedID != CurTid)
			KillThed(ThedID);
	DeleteThed();
}

/*ɱ������,����ɱ����ǰ����*/
long KillProc(WORD ProcID)
{
	return NO_ERROR;
}

/*�����û���ַ��*/
void *LockAllocUBlk(PROCESS_DESC *proc, DWORD siz, DWORD ptid)
{
	void *addr;

	lock(&proc->Ufdmt_l);
	if (proc->FstUBlk >= &proc->ublkt[UBLKT_LEN])
	{
		ulock(&proc->Ufdmt_l);
		return NULL;
	}
	if ((addr = alloc(proc->ufdmt, siz)) == NULL)
	{
		ulock(&proc->Ufdmt_l);
		return NULL;
	}
	proc->FstUBlk->addr = addr;
	proc->FstUBlk->siz = siz;
	proc->FstUBlk->ptid = ptid;
	do
		proc->FstUBlk++;
	while (proc->FstUBlk < &proc->ublkt[UBLKT_LEN] && proc->FstUBlk->siz);
	if (proc->EndUBlk < proc->FstUBlk)
		proc->EndUBlk = proc->FstUBlk;
	ulock(&proc->Ufdmt_l);
	return addr;
}

/*�����û���ַ��*/
long LockFreeUBlk(PROCESS_DESC *proc, void *addr)
{
	BLK_DESC *CurBlk;

	*((DWORD*)&addr) &= 0xFFFFF000;
	lock(&proc->Ufdmt_l);
	for (CurBlk = proc->ublkt; CurBlk < proc->EndUBlk; CurBlk++)
		if (CurBlk->addr == addr)
		{
			DWORD siz, ptid;

			siz = CurBlk->siz;
			ptid = CurBlk->ptid;
			free(proc->ufdmt, addr, siz);
			CurBlk->siz = 0;
			if (proc->FstUBlk > CurBlk)
				proc->FstUBlk = CurBlk;
			while (proc->EndUBlk > proc->ublkt && (proc->EndUBlk - 1)->siz == 0)
				proc->EndUBlk--;
			ulock(&proc->Ufdmt_l);
			if ((ptid & BLK_PTID_SPECIAL) != BLK_PTID_SPECIAL)	/*��������PTID*/
				UnmapProcAddr(addr, siz, *((THREAD_ID*)&ptid));
			else
				UnmapAddr(addr, siz, ptid);
			return NO_ERROR;
		}
	ulock(&proc->Ufdmt_l);
	return ERROR_HAVENO_LINEADDR;
}

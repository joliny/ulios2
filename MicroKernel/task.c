/*	task.c for ulios
	���ߣ�����
	���ܣ���������������̺��߳�
	����޸����ڣ�2009-06-25
*/

#include "knldef.h"

/*�����л�*/
void SwitchTS()
{
	TSS *CurTss;

	if (CurPmd)	/*�о�������*/
		CurTss = &CurPmd->CurTmd->tss;
	else	/*û�о�������,�л����ں�*/
		CurTss = &KnlTss;
	SetSegDesc(&gdt[TSS_SEL >> 3], (DWORD)CurTss, sizeof(TSS) - 1, DESC_ATTR_P | SYSSEG_ATTR_T_TSS);	/*���������TSS������*/
	__asm__("ljmp %0, $0":: "i"(TSS_SEL));	/*����תִ���л�*/
}

/*����ս���ID*/
static inline long AllocPid(PROCESS_DESC *proc)
{
	if (FstPmd >= &pmt[PMT_LEN])
		return KERR_PROC_NOT_ENOUGH;	/*û�пս���ID*/
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
		return KERR_THED_NOT_ENOUGH;	/*û�п��߳�ID*/
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
	thed->attr &= (~(THED_ATTR_SLEEP | THED_ATTR_WAITMSG | THED_ATTR_WAITTIME));	/*�������״̬,�����߳�*/
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
void sleep(BOOL isWaitMsg, DWORD cs)
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
	if (isWaitMsg)	/*�����ȴ���Ϣ*/
		CurThed->attr |= THED_ATTR_WAITMSG;
	if (cs != INVALID)	/*����һ��ʱ��*/
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
		ThedExit(KERR_THED_KILLED);
	}
}

/*�����߳�*/
long CreateThed(DWORD attr, DWORD proc, DWORD args, THREAD_ID *ptid)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed, *NewThed, *NxtThed;

	CurProc = CurPmd;
	if ((NewThed = (THREAD_DESC*)LockKmalloc(sizeof(THREAD_DESC))) == NULL)
		return KERR_OUT_OF_KNLMEM;
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
	NewThed->kstk[0] = attr;	/*���Ʋ���*/
	NewThed->kstk[1] = proc;
	NewThed->kstk[2] = args;
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
		return KERR_THED_NOT_ENOUGH;
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
		return KERR_INVALID_THEDID;
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	cli();
	DstThed = CurProc->tmt[ThedID];
	if (DstThed == NULL || (DstThed->attr & THED_ATTR_DEL))
	{
		sti();
		return KERR_THED_NOT_EXIST;
	}
	if (DstThed == CurThed)
	{
		sti();
		return KERR_THED_KILL_SELF;
	}
	DstThed->attr |= THED_ATTR_KILLED;	/*���Ϊ��ɱ��*/
	if (DstThed->attr & THED_ATTR_SLEEP)	/*�߳�����,���Ȼ����߳�*/
		wakeup(DstThed);
	sti();
	return NO_ERROR;
}

/*��������*/
long CreateProc(DWORD attr, DWORD exec, DWORD args, THREAD_ID *ptid)
{
	PROCESS_DESC *CurProc, *NewProc;
	THREAD_DESC *NewThed;
	DWORD NewPdt, pdti;

	CurProc = CurPmd;
	if (CurProc && (CurProc->attr & PROC_ATTR_APPS) && (attr & EXEC_ARGV_DRIVER))	/*Ӧ�ý�����Ȩ������������*/
		return KERR_NO_DRIVER_PRIVILEGE;
	if ((NewProc = (PROCESS_DESC*)LockKmalloc(sizeof(PROCESS_DESC))) == NULL)
		return KERR_OUT_OF_KNLMEM;
	if ((NewThed = (THREAD_DESC*)LockKmalloc(sizeof(THREAD_DESC))) == NULL)
	{
		LockKfree(NewProc, sizeof(PROCESS_DESC));
		return KERR_OUT_OF_KNLMEM;
	}
	if ((NewPdt = LockAllocPage()) == 0)
	{
		LockKfree(NewThed, sizeof(THREAD_DESC));
		LockKfree(NewProc, sizeof(PROCESS_DESC));
		return KERR_OUT_OF_PHYMEM;
	}
	memset32(NewProc, 0, sizeof(PROCESS_DESC) / sizeof(DWORD));
	NewProc->par = CurProc ? CurProc->CurTmd->id.ProcID : INVALID;
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
	cli();
	if (AllocPid(NewProc) != NO_ERROR)
	{
		sti();
		LockFreePage(NewPdt);
		LockKfree(NewThed, sizeof(THREAD_DESC));
		LockKfree(NewProc, sizeof(PROCESS_DESC));
		return KERR_PROC_NOT_ENOUGH;
	}
	sti();
	pdti = NewThed->id.ProcID;
	NewThed->kstk[0] = attr;	/*���Ʋ���*/
	if (attr & EXEC_ARGV_BASESRV)
	{
		NewThed->kstk[1] = exec;
		NewThed->kstk[2] = args;
	}
	else
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];
		MESSAGE_DESC *msg;

		data[MSG_API_ID] = FS_API_GETEXEC;
		data[3] = pdti;
		ptid = kpt[FS_KPORT];
		if ((data[MSG_API_ID] = MapProcAddr((void*)exec, PROC_EXEC_SIZE, &ptid, FALSE, TRUE, data, FS_OUT_TIME)) != NO_ERROR || data[MSG_RES_ID] != NO_ERROR)	/*���ļ�ϵͳ���Ϳ�ִ��������*/
		{
			cli();
			FreePid(&pmt[pdti]);
			sti();
			LockFreePage(NewPdt);
			LockKfree(NewThed, sizeof(THREAD_DESC));
			LockKfree(NewProc, sizeof(PROCESS_DESC));
			return data[MSG_API_ID] != NO_ERROR ? data[MSG_API_ID] : data[MSG_RES_ID];
		}
		if ((data[MSG_API_ID] = RecvProcMsg(&msg, kpt[FS_KPORT], FS_OUT_TIME)) != NO_ERROR)
		{
			cli();
			FreePid(&pmt[pdti]);
			sti();
			LockFreePage(NewPdt);
			LockKfree(NewThed, sizeof(THREAD_DESC));
			LockKfree(NewProc, sizeof(PROCESS_DESC));
			return data[MSG_API_ID];
		}
		memcpy32(&NewThed->kstk[1], msg->data, 8);
		FreeMsg(msg);
		if (args)
			strcpy((BYTE*)&NewThed->kstk[9], (const BYTE*)args);
	}
	cli();
	NewPdt |= PAGE_ATTR_P;
	pddt[pdti] = NewPdt;	/*ӳ���½��̵�ҳĿ¼��*/
	pdti <<= 10;
	memset32(&pdt[pdti], 0, 0x400);
	memcpy32(&pdt[pdti], kpdt, 3);	/*����ҳĿ¼��*/
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
void DeleteProc(DWORD ExitCode)
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
	ThedExit(ExitCode);
}

/*ɱ������*/
long KillProc(WORD ProcID)
{
	PROCESS_DESC *CurProc;
	PROCESS_DESC *DstProc;
	THREAD_DESC **Thedi;

	if (ProcID >= PMT_LEN)
		return KERR_INVALID_PROCID;
	CurProc = CurPmd;
	cli();
	DstProc = pmt[ProcID];
	if (DstProc == NULL || (DstProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return KERR_PROC_NOT_EXIST;
	}
	if (DstProc == CurProc)
	{
		sti();
		return KERR_PROC_KILL_SELF;
	}
	if (!(DstProc->attr & PROC_ATTR_APPS) && (CurProc->attr & PROC_ATTR_APPS))	/*Ӧ�ý�����Ȩɱ����������*/
	{
		sti();
		return KERR_NO_DRIVER_PRIVILEGE;
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

	blk = proc->FstUBlk;
	if (blk == NULL)
		return NULL;
	if (proc->EndUBlk < blk + 1)
		proc->EndUBlk = blk + 1;
	proc->FstUBlk = (BLK_DESC*)blk->addr;
	if ((blk->addr = alloc(proc->ufdmt, siz)) == NULL)
		return NULL;
	blk->siz = siz;
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
	blk->addr = (BLK_DESC*)proc->FstUBlk;	/*�ͷű���*/
	proc->FstUBlk = blk;
	while (proc->EndUBlk > proc->ublkt && (proc->EndUBlk - 1)->siz == 0)
		proc->EndUBlk--;
}

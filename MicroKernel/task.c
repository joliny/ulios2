/*	task.c for ulios
	作者：孙亮
	功能：任务管理，包括进程和线程
	最后修改日期：2009-06-25
*/

#include "knldef.h"

/*初始化进程管理表*/
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

/*初始化内核进程*/
void InitKnlProc()
{
	memset32(&KnlTss, 0, sizeof(TSS) / sizeof(DWORD));
	KnlTss.cr3 = (DWORD)kpdt;
	KnlTss.io = sizeof(TSS);
	SetSegDesc(&gdt[UCODE_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_E | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);	/*初始化用户GDT项*/
	SetSegDesc(&gdt[UDATA_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_D_W | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);
	SetSegDesc(&gdt[TSS_SEL >> 3], (DWORD)&KnlTss, sizeof(TSS) - 1, DESC_ATTR_P | SYSSEG_ATTR_T_TSS);
	__asm__("ltr %%ax":: "a"(TSS_SEL));
}

/*任务切换*/
static void SwitchTS()
{
	TSS *CurTss;

	if (CurPmd)	/*有就绪进程*/
		CurTss = &CurPmd->CurTmd->tss;
	else	/*没有就绪进程,切换到内核*/
		CurTss = &KnlTss;
	SetSegDesc(&gdt[TSS_SEL >> 3], (DWORD)CurTss, sizeof(TSS) - 1, DESC_ATTR_P | SYSSEG_ATTR_T_TSS);	/*设置任务的TSS描述符*/
	__asm__("ljmp %0, $0":: "i"(TSS_SEL));
}

/*分配空进程ID*/
static inline long AllocPid(PROCESS_DESC *proc)
{
	if (FstPmd >= &pmt[PMT_LEN])
		return ERROR_HAVENO_PROCID;	// 没有空进程ID
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

/*释放空进程ID*/
static inline void FreePid(PROCESS_DESC **pmd)
{
	*pmd = NULL;
	if (FstPmd > pmd)
		FstPmd = pmd;
	while (EndPmd > pmt && *(EndPmd - 1) == NULL)
		EndPmd--;
	PmdCou--;
}

/*分配空线程ID*/
static inline long AllocTid(PROCESS_DESC *proc, THREAD_DESC *thed)
{
	if (proc->FstTmd >= &proc->tmt[TMT_LEN])
		return ERROR_HAVENO_THEDID;	// 没有空线程ID
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

/*释放空线程ID*/
static inline void FreeTid(PROCESS_DESC *proc, THREAD_DESC **tmd)
{
	*tmd = NULL;
	if (proc->FstTmd > tmd)
		proc->FstTmd = tmd;
	while (proc->EndTmd > proc->tmt && *(proc->EndTmd - 1) == NULL)
		proc->EndTmd--;
	proc->TmdCou--;
}

/*将延时线程插入延时阻塞链表*/
static inline void InsertSleepList(THREAD_DESC *thed)
{
	THREAD_DESC *PreThed, *NxtThed;

	for (PreThed = NULL, NxtThed = SleepList; NxtThed; NxtThed = (PreThed = NxtThed)->nxt)
		if (thed->WakeupClock < NxtThed->WakeupClock)	/*进行排序插入*/
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

/*将延时线程从延时阻塞链表头中删除*/
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

/*线程调度,调用前关中断*/
void schedul()
{
	if (CurPmd)	/*有就绪进程*/
	{
		PROCESS_DESC *NxtProc;
		THREAD_DESC *NxtThed;

		NxtProc = CurPmd->nxt;	/*选择下一个进程*/
		NxtThed = NxtProc->CurTmd->nxt;	/*选择下一个线程*/
		if (NxtProc != CurPmd || NxtThed != NxtProc->CurTmd)	/*如果当前任务切换到当前任务则不必切换*/
		{
			CurPmd = NxtProc;
			NxtProc->CurTmd = NxtThed;
			SwitchTS();
		}
	}
}

/*唤醒线程*/
void wakeup(THREAD_DESC *thed)
{
	PROCESS_DESC *CurProc, *NewProc;
	THREAD_DESC *CurThed;

	CurProc = CurPmd;
	NewProc = pmt[thed->id.ProcID];
	CurThed = NewProc->CurTmd;
	if (thed->attr & THED_ATTR_WAITTIME)	/*从延时链表中删除*/
		DeleteSleepList(thed);
	thed->attr &= (~(THED_ATTR_SLEEP | THED_ATTR_WAITTIME));	/*清除阻塞状态,唤醒线程*/
	if (CurThed)	/*有就绪线程,插入环形链表*/
	{
		THREAD_DESC *NxtThed;

		NxtThed = CurThed->nxt;
		thed->pre = CurThed;
		thed->nxt = NxtThed;
		CurThed->nxt = thed;
		NxtThed->pre = thed;
	}
	else	/*进程中没有就绪线程,首先唤醒进程*/
	{
		thed->pre = thed;	/*线程放入空环形链表*/
		thed->nxt = thed;
		if (CurProc)	/*有就绪进程,插入环形链表*/
		{
			PROCESS_DESC *NxtProc;

			NxtProc = CurProc->nxt;
			NewProc->pre = CurProc;
			NewProc->nxt = NxtProc;
			CurProc->nxt = NewProc;
			NxtProc->pre = NewProc;
		}
		else	/*没有就绪进程,进程放入空环形链表*/
		{
			NewProc->pre = NewProc;
			NewProc->nxt = NewProc;
		}
	}
	CurPmd = NewProc;
	NewProc->CurTmd = thed;
	SwitchTS();
}

/*阻塞线程*/
void sleep(DWORD cs)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (CurThed->nxt == CurThed)	/*只剩当前一个就绪线程,需要阻塞进程*/
	{
		if (CurProc->nxt == CurProc)	/*只剩当前一个就绪进程,切换到内核任务*/
			CurPmd = NULL;
		else	/*至少一个就绪进程,从环形链表中删除*/
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
	else	/*至少一个就绪线程,从环形链表中删除*/
	{
		THREAD_DESC *PreThed, *NxtThed;

		PreThed = CurThed->pre;
		NxtThed = CurThed->nxt;
		PreThed->nxt = NxtThed;
		NxtThed->pre = PreThed;
		CurProc->CurTmd = NxtThed;
	}
	CurThed->attr |= THED_ATTR_SLEEP;	/*阻塞线程*/
	if (cs != INVALID)	/*因延时而阻塞*/
	{
		CurThed->WakeupClock = clock + cs;
		CurThed->attr |= THED_ATTR_WAITTIME;
		InsertSleepList(CurThed);
	}
	SwitchTS();
	if (CurThed->attr & THED_ATTR_KILLED)	/*线程被杀死*/
	{
		CurThed->attr &= (~THED_ATTR_KILLED);
		sti();
		ThedExit();
	}
}

/*创建线程*/
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
	memcpy32(NewThed->kstk, argv, 3);	/*复制参数*/
	cli();
	if (CurProc->attr & PROC_ATTR_DEL)	/*正在被删除的进程不创建线程*/
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
	NxtThed = CurThed->nxt;	/*插入当前线程后*/
	NewThed->pre = CurThed;
	NewThed->nxt = NxtThed;
	CurThed->nxt = NewThed;
	NxtThed->pre = NewThed;
	sti();
	*ptid = NewThed->id;
	return NO_ERROR;
}

/*删除线程*/
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
	for (Thedi = CurProc->tmt; Thedi < CurProc->EndTmd; Thedi++)	/*调整子线程的父线程ID*/
		if (*Thedi && (*Thedi)->par == ptid)
			(*Thedi)->par = CurThed->par;
	if (CurThed->nxt == CurThed)	/*只剩当前一个就绪线程,需要阻塞进程*/
	{
		if (CurProc->TmdCou == 0)	/*只剩当前一个线程,清除进程资源*/
		{
			PROCESS_DESC **Proci;

			FreePage(pt[(PT_ID << 10) | PT_ID]);
			ptid = CurThed->id.ProcID;
			kfree(CurProc, sizeof(PROCESS_DESC));
			FreePid(&pmt[ptid]);
			for (Proci = pmt; Proci < EndPmd; Proci++)	/*调整子进程的父进程ID*/
				if (*Proci && (*Proci)->par == ptid)
					(*Proci)->par = CurProc->par;
		}
		if (CurProc->nxt == CurProc)	/*只剩当前一个就绪进程,切换到内核任务*/
			CurPmd = NULL;
		else	/*至少一个就绪进程,从环形链表中删除*/
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
	else	/*至少一个就绪线程,从环形链表中删除*/
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

/*杀死线程*/
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
	DstThed->attr |= THED_ATTR_KILLED;	/*标记为被杀死*/
	if (DstThed->attr & THED_ATTR_SLEEP)	/*线程阻塞,首先唤醒线程*/
		wakeup(DstThed);
	sti();
	return NO_ERROR;
}

/*创建进程*/
long CreateProc(const DWORD *argv, THREAD_ID *ptid)
{
	PROCESS_DESC *CurProc, *NewProc;
	THREAD_DESC *NewThed;
	DWORD NewPdt, pdti;

	CurProc = CurPmd;
	if (CurProc && (CurProc->attr & PROC_ATTR_APPS) && (argv[0] & EXEC_ARGV_DRIVER))	/*应用进程无权启动驱动进程*/
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
	NewProc->MemSiz = PAGE_SIZE;	/*页目录表占用*/
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
		memcpy32(NewThed->kstk, argv, 3);	/*复制参数*/
	else
	{
		memcpy32(NewThed->kstk, argv, 2);	/*复制参数*/
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
	pddt[pdti] = NewPdt;	/*映射新进程的页目录表*/
	pdti <<= 10;
	memset32(&pdt[pdti], 0, 0x400);
	memcpy32(&pdt[pdti], kpdt, 4);	/*复制页目录表*/
	pdt[pdti | PT_ID] = NewPdt;	/*映射新进程的页表*/
	if (CurProc)	/*有就绪进程,插入当前进程后*/
	{
		PROCESS_DESC *NxtProc;

		NxtProc = CurProc->nxt;
		NewProc->pre = CurProc;
		NewProc->nxt = NxtProc;
		CurProc->nxt = NewProc;
		NxtProc->pre = NewProc;
	}
	else	/*没有就绪进程,进程放入空环形链表*/
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

/*删除进程*/
void DeleteProc()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC **Thedi;
	THREAD_DESC *CurThed;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	cli();
	CurProc->attr |= PROC_ATTR_DEL;	/*标记为正在被删除*/
	for (Thedi = CurProc->tmt; Thedi < CurProc->EndTmd; Thedi++)
	{
		THREAD_DESC *DstThed;

		DstThed = *Thedi;
		if (DstThed && DstThed != CurThed && !(DstThed->attr & THED_ATTR_DEL))
		{
			DstThed->attr |= THED_ATTR_KILLED;	/*标记为被杀死*/
			if (DstThed->attr & THED_ATTR_SLEEP)	/*线程阻塞,首先唤醒线程*/
				wakeup(DstThed);
		}
	}
	sti();
	ThedExit();
}

/*杀死进程*/
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
	if (!(DstProc->attr & PROC_ATTR_APPS) && (CurProc->attr & PROC_ATTR_APPS))	/*应用进程无权杀死驱动进程*/
	{
		sti();
		return ERROR_NOT_DRIVER;
	}
	DstProc->attr |= PROC_ATTR_DEL;	/*标记为正在被删除*/
	for (Thedi = DstProc->tmt; Thedi < DstProc->EndTmd; Thedi++)
	{
		THREAD_DESC *DstThed;

		DstThed = *Thedi;
		if (DstThed && !(DstThed->attr & THED_ATTR_DEL))
		{
			DstThed->attr |= THED_ATTR_KILLED;	/*标记为被杀死*/
			if (DstThed->attr & THED_ATTR_SLEEP)	/*线程阻塞,首先唤醒线程*/
				wakeup(DstThed);
		}
	}
	sti();
	return NO_ERROR;
}

/*分配用户地址块*/
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

/*搜索用户地址块*/
BLK_DESC *FindUBlk(PROCESS_DESC *proc, void *addr)
{
	BLK_DESC *blk;

	for (blk = proc->ublkt; blk < proc->EndUBlk; blk++)
		if (blk->addr == addr)
			return blk;
	return NULL;
}

/*回收用户地址块*/
void FreeUBlk(PROCESS_DESC *proc, BLK_DESC *blk)
{
	free(proc->ufdmt, blk->addr, blk->siz);
	blk->siz = 0;
	if (proc->FstUBlk > blk)
		proc->FstUBlk = blk;
	while (proc->EndUBlk > proc->ublkt && (proc->EndUBlk - 1)->siz == 0)
		proc->EndUBlk--;
}

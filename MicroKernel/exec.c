/*	exec.c for ulios
	作者：孙亮
	功能：可执行体管理，进程线程初始化
	最后修改日期：2009-09-01
*/

#include "knldef.h"

/*初始化可执行体管理表*/
void InitEXMT()
{
	memset32(exmt, 0, EXMT_LEN * sizeof(EXEC_DESC*) / sizeof(DWORD));
	EndExmd = FstExmd = exmt;
}

/*分配空可执行体ID*/
long AllocExid(EXEC_DESC *exec)
{
	cli();
	if (FstExmd >= &exmt[EXMT_LEN])
	{
		sti();
		return ERROR_HAVENO_EXECID;	// 没有空可执行体ID
	}
	exec->id = FstExmd - exmt;
	exec->cou = 1;
	*FstExmd = exec;
	do
		FstExmd++;
	while (FstExmd < &exmt[EXMT_LEN] && *FstExmd);
	if (EndExmd < FstExmd)
		EndExmd = FstExmd;
	sti();
	return NO_ERROR;
}

/*释放空可执行体ID*/
void FreeExid(EXEC_DESC **exmd)
{
	cli();
	*exmd = NULL;
	if (FstExmd > exmd)
		FstExmd = exmd;
	while (EndExmd > exmt && *(EndExmd - 1) == NULL)
		EndExmd--;
	sti();
}

/*线程起点*/
void ThedStart()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	DWORD UstkSiz;

	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	UstkSiz = CurThed->kstk[1];
	UstkSiz = (UstkSiz != 0) ? (UstkSiz + 0x00000FFF) & 0xFFFFF000 : THEDSTK_SIZ;
	if ((CurThed->ustk = LockAllocUFData(CurProc, UstkSiz)))
		DeleteThed();
	CurThed->UstkSiz = UstkSiz;
	CurThed->attr |= THED_ATTR_APPS;
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
		"iret"				/*进入用户态*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(CurThed->kstk[0])
	);
}

/*进程起点*/
void ProcStart()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *NewExec;

	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (CurThed->kstk[0] & EXEC_ARGS_BASESRV)	/*启动基础服务进程*/
	{
		PHYBLK_DESC *CurSrv;
		DWORD NewPdt, pdti, i, end, PhyAddr;

		if ((NewExec = (EXEC_DESC*)LockKmalloc(sizeof(EXEC_DESC))) == NULL)
			DeleteThed();
		if ((NewPdt = LockAllocPage()) == 0)
		{
			LockKfree(NewExec, sizeof(EXEC_DESC));
			DeleteThed();
		}
		memset32(NewExec, 0, sizeof(EXEC_DESC) / sizeof(DWORD));
		CurSrv = (PHYBLK_DESC*)&CurThed->kstk[1];
		NewExec->entry = NewExec->CodeOff = BASESRV_OFF;
		NewExec->BssEnd = NewExec->DataEnd = NewExec->DataOff = NewExec->CodeEnd = BASESRV_OFF + CurSrv->siz;
		pdti = CurThed->id.ProcID;
		pddt0[pdti] = PAGE_ATTR_P | NewPdt;	/*映射页目录表副本*/
		pdti <<= 10;
		pdt[pdti | PT0_ID] = PAGE_ATTR_P | NewPdt;	/*映射页表副本*/
		memset32(&pdt0[pdti], 0, 0x400);
		for (i = pdti | ((DWORD)BASESRV_OFF >> 22), end = pdti | (((DWORD)NewExec->CodeEnd + 0x003FFFFF) >> 22); i < end; i++)
		{
			DWORD PtAddr;	/*页表的物理地址*/

			if ((PtAddr = LockAllocPage()) == 0)
			{
				LockKfree(NewExec, sizeof(EXEC_DESC));
				DeleteThed();
			}
			pdt0[i] = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr;	/*修改页目录表,设为只读*/
			memset32(&pt0[(i << 10) & 0x000FFC00], 0, 0x400);	/*清空页表*/
		}
		PhyAddr = PAGE_ATTR_P | PAGE_ATTR_U | (DWORD)CurSrv->addr;
		for (i = ((DWORD)BASESRV_OFF >> 12), end = (((DWORD)NewExec->CodeEnd + 0x00000FFF) >> 12); i < end; i++)	/*修改页表,映射地址*/
		{
			pt0[i] = PhyAddr;
			PhyAddr += PAGE_SIZE;
		}
		if (AllocExid(NewExec) != NO_ERROR)
		{
			LockFreePage(NewPdt);
			LockKfree(NewExec, sizeof(EXEC_DESC));
			DeleteThed();
		}
	}
	else	/*启动可执行文件进程*/
	{
		NewExec = NULL;
	}
	if (!(CurThed->kstk[0] & EXEC_ARGS_DRIVER))	/*设置用户应用进程*/
		CurProc->attr |= PROC_ATTR_APPS;
	CurProc->exec = NewExec;
	CurProc->EndUBlk = CurProc->FstUBlk = CurProc->ublkt;
	InitFbt(CurProc->ufdmt, UFDMT_LEN, UFDATA_OFF, UFDATA_SIZ);	/*初始化用户自由数据区*/
	CurThed->ustk = alloc(CurProc->ufdmt, THEDSTK_SIZ);
	CurThed->UstkSiz = THEDSTK_SIZ;
	CurThed->attr |= THED_ATTR_APPS;
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
		"iret"				/*进入用户态*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(NewExec->entry)
	);
}

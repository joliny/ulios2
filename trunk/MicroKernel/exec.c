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
	if ((CurThed->ustk = LockAllocUFData(CurProc, UstkSiz)) == NULL)
		DeleteThed();
	CurThed->UstkSiz = UstkSiz;
	CurThed->attr |= THED_ATTR_APPS;
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	*(DWORD*)(CurThed->ustk + UstkSiz - sizeof(DWORD)) = CurThed->kstk[2];	/*复制子线程参数到用户堆栈*/
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
		"iret"				/*进入用户态*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz - sizeof(DWORD) * 2), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(CurThed->kstk[0])
	);
}

/*进程起点*/
void ProcStart()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *NewExec;
	DWORD NewPdt, PhyAddr;

	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (CurThed->kstk[0] & EXEC_ARGV_BASESRV)	/*启动基础服务进程*/
	{
		PHYBLK_DESC *CurSrv;
		PAGE_DESC *FstPg, *EndPg;

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
		NewExec->DataEnd = NewExec->DataOff = NewExec->CodeEnd = BASESRV_OFF + CurSrv->siz;
		NewPdt |= PAGE_ATTR_P;
		pt[(PT_ID << 10) | PT0_ID] = pddt0[CurThed->id.ProcID] = NewPdt;	/*映射页目录表副本*/
		memset32(&pt[PT0_ID << 10], 0, 0x400);
		pt[(PT0_ID << 10) | PT_ID] = NewPdt;	/*映射页目录表副本自身*/
		FstPg = &pt0[(DWORD)BASESRV_OFF >> 12];
		EndPg = &pt0[((DWORD)NewExec->CodeEnd + 0x00000FFF) >> 12];
		if (FillPt(FstPg, EndPg, PAGE_ATTR_P | PAGE_ATTR_U) != NO_ERROR)
		{
			LockFreePage(NewPdt);
			LockKfree(NewExec, sizeof(EXEC_DESC));
			DeleteThed();
		}
		PhyAddr = PAGE_ATTR_P | PAGE_ATTR_U | (DWORD)CurSrv->addr;
		for (; FstPg < EndPg; FstPg++)
		{
			*FstPg = PhyAddr;
			PhyAddr += PAGE_SIZE;
		}
		if (AllocExid(NewExec) != NO_ERROR)
		{
			ClearPage(&pt0[(DWORD)UADDR_OFF >> 12], &pt0[(DWORD)SHRDLIB_OFF >> 12], FALSE);
			LockFreePage(NewPdt);
			LockKfree(NewExec, sizeof(EXEC_DESC));
			DeleteThed();
		}
	}
	else	/*启动可执行文件进程*/
	{
		MESSAGE_DESC *msg;
		WORD pid;

		if ((msg = AllocMsg()) == NULL)	/*申请消息结构*/
			DeleteThed();
		msg->ptid = kpt[FS_KPORT];
		msg->data[0] = MSG_ATTR_USER;
		msg->data[3] = FS_API_GETEXEC;
		msg->data[4] = CurProc->par;
		msg->data[5] = CurThed->kstk[1];
		if (SendMsg(msg) != NO_ERROR)	/*向文件系统发送可执行体请求*/
		{
			FreeMsg(msg);
			DeleteThed();
		}
		if (WaitThedMsg(&msg, kpt[FS_KPORT], INVALID) != NO_ERROR)
			DeleteThed();
		if (msg->data[1] < (DWORD)UADDR_OFF)	/*可执行体信息有误*/
		{
			FreeMsg(msg);
			DeleteThed();
		}
		if ((pid = (WORD)msg->data[0]) != 0xFFFF)	/*有可重用可执行体*/
		{
			cli();
			if (pmt[pid] && pmt[pid]->exec)	/*可执行体有效*/
			{
				NewExec = pmt[pid]->exec;
				NewExec->cou++;
				pt[(PT_ID << 10) | PT0_ID] = pddt0[CurThed->id.ProcID] = pddt0[pid];	/*映射页目录表副本*/
				sti();
				goto strset;
			}
			sti();
		}
		if ((NewExec = (EXEC_DESC*)LockKmalloc(sizeof(EXEC_DESC))) == NULL)	/*没有可重用可执行体*/
		{
			msg->ptid = kpt[FS_KPORT];	/*通知文件服务器退出消息*/
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
		memcpy32(NewExec, msg->data, 8);	/*复制文件系统取得的可执行体信息*/
		ulock(&NewExec->Page_l);
		if ((NewPdt = LockAllocPage()) == 0)
		{
			LockKfree(NewExec, sizeof(EXEC_DESC));
			msg->ptid = kpt[FS_KPORT];	/*通知文件服务器退出消息*/
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
		NewPdt |= PAGE_ATTR_P;
		pt[(PT_ID << 10) | PT0_ID] = pddt0[CurThed->id.ProcID] = NewPdt;	/*映射页目录表副本*/
		memset32(&pt[PT0_ID << 10], 0, 0x400);
		pt[(PT0_ID << 10) | PT_ID] = NewPdt;	/*映射页目录表副本自身*/
		if (AllocExid(NewExec) != NO_ERROR)
		{
			LockFreePage(NewPdt);
			LockKfree(NewExec, sizeof(EXEC_DESC));
			msg->ptid = kpt[FS_KPORT];	/*通知文件服务器退出消息*/
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
strset:	FreeMsg(msg);
	}
	if (!(CurThed->kstk[0] & EXEC_ARGV_DRIVER))	/*设置用户应用进程*/
		CurProc->attr |= PROC_ATTR_APPS;
	CurProc->exec = NewExec;
	CurProc->EndUBlk = CurProc->FstUBlk = CurProc->ublkt;
	InitFbt(CurProc->ufdmt, UFDMT_LEN, UFDATA_OFF, UFDATA_SIZ);	/*初始化用户自由数据区*/
	CurThed->ustk = alloc(CurProc->ufdmt, THEDSTK_SIZ);
	CurThed->UstkSiz = THEDSTK_SIZ;
	CurThed->attr |= THED_ATTR_APPS;
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	*(DWORD*)(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE - sizeof(DWORD)) = (DWORD)CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE;
	strcpy((BYTE*)(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE), (const BYTE*)&CurThed->kstk[2]);	/*复制参数到用户堆栈*/
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
		"iret"				/*进入用户态*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE - sizeof(DWORD)), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(NewExec->entry)
	);
}

/*删除线程资源并退出*/
void ThedExit()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *CurExec;
	MESSAGE_DESC *msg;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	CurExec = CurProc->exec;
	CurThed->attr |= THED_ATTR_DEL;	/*标记为正在被删除*/
	UnregAllIrq();	/*清除线程资源*/
	UnregAllKnlPort();
	FreeAllMsg();
	lock(&CurProc->Page_l);
	ClearPage(&pt[(DWORD)CurThed->ustk >> 12], &pt[((DWORD)CurThed->ustk + CurThed->UstkSiz) >> 12], TRUE);
	ulock(&CurProc->Page_l);
	LockFreeUFData(CurProc, CurThed->ustk, CurThed->UstkSiz);
	if (CurThed->i387)	/*清除协处理器寄存器*/
	{
		LockKfree(CurThed->i387, sizeof(I387));
		cli();
		if (LastI387 == CurThed->i387)
			LastI387 = NULL;
		sti();
	}
	if ((msg = AllocMsg()) != NULL)	/*通知父线程退出消息*/
	{
		msg->ptid.ProcID = CurThed->id.ProcID;
		msg->ptid.ThedID = CurThed->par;
		msg->data[0] = MSG_ATTR_THEDEXT;
		if (SendMsg(msg) != NO_ERROR)
			FreeMsg(msg);
	}
	if (CurProc->TmdCou == 1)	/*只剩当前一个线程*/
	{
		CurProc->attr |= PROC_ATTR_DEL;	/*进程标记为正在被删除*/
		FreeAllMap();	/*清除进程资源*/
		lock(&CurProc->Page_l);
		ClearPageNoPt0(&pt[(DWORD)UADDR_OFF >> 12], &pt[(DWORD)SHRDLIB_OFF >> 12]);	/*释放地址空间页表*/
		ulock(&CurProc->Page_l);
		if ((msg = AllocMsg()) != NULL)	/*通知父进程退出消息*/
		{
			msg->ptid.ProcID = CurProc->par;
			msg->ptid.ThedID = 0;
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		if ((msg = AllocMsg()) != NULL)	/*通知文件服务器退出消息*/
		{
			msg->ptid = kpt[FS_KPORT];
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		if (--CurExec->cou == 0)	/*清除可执行体信息*/
		{
			lock(&CurExec->Page_l);
			ClearPage(&pt0[(DWORD)UADDR_OFF >> 12], &pt0[(DWORD)SHRDLIB_OFF >> 12], TRUE);
			ulock(&CurExec->Page_l);
			LockKfree(CurExec, sizeof(EXEC_DESC));
			FreeExid(&exmt[CurExec->id]);
			LockFreePage(pt[(PT_ID << 10) | PT0_ID]);
		}
	}
	DeleteThed();
}
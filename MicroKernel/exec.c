/*	exec.c for ulios
	���ߣ�����
	���ܣ���ִ������������̳߳�ʼ��
	����޸����ڣ�2009-09-01
*/

#include "knldef.h"

/*�߳����*/
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
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
	*(DWORD*)(CurThed->ustk + UstkSiz - sizeof(DWORD)) = CurThed->kstk[2];	/*�������̲߳������û���ջ*/
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
		"movw %0, %%ax\n"
		"movw %%ax, %%ds\n"
		"movw %%ax, %%es\n"
		"movw %%ax, %%fs\n"
		"movw %%ax, %%gs\n"
		"iret"				/*�����û�̬*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz - sizeof(DWORD) * 2), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(CurThed->kstk[0])
	);
}

/*�������*/
void ProcStart()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *NewExec;
	BLK_DESC *blk;
	DWORD NewPdt;

	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (CurThed->kstk[0] & EXEC_ARGV_BASESRV)	/*���������������*/
	{
		if ((NewExec = (EXEC_DESC*)LockKmalloc(sizeof(EXEC_DESC))) == NULL)
			DeleteThed();
		if ((NewPdt = LockAllocPage()) == 0)
		{
			LockKfree(NewExec, sizeof(EXEC_DESC));
			DeleteThed();
		}
		memset32(NewExec, 0, sizeof(EXEC_DESC) / sizeof(DWORD));
		NewExec->cou = 1;
		NewExec->entry = NewExec->CodeOff = BASESRV_OFF;
		NewExec->DataEnd = NewExec->DataOff = NewExec->CodeEnd = BASESRV_OFF + CurThed->kstk[2];
		NewPdt |= PAGE_ATTR_P;
		pt[(PT_ID << 10) | PT0_ID] = NewPdt;	/*ӳ��ҳĿ¼����*/
		memset32(&pt[PT0_ID << 10], 0, 0x400);
		if (FillConAddr(&pt0[(DWORD)BASESRV_OFF >> 12], &pt0[((DWORD)NewExec->CodeEnd + 0x00000FFF) >> 12], CurThed->kstk[1], PAGE_ATTR_P | PAGE_ATTR_U) != NO_ERROR)
		{
			LockFreePage(NewPdt);
			LockKfree(NewExec, sizeof(EXEC_DESC));
			DeleteThed();
		}
	}
	else	/*������ִ���ļ�����*/
	{
		MESSAGE_DESC *msg;
		DWORD pid;

		if ((pid = CurThed->kstk[1] & 0xFFFF) != 0xFFFF)	/*�п����ÿ�ִ����*/
		{
			cli();
			if (pmt[pid] && pmt[pid]->exec)	/*��ִ������Ч*/
			{
				NewExec = pmt[pid]->exec;
				NewExec->cou++;
				pt[(PT_ID << 10) | PT0_ID] = pdt[(pid << 10) | PT0_ID];	/*ӳ��ҳĿ¼����*/
				sti();
				goto strset;
			}
			sti();
		}
		if ((NewExec = (EXEC_DESC*)LockKmalloc(sizeof(EXEC_DESC))) == NULL)	/*û�п����ÿ�ִ����*/
		{
			if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
				DeleteThed();
			msg->ptid = kpt[FS_KPORT];	/*֪ͨ�ļ��������˳���Ϣ*/
			msg->data[MSG_ATTR_ID] = MSG_ATTR_PROCEXIT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
		memcpy32(NewExec, &CurThed->kstk[1], 8);	/*���ƿ�ִ������Ϣ*/
		NewExec->cou = 1;
		ulockw(&NewExec->Page_l);
		if ((NewPdt = LockAllocPage()) == 0)
		{
			LockKfree(NewExec, sizeof(EXEC_DESC));
			if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
				DeleteThed();
			msg->ptid = kpt[FS_KPORT];	/*֪ͨ�ļ��������˳���Ϣ*/
			msg->data[MSG_ATTR_ID] = MSG_ATTR_PROCEXIT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
		NewPdt |= PAGE_ATTR_P;
		pt[(PT_ID << 10) | PT0_ID] = NewPdt;	/*ӳ��ҳĿ¼����*/
		memset32(&pt[PT0_ID << 10], 0, 0x400);
	}
strset:
	if (!(CurThed->kstk[0] & EXEC_ARGV_DRIVER))	/*�����û�Ӧ�ý���*/
		CurProc->attr |= PROC_ATTR_APPS;
	CurProc->exec = NewExec;
	CurProc->EndUBlk = CurProc->FstUBlk = CurProc->ublkt;
	for (blk = CurProc->ublkt; blk < &CurProc->ublkt[UBLKT_LEN - 1]; blk++)
		blk->addr = (void*)(blk + 1);
	blk->addr = NULL;
	InitFbt(CurProc->ufdmt, UFDMT_LEN, UFDATA_OFF, UFDATA_SIZ);	/*��ʼ���û�����������*/
	CurThed->ustk = alloc(CurProc->ufdmt, THEDSTK_SIZ);
	CurThed->UstkSiz = THEDSTK_SIZ;
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
	*(DWORD*)(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE - sizeof(DWORD)) = (DWORD)CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE;
	strcpy((BYTE*)(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE), (const BYTE*)&CurThed->kstk[9]);	/*���Ʋ������û���ջ*/
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
		"movw %0, %%ax\n"
		"movw %%ax, %%ds\n"
		"movw %%ax, %%es\n"
		"movw %%ax, %%fs\n"
		"movw %%ax, %%gs\n"
		"iret"				/*�����û�̬*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE - sizeof(DWORD) * 2), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(NewExec->entry)
	);
}

/*ɾ���߳���Դ���˳�*/
void ThedExit(DWORD ExitCode)
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *CurExec;
	MESSAGE_DESC *msg;

	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	CurExec = CurProc->exec;
	CurThed->attr |= THED_ATTR_DEL;	/*���Ϊ���ڱ�ɾ��*/
	UnregAllIrq();	/*����߳���Դ*/
	UnregAllKnlPort();
	FreeAllMsg();
	lock(&CurProc->Page_l);
	ClearPage(&pt[(DWORD)CurThed->ustk >> 12], &pt[((DWORD)CurThed->ustk + CurThed->UstkSiz) >> 12], TRUE);	/*�����ջҳ*/
	ulock(&CurProc->Page_l);
	LockFreeUFData(CurProc, CurThed->ustk, CurThed->UstkSiz);
	if (CurThed->i387)	/*���Э�������Ĵ���*/
	{
		LockKfree(CurThed->i387, sizeof(I387));
		cli();
		if (LastI387 == CurThed->i387)
			LastI387 = NULL;
		sti();
	}
	if ((msg = AllocMsg()) != NULL)	/*֪ͨ���߳��˳���Ϣ*/
	{
		msg->ptid.ProcID = CurThed->id.ProcID;
		msg->ptid.ThedID = CurThed->par;
		msg->data[MSG_ATTR_ID] = MSG_ATTR_THEDEXIT;
		msg->data[1] = ExitCode;
		if (SendMsg(msg) != NO_ERROR)
			FreeMsg(msg);
	}
	if (CurProc->TmdCou == 1)	/*ֻʣ��ǰһ���߳�*/
	{
		CurProc->attr |= PROC_ATTR_DEL;	/*���̱��Ϊ���ڱ�ɾ��*/
		FreeAllMap();	/*���������Դ*/
		lock(&CurProc->Page_l);
		ClearPageNoPt0(&pt[(DWORD)UADDR_OFF >> 12], &pt[(DWORD)SHRDLIB_OFF >> 12]);	/*�ͷŵ�ַ�ռ�ҳ��*/
		ulock(&CurProc->Page_l);
		if ((msg = AllocMsg()) != NULL)	/*֪ͨ�������˳���Ϣ*/
		{
			msg->ptid.ProcID = CurProc->par;
			msg->ptid.ThedID = 0;
			msg->data[MSG_ATTR_ID] = MSG_ATTR_PROCEXIT;
			msg->data[1] = ExitCode;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		if ((msg = AllocMsg()) != NULL)	/*֪ͨ�ļ��������˳���Ϣ*/
		{
			msg->ptid = kpt[FS_KPORT];
			msg->data[MSG_ATTR_ID] = MSG_ATTR_PROCEXIT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		if (--CurExec->cou == 0)	/*�����ִ������Ϣ*/
		{
			lockw(&CurExec->Page_l);
			ClearPage(&pt0[(DWORD)UADDR_OFF >> 12], &pt0[(DWORD)SHRDLIB_OFF >> 12], TRUE);
			ulockw(&CurExec->Page_l);
			LockKfree(CurExec, sizeof(EXEC_DESC));
			LockFreePage(pt[(PT_ID << 10) | PT0_ID]);
		}
	}
	DeleteThed();
}

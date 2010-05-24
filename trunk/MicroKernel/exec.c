/*	exec.c for ulios
	���ߣ�����
	���ܣ���ִ������������̳߳�ʼ��
	����޸����ڣ�2009-09-01
*/

#include "knldef.h"

/*��ʼ����ִ��������*/
void InitEXMT()
{
	memset32(exmt, 0, EXMT_LEN * sizeof(EXEC_DESC*) / sizeof(DWORD));
	EndExmd = FstExmd = exmt;
}

/*����տ�ִ����ID*/
long AllocExid(EXEC_DESC *exec)
{
	cli();
	if (FstExmd >= &exmt[EXMT_LEN])
	{
		sti();
		return ERROR_HAVENO_EXECID;	// û�пտ�ִ����ID
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

/*�ͷſտ�ִ����ID*/
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
	CurThed->attr |= THED_ATTR_APPS;
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	*(DWORD*)(CurThed->ustk + UstkSiz - sizeof(DWORD)) = CurThed->kstk[2];	/*�������̲߳������û���ջ*/
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
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
	DWORD NewPdt, PhyAddr;

	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (CurThed->kstk[0] & EXEC_ARGV_BASESRV)	/*���������������*/
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
		pt[(PT_ID << 10) | PT0_ID] = pddt0[CurThed->id.ProcID] = NewPdt;	/*ӳ��ҳĿ¼����*/
		memset32(&pt[PT0_ID << 10], 0, 0x400);
		pt[(PT0_ID << 10) | PT_ID] = NewPdt;	/*ӳ��ҳĿ¼��������*/
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
	else	/*������ִ���ļ�����*/
	{
		MESSAGE_DESC *msg;
		WORD pid;

		if ((msg = AllocMsg()) == NULL)	/*������Ϣ�ṹ*/
			DeleteThed();
		msg->ptid = kpt[FS_KPORT];
		msg->data[0] = MSG_ATTR_USER;
		msg->data[3] = FS_API_GETEXEC;
		msg->data[4] = CurProc->par;
		msg->data[5] = CurThed->kstk[1];
		if (SendMsg(msg) != NO_ERROR)	/*���ļ�ϵͳ���Ϳ�ִ��������*/
		{
			FreeMsg(msg);
			DeleteThed();
		}
		if (WaitThedMsg(&msg, kpt[FS_KPORT], INVALID) != NO_ERROR)
			DeleteThed();
		if (msg->data[1] < (DWORD)UADDR_OFF)	/*��ִ������Ϣ����*/
		{
			FreeMsg(msg);
			DeleteThed();
		}
		if ((pid = (WORD)msg->data[0]) != 0xFFFF)	/*�п����ÿ�ִ����*/
		{
			cli();
			if (pmt[pid] && pmt[pid]->exec)	/*��ִ������Ч*/
			{
				NewExec = pmt[pid]->exec;
				NewExec->cou++;
				pt[(PT_ID << 10) | PT0_ID] = pddt0[CurThed->id.ProcID] = pddt0[pid];	/*ӳ��ҳĿ¼����*/
				sti();
				goto strset;
			}
			sti();
		}
		if ((NewExec = (EXEC_DESC*)LockKmalloc(sizeof(EXEC_DESC))) == NULL)	/*û�п����ÿ�ִ����*/
		{
			msg->ptid = kpt[FS_KPORT];	/*֪ͨ�ļ��������˳���Ϣ*/
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
		memcpy32(NewExec, msg->data, 8);	/*�����ļ�ϵͳȡ�õĿ�ִ������Ϣ*/
		ulock(&NewExec->Page_l);
		if ((NewPdt = LockAllocPage()) == 0)
		{
			LockKfree(NewExec, sizeof(EXEC_DESC));
			msg->ptid = kpt[FS_KPORT];	/*֪ͨ�ļ��������˳���Ϣ*/
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
		NewPdt |= PAGE_ATTR_P;
		pt[(PT_ID << 10) | PT0_ID] = pddt0[CurThed->id.ProcID] = NewPdt;	/*ӳ��ҳĿ¼����*/
		memset32(&pt[PT0_ID << 10], 0, 0x400);
		pt[(PT0_ID << 10) | PT_ID] = NewPdt;	/*ӳ��ҳĿ¼��������*/
		if (AllocExid(NewExec) != NO_ERROR)
		{
			LockFreePage(NewPdt);
			LockKfree(NewExec, sizeof(EXEC_DESC));
			msg->ptid = kpt[FS_KPORT];	/*֪ͨ�ļ��������˳���Ϣ*/
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
			DeleteThed();
		}
strset:	FreeMsg(msg);
	}
	if (!(CurThed->kstk[0] & EXEC_ARGV_DRIVER))	/*�����û�Ӧ�ý���*/
		CurProc->attr |= PROC_ATTR_APPS;
	CurProc->exec = NewExec;
	CurProc->EndUBlk = CurProc->FstUBlk = CurProc->ublkt;
	InitFbt(CurProc->ufdmt, UFDMT_LEN, UFDATA_OFF, UFDATA_SIZ);	/*��ʼ���û�����������*/
	CurThed->ustk = alloc(CurProc->ufdmt, THEDSTK_SIZ);
	CurThed->UstkSiz = THEDSTK_SIZ;
	CurThed->attr |= THED_ATTR_APPS;
	CurThed->tss.stk[0].esp = (DWORD)CurThed + sizeof(THREAD_DESC);
	CurThed->tss.stk[0].ss = KDATA_SEL;
	*(DWORD*)(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE - sizeof(DWORD)) = (DWORD)CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE;
	strcpy((BYTE*)(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE), (const BYTE*)&CurThed->kstk[2]);	/*���Ʋ������û���ջ*/
	__asm__
	(
		"pushl %0\n"		/*ss3*/
		"pushl %1\n"		/*esp3*/
		"pushl %2\n"		/*eflags*/
		"pushl %3\n"		/*cs*/
		"pushl %4\n"		/*eip*/
		"iret"				/*�����û�̬*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz - PROC_ARGS_SIZE - sizeof(DWORD)), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(NewExec->entry)
	);
}

/*ɾ���߳���Դ���˳�*/
void ThedExit()
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
	ClearPage(&pt[(DWORD)CurThed->ustk >> 12], &pt[((DWORD)CurThed->ustk + CurThed->UstkSiz) >> 12], TRUE);
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
		msg->data[0] = MSG_ATTR_THEDEXT;
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
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		if ((msg = AllocMsg()) != NULL)	/*֪ͨ�ļ��������˳���Ϣ*/
		{
			msg->ptid = kpt[FS_KPORT];
			msg->data[0] = MSG_ATTR_PROCEXT;
			if (SendMsg(msg) != NO_ERROR)
				FreeMsg(msg);
		}
		if (--CurExec->cou == 0)	/*�����ִ������Ϣ*/
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
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
		"iret"				/*�����û�̬*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(CurThed->kstk[0])
	);
}

/*�������*/
void ProcStart()
{
	PROCESS_DESC *CurProc;
	THREAD_DESC *CurThed;
	EXEC_DESC *NewExec;

	sti();
	CurProc = CurPmd;
	CurThed = CurProc->CurTmd;
	if (CurThed->kstk[0] & EXEC_ARGS_BASESRV)	/*���������������*/
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
		pddt0[pdti] = PAGE_ATTR_P | NewPdt;	/*ӳ��ҳĿ¼����*/
		pdti <<= 10;
		pdt[pdti | PT0_ID] = PAGE_ATTR_P | NewPdt;	/*ӳ��ҳ����*/
		memset32(&pdt0[pdti], 0, 0x400);
		for (i = pdti | ((DWORD)BASESRV_OFF >> 22), end = pdti | (((DWORD)NewExec->CodeEnd + 0x003FFFFF) >> 22); i < end; i++)
		{
			DWORD PtAddr;	/*ҳ��������ַ*/

			if ((PtAddr = LockAllocPage()) == 0)
			{
				LockKfree(NewExec, sizeof(EXEC_DESC));
				DeleteThed();
			}
			pdt0[i] = PAGE_ATTR_P | PAGE_ATTR_U | PtAddr;	/*�޸�ҳĿ¼��,��Ϊֻ��*/
			memset32(&pt0[(i << 10) & 0x000FFC00], 0, 0x400);	/*���ҳ��*/
		}
		PhyAddr = PAGE_ATTR_P | PAGE_ATTR_U | (DWORD)CurSrv->addr;
		for (i = ((DWORD)BASESRV_OFF >> 12), end = (((DWORD)NewExec->CodeEnd + 0x00000FFF) >> 12); i < end; i++)	/*�޸�ҳ��,ӳ���ַ*/
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
	else	/*������ִ���ļ�����*/
	{
		NewExec = NULL;
	}
	if (!(CurThed->kstk[0] & EXEC_ARGS_DRIVER))	/*�����û�Ӧ�ý���*/
		CurProc->attr |= PROC_ATTR_APPS;
	CurProc->exec = NewExec;
	CurProc->EndUBlk = CurProc->FstUBlk = CurProc->ublkt;
	InitFbt(CurProc->ufdmt, UFDMT_LEN, UFDATA_OFF, UFDATA_SIZ);	/*��ʼ���û�����������*/
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
		"iret"				/*�����û�̬*/
		::"i"(UDATA_SEL), "r"(CurThed->ustk + CurThed->UstkSiz), "r"((CurProc->attr & PROC_ATTR_APPS) ? EFLAGS_IF : (EFLAGS_IF | EFLAGS_IOPL)), "i"(UCODE_SEL), "m"(NewExec->entry)
	);
}

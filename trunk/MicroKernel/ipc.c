/*	ipc.c for ulios
	���ߣ�����
	���ܣ����̼�ͨѶ
	����޸����ڣ�2009-06-22
*/

#include "knldef.h"

/*��ʼ����Ϣ����*/
long InitMsg()
{
	if ((msgmt = (MESSAGE_DESC*)kmalloc(MSGMT_LEN * sizeof(MESSAGE_DESC))) == NULL)
		return ERROR_HAVENO_KMEM;
	memset32(msgmt, 0, MSGMT_LEN * sizeof(MESSAGE_DESC) / sizeof(DWORD));
	FstMsg = msgmt;

	return NO_ERROR;
}

/*������Ϣ�ṹ*/
MESSAGE_DESC *AllocMsg()
{
	MESSAGE_DESC *msg;

	cli();
	if (FstMsg >= &msgmt[MSGMT_LEN])
	{
		sti();
		return NULL;
	}
	msg = FstMsg;
	msg->data[0] = INVALID;
	do
		FstMsg++;
	while (FstMsg < &msgmt[MSGMT_LEN] && FstMsg->data[0]);
	sti();
	return msg;
}

/*�ͷ���Ϣ�ṹ*/
void FreeMsg(MESSAGE_DESC *msg)
{
	cli();
	msg->data[0] = 0;
	if (FstMsg > msg)
		FstMsg = msg;
	sti();
}

/*������Ϣ*/
long SendMsg(MESSAGE_DESC *msg)
{
	PROCESS_DESC *RcvProc;
	THREAD_DESC *RcvThed;
	THREAD_ID rptid;

	rptid = msg->ptid;
	cli();	/*Ҫ�����������̵���Ϣ,���Է�ֹ�����л�*/
	RcvProc = pmt[rptid.ProcID];
	if (RcvProc == NULL || (RcvProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_PROCID;
	}
	RcvThed = RcvProc->tmt[rptid.ThedID];
	if (RcvThed == NULL || (RcvThed->attr & THED_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_THEDID;
	}
	if (RcvThed->MsgCou >= THED_MSG_LEN)
	{
		sti();
		return ERROR_HAVENO_MSGDESC;
	}
	if (CurPmd)	/*���÷�����ID*/
		msg->ptid = CurPmd->CurTmd->id;
	else
		msg->ptid.ProcID = INVWID;
	msg->nxt = NULL;
	if (RcvThed->msg == NULL)
		RcvThed->msg = msg;
	else
		RcvThed->lst->nxt = msg;
	RcvThed->lst = msg;
	RcvThed->MsgCou++;
	if (RcvThed->attr & THED_ATTR_SLEEP)	/*�߳�����,���Ȼ����߳�*/
		wakeup(RcvThed);
	sti();
	return NO_ERROR;
}

/*������Ϣ*/
long RecvMsg(MESSAGE_DESC **msg)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	cli();
	if (CurThed->msg)	/*�����Ϣ����*/
	{
		*msg = CurThed->msg;
		CurThed->msg = (*msg)->nxt;
		CurThed->MsgCou--;
	}
	else	/*û����Ϣ*/
	{
		sti();
		return ERROR_HAVENO_MSGDESC;
	}
	sti();
	return NO_ERROR;
}

/*�������ȴ���Ϣ*/
void WaitMsg(MESSAGE_DESC **msg)
{
	if (RecvMsg(msg) != NO_ERROR)	/*����Ϣ������Ϣ*/
	{
		cli();
		sleep(FALSE);	/*û����Ϣ�͵ȴ�*/
		sti();
		RecvMsg(msg);
	}
}

/*����̵߳���Ϣ����*/
void FreeAllMsg()
{
	THREAD_DESC *CurThed;
	MESSAGE_DESC *msg;

	CurThed = CurPmd->CurTmd;
	cli();
	for (msg = CurThed->msg; msg; msg = msg->nxt)
	{
		msg->data[0] = 0;
		if (FstMsg > msg)
			FstMsg = msg;
	}
	CurThed->msg = NULL;
	CurThed->MsgCou = 0;
	sti();
}

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

/*������Ϣ*/
long SendMsg(MESSAGE_DESC *msg)
{
	PROCESS_DESC *DstProc;
	THREAD_DESC *CurThed, *DstThed;
	THREAD_ID ptid;

	ptid = msg->ptid;
	if (ptid.ProcID >= PMT_LEN)
		return ERROR_WRONG_PROCID;
	if (ptid.ThedID >= TMT_LEN)
		return ERROR_WRONG_THEDID;
	CurThed = CurPmd ? CurPmd->CurTmd : NULL;
	cli();	/*Ҫ�����������̵���Ϣ,���Է�ֹ�����л�*/
	DstProc = pmt[ptid.ProcID];
	if (DstProc == NULL || (DstProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_PROCID;
	}
	DstThed = DstProc->tmt[ptid.ThedID];
	if (DstThed == NULL || (DstThed->attr & THED_ATTR_DEL))
	{
		sti();
		return ERROR_WRONG_THEDID;
	}
	if (DstThed->MsgCou >= THED_MSG_LEN && msg->data[0] >= MSG_ATTR_USER && *(DWORD*)(&DstThed->WaitId) != *(DWORD*)(&CurThed->id))
	{
		sti();
		return ERROR_HAVENO_MSGDESC;	/*��Ϣ�������û���Ϣ��Ŀ���߳�û�еȴ����̵߳���Ϣ,ȡ��������Ϣ*/
	}
	if (CurThed)	/*���÷�����ID*/
		msg->ptid = CurThed->id;
	else
		*(DWORD*)(&msg->ptid) = INVALID;
	msg->nxt = NULL;
	if (DstThed->msg == NULL)
		DstThed->msg = msg;
	else
		DstThed->lst->nxt = msg;
	DstThed->lst = msg;
	DstThed->MsgCou++;
	if (DstThed->attr & THED_ATTR_SLEEP)	/*�߳�����,���Ȼ����߳�*/
		wakeup(DstThed);
	sti();
	return NO_ERROR;
}

/*������Ϣ*/
long RecvMsg(MESSAGE_DESC **msg, DWORD cs)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	cli();
	if (CurThed->msg)
		goto getmsg;
	if (cs == 0)	/*û����Ϣ�Ҳ��ȴ�*/
	{
		sti();
		return ERROR_HAVENO_MSGDESC;
	}
	sleep(cs);
	if (CurThed->msg == NULL)	/*��Ȼû����Ϣ*/
	{
		sti();
		return ERROR_OUT_OF_TIME;
	}
getmsg:
	*msg = CurThed->msg;
	CurThed->msg = (*msg)->nxt;
	CurThed->MsgCou--;
	sti();
	return NO_ERROR;
}

/*�������ȴ�ָ�����̵���Ϣ*/
long WaitThedMsg(MESSAGE_DESC **msg, THREAD_ID ptid, DWORD cs)
{
	THREAD_DESC *CurThed;
	MESSAGE_DESC *PreMsg, *CurMsg;

	CurThed = CurPmd->CurTmd;
	cli();
	for (PreMsg = NULL, CurMsg = CurThed->msg; CurMsg; CurMsg = (PreMsg = CurMsg)->nxt)	/*����������Ϣ*/
		if (CurMsg->ptid.ProcID == ptid.ProcID)
			goto getmsg;
	CurThed->WaitId = ptid;	/*���õȴ���Ϣ�߳�*/
	if (cs != INVALID)
		for (;;)	/*�ȴ�һ��ʱ��*/
		{
			DWORD CurClock;

			CurClock = clock;
			sleep(cs);
			if (PreMsg)
				CurMsg = PreMsg->nxt;
			else
				CurMsg = CurThed->msg;
			if (CurMsg == NULL)	/*��ʱ����Ϣ*/
			{
				*(DWORD*)(&CurThed->WaitId) = INVALID;
				sti();
				return ERROR_OUT_OF_TIME;
			}
			if (CurMsg->ptid.ProcID == ptid.ProcID)
				break;
			if (clock - CurClock >= cs)
			{
				*(DWORD*)(&CurThed->WaitId) = INVALID;
				sti();
				return ERROR_OUT_OF_TIME;
			}
			cs -= clock - CurClock;
			PreMsg = CurMsg;
		}
	else
		for (;;)	/*���޵ȴ�*/
		{
			sleep(INVALID);
			if (PreMsg)
				CurMsg = PreMsg->nxt;
			else
				CurMsg = CurThed->msg;
			if (CurMsg->ptid.ProcID == ptid.ProcID)
				break;
			PreMsg = CurMsg;
		}
	*(DWORD*)(&CurThed->WaitId) = INVALID;
getmsg:
	if (PreMsg)
		PreMsg->nxt = CurMsg->nxt;
	else
		CurThed->msg = CurMsg->nxt;
	if (CurThed->lst == CurMsg)
		CurThed->lst = PreMsg;
	CurThed->MsgCou--;
	sti();
	*msg = CurMsg;
	return NO_ERROR;
}

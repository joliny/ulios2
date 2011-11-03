/*	ipc.c for ulios
	���ߣ�����
	���ܣ����̼�ͨѶ
	����޸����ڣ�2009-06-22
*/

#include "knldef.h"

/*������Ϣ�ṹ*/
MESSAGE_DESC *AllocMsg()
{
	MESSAGE_DESC *msg;

	cli();
	if (FstMsg == NULL)
	{
		sti();
		return NULL;
	}
	FstMsg = (msg = FstMsg)->nxt;
	sti();
	return msg;
}

/*�ͷ���Ϣ�ṹ*/
void FreeMsg(MESSAGE_DESC *msg)
{
	cli();
	msg->nxt = FstMsg;
	FstMsg = msg;
	sti();
}

/*����̵߳���Ϣ����*/
void FreeAllMsg()
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	cli();
	if (CurThed->msg)
	{
		CurThed->lst->nxt = FstMsg;
		FstMsg = CurThed->msg;
		CurThed->msg = NULL;
		CurThed->MsgCou = 0;
	}
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
		return KERR_INVALID_PROCID;
	if (ptid.ThedID >= TMT_LEN)
		return KERR_INVALID_THEDID;
	CurThed = (msg->data[MSG_ATTR_ID] == MSG_ATTR_IRQ) ? NULL : CurPmd->CurTmd;	/*IRQ��Ϣ�������κ��߳��з���,������Ϣ������������*/
	cli();	/*Ҫ�����������̵���Ϣ,���Է�ֹ�����л�*/
	DstProc = pmt[ptid.ProcID];
	if (DstProc == NULL || (DstProc->attr & PROC_ATTR_DEL))
	{
		sti();
		return KERR_PROC_NOT_EXIST;
	}
	DstThed = DstProc->tmt[ptid.ThedID];
	if (DstThed == NULL || (DstThed->attr & THED_ATTR_DEL))
	{
		sti();
		return KERR_THED_NOT_EXIST;
	}
	if (DstThed->MsgCou >= THED_MSG_LEN && msg->data[MSG_ATTR_ID] >= MSG_ATTR_USER && *(DWORD*)(&DstThed->WaitId) != *(DWORD*)(&CurThed->id))
	{
		sti();
		return KERR_MSG_QUEUE_FULL;	/*��Ϣ�������û���Ϣ��Ŀ���߳�û�еȴ����̵߳���Ϣ,ȡ��������Ϣ*/
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
	if (DstThed->attr & THED_ATTR_WAITMSG)	/*�߳�����,�ȴ���Ϣ,���Ȼ����߳�*/
		wakeup(DstThed);
	else if (DstThed->MsgCou >= THED_MSG_LEN * 3 / 4 && !(DstThed->attr & THED_ATTR_SLEEP))	/*��Ϣ���н���ʱ�л���Ŀ���߳�*/
	{
		CurPmd = DstProc;
		DstProc->CurTmd = DstThed;
		SwitchTS();
	}
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
		return KERR_MSG_QUEUE_EMPTY;
	}
	sleep(TRUE, cs);
	if (CurThed->msg == NULL)	/*��Ȼû����Ϣ*/
	{
		sti();
		return KERR_OUT_OF_TIME;
	}
getmsg:
	*msg = CurThed->msg;
	CurThed->msg = (*msg)->nxt;
	CurThed->MsgCou--;
	sti();
	return NO_ERROR;
}

/*����ָ�����̵���Ϣ*/
long RecvProcMsg(MESSAGE_DESC **msg, THREAD_ID ptid, DWORD cs)
{
	THREAD_DESC *CurThed;
	MESSAGE_DESC *PreMsg, *CurMsg;

	CurThed = CurPmd->CurTmd;
	cli();
	for (PreMsg = NULL, CurMsg = CurThed->msg; CurMsg; CurMsg = (PreMsg = CurMsg)->nxt)	/*����������Ϣ*/
		if (CurMsg->ptid.ProcID == ptid.ProcID)
			goto getmsg;
	CurThed->WaitId = ptid;	/*���õȴ���Ϣ�߳�*/
	for (;;)	/*�ȴ���Ϣ*/
	{
		DWORD CurClock;

		CurClock = 0;
		if (cs != INVALID)
			CurClock = clock;
		sleep(TRUE, cs);
		if (PreMsg)
			CurMsg = PreMsg->nxt;
		else
			CurMsg = CurThed->msg;
		if (CurMsg == NULL)	/*��ʱ����Ϣ*/
		{
			*(DWORD*)(&CurThed->WaitId) = INVALID;
			sti();
			return KERR_OUT_OF_TIME;
		}
		if (CurMsg->ptid.ProcID == ptid.ProcID)
		{
			*(DWORD*)(&CurThed->WaitId) = INVALID;
			break;
		}
		if (cs != INVALID)
		{
			if (clock - CurClock >= cs)	/*���ǵȴ�������*/
			{
				*(DWORD*)(&CurThed->WaitId) = INVALID;
				sti();
				return KERR_OUT_OF_TIME;
			}
			cs -= clock - CurClock;
		}
		PreMsg = CurMsg;
	}
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

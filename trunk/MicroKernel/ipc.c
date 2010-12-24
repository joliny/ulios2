/*	ipc.c for ulios
	作者：孙亮
	功能：进程间通讯
	最后修改日期：2009-06-22
*/

#include "knldef.h"

/*分配消息结构*/
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

/*释放消息结构*/
void FreeMsg(MESSAGE_DESC *msg)
{
	cli();
	msg->nxt = FstMsg;
	FstMsg = msg;
	sti();
}

/*清除线程的消息队列*/
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

/*发送消息*/
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
	CurThed = (msg->data[0] == MSG_ATTR_IRQ) ? NULL : CurPmd->CurTmd;	/*IRQ消息可能在任何线程中发出,所以消息发送者是无效的*/
	cli();	/*要访问其他进程的信息,所以防止任务切换*/
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
		return ERROR_HAVENO_MSGDESC;	/*消息满且是用户消息且目标线程没有等待本线程的消息,取消发送消息*/
	}
	if (CurThed)	/*设置发送者ID*/
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
	if (DstThed->attr & THED_ATTR_SLEEP)	/*线程阻塞,首先唤醒线程*/
		wakeup(DstThed);
	sti();
	return NO_ERROR;
}

/*接收消息*/
long RecvMsg(MESSAGE_DESC **msg, DWORD cs)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	cli();
	if (CurThed->msg)
		goto getmsg;
	if (cs == 0)	/*没有消息且不等待*/
	{
		sti();
		return ERROR_HAVENO_MSGDESC;
	}
	sleep(cs);
	if (CurThed->msg == NULL)	/*仍然没有消息*/
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

/*接收指定进程的消息*/
long RecvProcMsg(MESSAGE_DESC **msg, THREAD_ID ptid, DWORD cs)
{
	THREAD_DESC *CurThed;
	MESSAGE_DESC *PreMsg, *CurMsg;

	CurThed = CurPmd->CurTmd;
	cli();
	for (PreMsg = NULL, CurMsg = CurThed->msg; CurMsg; CurMsg = (PreMsg = CurMsg)->nxt)	/*查找已有消息*/
		if (CurMsg->ptid.ProcID == ptid.ProcID)
			goto getmsg;
	CurThed->WaitId = ptid;	/*设置等待消息线程*/
	for (;;)	/*等待消息*/
	{
		DWORD CurClock;

		CurClock = 0;
		if (cs != INVALID)
			CurClock = clock;
		sleep(cs);
		if (PreMsg)
			CurMsg = PreMsg->nxt;
		else
			CurMsg = CurThed->msg;
		if (CurMsg == NULL)	/*超时无消息*/
		{
			*(DWORD*)(&CurThed->WaitId) = INVALID;
			sti();
			return ERROR_OUT_OF_TIME;
		}
		if (CurMsg->ptid.ProcID == ptid.ProcID)
		{
			*(DWORD*)(&CurThed->WaitId) = INVALID;
			break;
		}
		if (cs != INVALID)
		{
			if (clock - CurClock >= cs)	/*检查非等待发送者*/
			{
				*(DWORD*)(&CurThed->WaitId) = INVALID;
				sti();
				return ERROR_OUT_OF_TIME;
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

/*	ipc.c for ulios
	作者：孙亮
	功能：进程间通讯
	最后修改日期：2009-06-22
*/

#include "knldef.h"

/*初始化消息管理*/
long InitMsg()
{
	if ((msgmt = (MESSAGE_DESC*)kmalloc(MSGMT_LEN * sizeof(MESSAGE_DESC))) == NULL)
		return ERROR_HAVENO_KMEM;
	memset32(msgmt, 0, MSGMT_LEN * sizeof(MESSAGE_DESC) / sizeof(DWORD));
	FstMsg = msgmt;

	return NO_ERROR;
}

/*分配消息结构*/
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

/*释放消息结构*/
void FreeMsg(MESSAGE_DESC *msg)
{
	cli();
	msg->data[0] = 0;
	if (FstMsg > msg)
		FstMsg = msg;
	sti();
}

/*发送消息*/
long SendMsg(MESSAGE_DESC *msg)
{
	PROCESS_DESC *RcvProc;
	THREAD_DESC *RcvThed;
	THREAD_ID rptid;

	rptid = msg->ptid;
	cli();	/*要访问其他进程的信息,所以防止任务切换*/
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
	if (CurPmd)	/*设置发送者ID*/
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
	if (RcvThed->attr & THED_ATTR_SLEEP)	/*线程阻塞,首先唤醒线程*/
		wakeup(RcvThed);
	sti();
	return NO_ERROR;
}

/*接收消息*/
long RecvMsg(MESSAGE_DESC **msg)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	cli();
	if (CurThed->msg)	/*检查消息链表*/
	{
		*msg = CurThed->msg;
		CurThed->msg = (*msg)->nxt;
		CurThed->MsgCou--;
	}
	else	/*没有消息*/
	{
		sti();
		return ERROR_HAVENO_MSGDESC;
	}
	sti();
	return NO_ERROR;
}

/*阻塞并等待消息*/
void WaitMsg(MESSAGE_DESC **msg)
{
	if (RecvMsg(msg) != NO_ERROR)	/*有消息返回消息*/
	{
		cli();
		sleep(FALSE);	/*没有消息就等待*/
		sti();
		RecvMsg(msg);
	}
}

/*清除线程的消息队列*/
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

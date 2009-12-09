/*	ipc.h for ulios
	作者：孙亮
	功能：进程间通讯定义
	最后修改日期：2009-06-22
*/

#ifndef _IPC_H_
#define _IPC_H_

#include "ulidef.h"

#define MSGMT_LEN			0x0800		/*消息管理表长度*/

/*消息属性,只占用消息结构data[0]的高16位*/
#define MSG_ATTR_ISR		0x00010000	/*异常信号消息*/
#define MSG_ATTR_IRQ		0x00020000	/*中断信号消息*/
#define MSG_ATTR_SYS		0x00030000	/*系统消息*/
#define MSG_ATTR_READ		0x00040000	/*读数据消息,页映射方式读*/
#define MSG_ATTR_WRITE		0x00050000	/*写数据消息,页映射方式写*/
#define MSG_ATTR_PROC		0x01000000	/*进程自定义消息最小值*/
#define THED_MSG_LEN		16			/*线程中消息链表长度*/

typedef struct _MESSAGE_DESC
{
	THREAD_ID ptid;				/*进程线程ID*/
	DWORD data[4];				/*消息数据*/
	struct _MESSAGE_DESC *nxt;	/*后一项*/
}MESSAGE_DESC;	/*消息描述符*/

/*初始化消息管理*/
long InitMsg();

/*分配消息结构*/
MESSAGE_DESC *AllocMsg();

/*释放消息结构*/
void FreeMsg(MESSAGE_DESC *msg);

/*发送消息*/
long SendMsg(MESSAGE_DESC *msg);

/*接收消息*/
long RecvMsg(MESSAGE_DESC **msg);

/*阻塞并等待消息*/
void WaitMsg(MESSAGE_DESC **msg);

/*清除线程的消息队列*/
void FreeAllMsg();

#endif

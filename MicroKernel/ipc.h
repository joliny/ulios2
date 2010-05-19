/*	ipc.h for ulios
	作者：孙亮
	功能：进程间通讯定义
	最后修改日期：2009-06-22
*/

#ifndef _IPC_H_
#define _IPC_H_

#include "ulidef.h"

#define MSGMT_LEN			0x1000		/*消息管理表长度*/
/*消息属性,只占用消息结构data[0]的高16位*/
#define MSG_ATTR_ISR		0x00010000	/*硬件陷阱消息*/
#define MSG_ATTR_IRQ		0x00020000	/*硬件中断消息*/

#define MSG_ATTR_THEDEXT	0x00100000	/*线程退出消息*/
#define MSG_ATTR_PROCEXT	0x00110000	/*进程退出消息*/
#define MSG_ATTR_EXCEP		0x00120000	/*异常退出消息*/
#define MSG_ATTR_MAP		0x00130000	/*页映射消息*/
#define MSG_ATTR_UNMAP		0x00140000	/*解除页映射消息*/
#define MSG_ATTR_CNLMAP		0x00150000	/*取消页映射消息*/

#define MSG_ATTR_USER		0x01000000	/*用户自定义消息最小值*/
#define THED_MSG_LEN		32			/*线程中消息链表长度*/
#define MSG_DATA_LEN		8			/*消息数据总双字数*/

typedef struct _MESSAGE_DESC
{
	THREAD_ID ptid;				/*进程线程ID*/
	DWORD data[MSG_DATA_LEN];	/*消息数据*/
	struct _MESSAGE_DESC *nxt;	/*后一项*/
}MESSAGE_DESC;	/*消息描述符*/

/*初始化消息管理*/
long InitMsg();

/*分配消息结构*/
MESSAGE_DESC *AllocMsg();

/*释放消息结构*/
void FreeMsg(MESSAGE_DESC *msg);

/*清除线程的消息队列*/
void FreeAllMsg();

/*发送消息*/
long SendMsg(MESSAGE_DESC *msg);

/*接收消息*/
long RecvMsg(MESSAGE_DESC **msg, DWORD cs);

/*阻塞并等待指定线程的消息*/
long WaitThedMsg(MESSAGE_DESC **msg, THREAD_ID ptid, DWORD cs);

#endif

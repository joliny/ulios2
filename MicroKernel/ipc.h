/*	ipc.h for ulios
	���ߣ�����
	���ܣ����̼�ͨѶ����
	����޸����ڣ�2009-06-22
*/

#ifndef _IPC_H_
#define _IPC_H_

#include "ulidef.h"

#define MSGMT_LEN			0x0800		/*��Ϣ�������*/

/*��Ϣ����,ֻռ����Ϣ�ṹdata[0]�ĸ�16λ*/
#define MSG_ATTR_ISR		0x00010000	/*�쳣�ź���Ϣ*/
#define MSG_ATTR_IRQ		0x00020000	/*�ж��ź���Ϣ*/
#define MSG_ATTR_SYS		0x00030000	/*ϵͳ��Ϣ*/
#define MSG_ATTR_READ		0x00040000	/*��������Ϣ,ҳӳ�䷽ʽ��*/
#define MSG_ATTR_WRITE		0x00050000	/*д������Ϣ,ҳӳ�䷽ʽд*/
#define MSG_ATTR_PROC		0x01000000	/*�����Զ�����Ϣ��Сֵ*/
#define THED_MSG_LEN		16			/*�߳�����Ϣ������*/

typedef struct _MESSAGE_DESC
{
	THREAD_ID ptid;				/*�����߳�ID*/
	DWORD data[4];				/*��Ϣ����*/
	struct _MESSAGE_DESC *nxt;	/*��һ��*/
}MESSAGE_DESC;	/*��Ϣ������*/

/*��ʼ����Ϣ����*/
long InitMsg();

/*������Ϣ�ṹ*/
MESSAGE_DESC *AllocMsg();

/*�ͷ���Ϣ�ṹ*/
void FreeMsg(MESSAGE_DESC *msg);

/*������Ϣ*/
long SendMsg(MESSAGE_DESC *msg);

/*������Ϣ*/
long RecvMsg(MESSAGE_DESC **msg);

/*�������ȴ���Ϣ*/
void WaitMsg(MESSAGE_DESC **msg);

/*����̵߳���Ϣ����*/
void FreeAllMsg();

#endif

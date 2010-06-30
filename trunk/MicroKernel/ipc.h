/*	ipc.h for ulios
	���ߣ�����
	���ܣ����̼�ͨѶ����
	����޸����ڣ�2009-06-22
*/

#ifndef _IPC_H_
#define _IPC_H_

#include "ulidef.h"

#define MSGMT_LEN			0x1000		/*��Ϣ�������*/
/*��Ϣ����,ֻռ����Ϣ�ṹdata[0]�ĸ�16λ*/
#define MSG_ATTR_ISR		0x00010000	/*Ӳ��������Ϣ*/
#define MSG_ATTR_IRQ		0x00020000	/*Ӳ���ж���Ϣ*/
#define MSG_ATTR_THEDEXIT	0x00030000	/*�߳��˳���Ϣ*/
#define MSG_ATTR_PROCEXIT	0x00040000	/*�����˳���Ϣ*/
#define MSG_ATTR_EXCEP		0x00050000	/*�쳣�˳���Ϣ*/
#define MSG_ATTR_MAP		0x00060000	/*ҳӳ����Ϣ*/
#define MSG_ATTR_UNMAP		0x00070000	/*���ҳӳ����Ϣ*/
#define MSG_ATTR_CNLMAP		0x00080000	/*ȡ��ҳӳ����Ϣ*/

#define MSG_ATTR_USER		0x01000000	/*�û��Զ�����Ϣ��Сֵ*/
#define THED_MSG_LEN		64			/*�߳�����Ϣ������*/
#define MSG_DATA_LEN		8			/*��Ϣ������˫����*/

typedef struct _MESSAGE_DESC
{
	THREAD_ID ptid;				/*�����߳�ID*/
	DWORD data[MSG_DATA_LEN];	/*��Ϣ����*/
	struct _MESSAGE_DESC *nxt;	/*��һ��*/
}MESSAGE_DESC;	/*��Ϣ������*/

/*��ʼ����Ϣ����*/
long InitMsg();

/*������Ϣ�ṹ*/
MESSAGE_DESC *AllocMsg();

/*�ͷ���Ϣ�ṹ*/
void FreeMsg(MESSAGE_DESC *msg);

/*����̵߳���Ϣ����*/
void FreeAllMsg();

/*������Ϣ*/
long SendMsg(MESSAGE_DESC *msg);

/*������Ϣ*/
long RecvMsg(MESSAGE_DESC **msg, DWORD cs);

/*�������ȴ�ָ���̵߳���Ϣ*/
long WaitThedMsg(MESSAGE_DESC **msg, THREAD_ID ptid, DWORD cs);

#endif

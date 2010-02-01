/*	task.h for ulios
	���ߣ�����
	���ܣ����������
	����޸����ڣ�2009-06-25
*/

#ifndef _TASK_H_
#define _TASK_H_

#include "ulidef.h"

#define INVWID				0xFFFF	/*��Ч�����߳�ID*/

#define BLK_PTID_SPECIAL	0xFFFF0000	/*����PTID���ɰ�*/
#define BLK_PTID_FREEPG		0x0001	/*0:���ͷ�����ҳ1:�ͷ�����ҳ*/
typedef struct _BLK_DESC
{
	void *addr;					/*��ʼ��ַ*/
	DWORD siz;					/*�ֽ���,0��ʾ����*/
	DWORD ptid;					/*THREAD_ID:��ӳ���߳�ID,����Ϊ����PTID*/
}BLK_DESC;	/*���Ե�ַ��������*/

#define THED_ATTR_SLEEP		0x0001	/*0:����1:����*/
#define THED_ATTR_WAITTIME	0x0002	/*0:���ȴ�ʱ��1:�ȴ�ʱ��*/
#define THED_ATTR_APPS		0x0004	/*0:ϵͳ����̬1:Ӧ�ó���̬*/
#define THED_ATTR_DEL		0x0008	/*0:����״̬1:���ڱ�ɾ��*/
#define THED_ATTR_KILLED	0x0010	/*0:����״̬1:��ɱ����־*/
#define KSTK_LEN			475		/*�ں˶�ջ˫����,��֤�߳̽ṹ��С2KB*/
#define THEDSTK_SIZ			0x00100000	/*�߳�Ĭ�϶�ջ��С*/
typedef struct _THREAD_DESC
{
	struct _THREAD_DESC	*pre, *nxt;	/*ǰ��ָ��*/
	WORD par, attr;				/*���߳�ID,����*/
	THREAD_ID id;				/*�߳�ID*/

	MESSAGE_DESC *msg, *lst;	/*��Ϣ����,���һ����Ϣָ��*/
	DWORD MsgCou;				/*������Ϣ��*/

	DWORD WakeupClock;			/*��ʱ����ʱ��*/

	void *ustk;					/*�û���ջ��ַ*/
	DWORD UstkSiz;				/*�û���ջ��С*/
	I387 *i387;					/*��ѧЭ�������Ĵ����ṹ*/
	TSS tss;					/*����״̬�ṹ*/
	DWORD kstk[KSTK_LEN];		/*�ں�ջ*/
}THREAD_DESC;	/*�߳̽ṹ*/

#define PROC_ATTR_APPS		0x0001	/*0:��������1:Ӧ�ý���*/
#define PROC_ATTR_DEL		0x0002	/*0:����״̬1:���ڱ�ɾ��*/
#define TMT_LEN				0x100	/*�������̱߳���*/
#define UBLKT_LEN			0x100	/*��ַ��������*/
#define UFDMT_LEN			32		/*�û������������������*/
typedef struct _PROCESS_DESC
{
	struct _PROCESS_DESC *pre, *nxt;	/*ǰ��ָ��*/
	WORD par, attr;				/*������ID,����*/

	DWORD MemSiz;				/*��ռ���ڴ��ֽ���*/

	EXEC_DESC *exec;			/*��ִ����ָ��*/

	THREAD_DESC *tmt[TMT_LEN];	/*�̹߳����*/
	THREAD_DESC **FstTmd;		/*�׸����߳���ָ��*/
	THREAD_DESC **EndTmd;		/*ĩ���ǿ��߳���ָ��*/
	THREAD_DESC *CurTmd;		/*��ǰ�߳�ָ��*/
	DWORD TmdCou;				/*�����߳�����*/

	BLK_DESC ublkt[UBLKT_LEN];	/*��ַ������*/
	BLK_DESC *FstUBlk;			/*�׸��յ�ַ��������ָ��*/
	BLK_DESC *EndUBlk;			/*ĩ���ǿյ�ַ��������ָ��*/
	FREE_BLK_DESC ufdmt[UFDMT_LEN];	/*�û����������������*/
	volatile DWORD Ufdmt_l;		/*�û�������������*/
	volatile DWORD Page_l;		/*��ҳ������*/
}PROCESS_DESC;	/*���̽ṹ*/

/*��ʼ�����̹����*/
void InitPMT();

/*��ʼ���ں˽���*/
void InitKnlProc();

/*��ʱcs����*/
void SleepCs(DWORD cs);

/*�����߳�,�̱߳�����Ч������,���������ں�����*/
void wakeup(THREAD_DESC *thed);

/*�����߳�,�̱߳�����Ч�Ҿ���,�����������ں�����*/
void sleep(BOOL isWaitTime);

/*�����߳�*/
long CreateThed(const DWORD *args, THREAD_ID *ptid);

/*ɾ���߳�*/
void DeleteThed();

/*ɱ���߳�*/
long KillThed(WORD ThedID);

/*��������*/
long CreateProc(DWORD attr, const DWORD *args, THREAD_ID *ptid);

/*ɾ������*/
void DeleteProc();

/*ɱ������*/
long KillProc(WORD ProcID);

/*�����û���ַ��*/
void *LockAllocUBlk(PROCESS_DESC *proc, DWORD siz, DWORD ptid);

/*�����û���ַ��*/
long LockFreeUBlk(PROCESS_DESC *proc, void *addr);

/*�����û�����������(������ʽ)*/
static inline void *LockAllocUFData(PROCESS_DESC *proc, DWORD siz)
{
	void *res;
	lock(&proc->Ufdmt_l);
	res = alloc(proc->ufdmt, siz);
	ulock(&proc->Ufdmt_l);
	return res;
}

/*�����û�����������(������ʽ)*/
static inline void LockFreeUFData(PROCESS_DESC *proc, void *addr, DWORD siz)
{
	lock(&proc->Ufdmt_l);
	free(proc->ufdmt, addr, siz);
	ulock(&proc->Ufdmt_l);
}

#endif

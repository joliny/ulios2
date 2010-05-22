/*	task.h for ulios
	���ߣ�����
	���ܣ������������
	����޸����ڣ�2009-06-25
*/

#ifndef _TASK_H_
#define _TASK_H_

#include "ulidef.h"

typedef struct _BLK_DESC
{
	void *addr;					/*��ʼ��ַ*/
	DWORD siz;					/*�ֽ���,0��ʾ����*/
}BLK_DESC;	/*���Ե�ַ��������*/

#define THED_ATTR_SLEEP		0x0001	/*0:����״̬1:����״̬*/
#define THED_ATTR_WAITTIME	0x0002	/*0:���ȴ�ʱ��1:�ȴ�ʱ��*/
#define THED_ATTR_APPS		0x0004	/*0:ϵͳ����̬1:Ӧ�ó���̬*/
#define THED_ATTR_DEL		0x0008	/*0:����״̬1:���ڱ�ɾ��*/
#define THED_ATTR_KILLED	0x0010	/*0:����״̬1:��ɱ����־*/
#define KSTK_LEN			474		/*�ں˶�ջ˫����*/
#define THEDSTK_SIZ			0x00100000	/*�߳�Ĭ�϶�ջ��С*/
typedef struct _THREAD_DESC
{
	struct _THREAD_DESC	*pre, *nxt;	/*ǰ��ָ��*/
	WORD par, attr;				/*���߳�ID,����*/
	THREAD_ID id;				/*�߳�ID*/

	MESSAGE_DESC *msg, *lst;	/*��Ϣ����,���һ����Ϣָ��*/
	DWORD MsgCou;				/*������Ϣ��*/
	THREAD_ID WaitId;			/*�ȴ��߳�ID*/
	DWORD WakeupClock;			/*��ʱ����ʱ��*/

	void *ustk;					/*�û���ջ��ַ*/
	DWORD UstkSiz;				/*�û���ջ��С*/
	I387 *i387;					/*��ѧЭ�������Ĵ����ṹ*/
	TSS tss;					/*����״̬�ṹ*/
	DWORD kstk[KSTK_LEN];		/*�ں�ջ*/
}THREAD_DESC;	/*�߳̽ṹ*/

#define PROC_ATTR_APPS		0x0001	/*0:��������1:Ӧ�ý���*/
#define PROC_ATTR_DEL		0x0002	/*0:����״̬1:���ڱ�ɾ��*/
#define TMT_LEN				0x100	/*�������̱߳�����*/
#define UBLKT_LEN			0x400	/*��ַ�����������*/
#define UFDMT_LEN			64		/*�û���������������������*/
typedef struct _PROCESS_DESC
{
	struct _PROCESS_DESC *pre, *nxt;	/*ǰ��ָ��*/
	WORD par, attr;				/*������ID,����*/

	DWORD MemSiz;				/*��ռ���ڴ��ֽ���*/
	EXEC_DESC *exec;			/*��ִ����ָ��*/
	MAPBLK_DESC *map;			/*ӳ��ṹ����*/
	MAPBLK_DESC *map2;			/*��ӳ��ṹ����*/
	DWORD MapCou;				/*����ӳ��ṹ��*/
	volatile DWORD Map_l;		/*ӳ�������*/

	THREAD_DESC *tmt[TMT_LEN];	/*�̹߳�����*/
	THREAD_DESC **FstTmd;		/*�׸����߳���ָ��*/
	THREAD_DESC **EndTmd;		/*ĩ���ǿ��߳���ָ��*/
	THREAD_DESC *CurTmd;		/*��ǰ�߳�ָ��*/
	DWORD TmdCou;				/*�����߳�����*/

	BLK_DESC ublkt[UBLKT_LEN];	/*��ַ�������*/
	BLK_DESC *FstUBlk;			/*�׸��յ�ַ���������ָ��*/
	BLK_DESC *EndUBlk;			/*ĩ���ǿյ�ַ���������ָ��*/
	FREE_BLK_DESC ufdmt[UFDMT_LEN];	/*�û�����������������*/
	volatile DWORD Ufdmt_l;		/*�û�������������*/
	volatile DWORD Page_l;		/*��ҳ������*/
}PROCESS_DESC;	/*���̽ṹ*/

/*��ʼ�����̹�����*/
void InitPMT();

/*��ʼ���ں˽���*/
void InitKnlProc();

/*�����߳�*/
void wakeup(THREAD_DESC *thed);

/*�����߳�*/
void sleep(DWORD cs);

/*�����߳�*/
long CreateThed(const DWORD *argv, THREAD_ID *ptid);

/*ɾ���߳�*/
void DeleteThed();

/*ɱ���߳�*/
long KillThed(WORD ThedID);

/*��������*/
long CreateProc(const DWORD *argv, THREAD_ID *ptid);

/*ɾ������*/
void DeleteProc();

/*ɱ������*/
long KillProc(WORD ProcID);

/*�����û���ַ��*/
BLK_DESC *AllocUBlk(PROCESS_DESC *proc, DWORD siz);

/*�����û���ַ��*/
BLK_DESC *FindUBlk(PROCESS_DESC *proc, void *addr);

/*�����û���ַ��*/
void FreeUBlk(PROCESS_DESC *proc, BLK_DESC *blk);

/*�����߳�(���жϷ�ʽ)*/
static inline void CliWakeup(THREAD_DESC *thed)
{
	cli();
	wakeup(thed);
	sti();
}

/*�����߳�(���жϷ�ʽ)*/
static inline void CliSleep(DWORD cs)
{
	cli();
	sleep(cs);
	sti();
}

/*�����û���ַ��(������ʽ)*/
static inline BLK_DESC *LockAllocUBlk(PROCESS_DESC *proc, DWORD siz)
{
	BLK_DESC *res;
	lock(&proc->Ufdmt_l);
	res = AllocUBlk(proc, siz);
	ulock(&proc->Ufdmt_l);
	return res;
}

/*�����û���ַ��(������ʽ)*/
static inline BLK_DESC *LockFindUBlk(PROCESS_DESC *proc, void *addr)
{
	BLK_DESC *res;
	lock(&proc->Ufdmt_l);
	res = FindUBlk(proc, addr);
	ulock(&proc->Ufdmt_l);
	return res;
}

/*�����û���ַ��(������ʽ)*/
static inline void LockFreeUBlk(PROCESS_DESC *proc, BLK_DESC *blk)
{
	lock(&proc->Ufdmt_l);
	FreeUBlk(proc, blk);
	ulock(&proc->Ufdmt_l);
}

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
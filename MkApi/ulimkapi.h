/*	ulimkapi.h for ulios program
	���ߣ�����
	���ܣ�ulios΢�ں�API�ӿڶ��壬����Ӧ�ó�����Ҫ�������ļ�
	����޸����ڣ�2009-07-28
*/

#ifndef _ULIMKAPI_H_
#define _ULIMKAPI_H_

/**********��������**********/
typedef unsigned char		BYTE;	/*8λ*/
typedef unsigned short		WORD;	/*16λ*/
typedef unsigned long		DWORD;	/*32λ*/
typedef unsigned long		BOOL;

typedef struct _THREAD_ID
{
	WORD ProcID;
	WORD ThedID;
}THREAD_ID;	/*�����߳�ID*/

/**********����**********/
#define TRUE	1
#define FALSE	0
#define NULL	((void *)0)
#define INVALID	(~0)

/**********�������**********/
#define NO_ERROR					0	/*�޴�*/

#define ERROR_WRONG_APIN			-1	/*�����ϵͳ���ú�*/
#define ERROR_WRONG_THEDID			-2	/*������߳�ID*/
#define ERROR_WRONG_PROCID			-3	/*����Ľ���ID*/
#define ERROR_WRONG_KPTN			-4	/*������ں˶˿ں�*/
#define ERROR_WRONG_IRQN			-5	/*�����IRQ��*/
#define ERROR_WRONG_APPMSG			-6	/*�����Ӧ�ó�����Ϣ*/

#define ERROR_HAVENO_KMEM			-7	/*û���ں��ڴ�*/
#define ERROR_HAVENO_PMEM			-8	/*û�������ڴ�*/
#define ERROR_HAVENO_THEDID			-9	/*û���̹߳���ڵ�*/
#define ERROR_HAVENO_PROCID			-10	/*û�н��̹���ڵ�*/
#define ERROR_HAVENO_EXECID			-11	/*û�п�ִ����������*/
#define ERROR_HAVENO_MSGDESC		-12	/*û����Ϣ������*/
#define ERROR_HAVENO_LINEADDR		-13	/*û�����Ե�ַ�ռ�*/

#define ERROR_KPT_ISENABLED			-14	/*�ں˶˿��Ѿ���ע��*/
#define ERROR_KPT_ISDISABLED		-15	/*�ں˶˿�û�б�ע��*/
#define ERROR_KPT_WRONG_CURPROC		-16	/*��ǰ�����޷��Ķ��ں˶˿�*/
#define ERROR_IRQ_ISENABLED			-17	/*IRQ�Ѿ�����*/
#define ERROR_IRQ_ISDISABLED		-18	/*IRQ�Ѿ��ر�*/
#define ERROR_IRQ_WRONG_CURPROC		-19	/*��ǰ�߳��޷��Ķ�IRQ*/

#define ERROR_NOT_DRIVER			-20	/*�Ƿ�ִ������API*/
#define ERROR_INVALID_MAPADDR		-21	/*�Ƿ���ӳ���ַ*/
#define ERROR_INVALID_MAPSIZE		-22	/*�Ƿ���ӳ���С*/

#define MSG_ATTR_ISR		0x00010000	/*�쳣�ź���Ϣ*/
#define MSG_ATTR_IRQ		0x00020000	/*�ж��ź���Ϣ*/
#define MSG_ATTR_SYS		0x00030000	/*ϵͳ��Ϣ*/
#define MSG_ATTR_READ		0x00040000	/*��������Ϣ,ҳӳ�䷽ʽ��*/
#define MSG_ATTR_WRITE		0x00050000	/*д������Ϣ,ҳӳ�䷽ʽд*/
#define MSG_ATTR_PROC		0x01000000	/*�����Զ�����Ϣ��Сֵ*/

/**********��������**********/

/*32λ�ڴ�����*/
static inline void memset32(void *dest, DWORD d, DWORD n)
{
	void *_dest;
	DWORD _n;
	__asm__ __volatile__("cld;rep stosl": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(d), "1"(n): "flags", "memory");
}

/*32λ�ڴ渴��*/
static inline void memcpy32(void *dest, const void *src, DWORD n)
{
	void *_dest;
	const void *_src;
	DWORD _n;
	__asm__ __volatile__("cld;rep movsl": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

/*���ж�*/
static inline void cli()
{
	__asm__("cli");
}

/*���ж�*/
static inline void sti()
{
	__asm__("sti");
}

/*�˿�����ֽ�*/
static inline void outb(WORD port, BYTE b)
{
	__asm__ __volatile__("outb %1, %0":: "Nd"(port), "a"(b));
}

/*�˿������ֽ�*/
static inline BYTE inb(WORD port)
{
	register BYTE b;
	__asm__ __volatile__("inb %1, %0": "=a"(b): "Nd"(port));
	return b;
}

/**********ϵͳ���ýӿ�**********/

/*�������*/
static inline long KPrintf(char *str, DWORD num)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x000000), "b"(num), "S"(str));
	return res;
}

/*�������������*/
static inline long KGiveUp()
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x010000));
	return res;
}

/*˯��*/
static inline long KSleep(DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x020000), "b"(CentiSeconds));
	return res;
}

/*�����߳�*/
static inline long KCreateThread(void (*ThreadProc)(), DWORD StackSize, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x030000), "1"(ThreadProc), "c"(StackSize));
	return res;
}

/*�˳��߳�*/
static inline void KExitThread(DWORD ExitCode)
{
	__asm__ __volatile__("int $0xF0":: "a"(0x040000), "b"(ExitCode));
}

/*ɱ���߳�*/
static inline long KKillThread(DWORD ThreadID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x050000), "b"(ThreadID));
	return res;
}

/*��������*/
static inline long KCreateProcess(DWORD attr, char *command, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x060000), "1"(attr), "S"(command));
	return res;
}

/*�˳�����*/
static inline void KExitProcess(DWORD ExitCode)
{
	__asm__ __volatile__("int $0xF0":: "a"(0x070000), "b"(ExitCode));
}

/*ɱ������*/
static inline long KKillProcess(DWORD ProcessID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x080000), "b"(ProcessID));
	return res;
}

/*ע���ں˶˿ڶ�Ӧ�߳�*/
static inline long KRegKnlPort(DWORD KnlPort)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x090000), "b"(KnlPort));
	return res;
}

/*ע���ں˶˿ڶ�Ӧ�߳�*/
static inline long KUnregKnlPort(DWORD KnlPort)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0A0000), "b"(KnlPort));
	return res;
}

/*ȡ���ں˶˿ڶ�Ӧ�߳�*/
static inline long KGetKpToThed(DWORD KnlPort, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0B0000), "1"(KnlPort));
	return res;
}

/*ע��IRQ�źŵ���Ӧ�߳�*/
static inline long KRegIrq(DWORD irq)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0C0000), "b"(irq));
	return res;
}

/*ע��IRQ�źŵ���Ӧ�߳�*/
static inline long KUnregIrq(DWORD irq)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0D0000), "b"(irq));
	return res;
}

/*������Ϣ*/
static inline long KSendMsg(THREAD_ID ptid, DWORD c, DWORD d, DWORD src, DWORD dest)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0E0000), "b"(ptid), "c"(c), "d"(d), "S"(src), "D"(dest));
	return res;
}

/*������Ϣ*/
static inline long KRecvMsg(THREAD_ID *ptid, DWORD *c, DWORD *d, DWORD *src, DWORD *dest)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid), "=c"(*c), "=d"(*d), "=S"(*src), "=D"(*dest): "0"(0x0F0000));
	return res;
}

/*�������ȴ���Ϣ*/
static inline long KWaitMsg(DWORD CentiSeconds, THREAD_ID *ptid, DWORD *c, DWORD *d, DWORD *src, DWORD *dest)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid), "=c"(*c), "=d"(*d), "=S"(*src), "=D"(*dest): "0"(0x100000), "1"(CentiSeconds));
	return res;
}

/*ӳ�������ַ*/
static inline long KMapPhyAddr(DWORD *addr, DWORD PhyAddr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*addr): "0"(0x110000), "1"(PhyAddr), "c"(siz));
	return res;
}

/*ӳ���û���ַ*/
static inline long KMapUserAddr(DWORD *addr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*addr): "0"(0x120000), "1"(siz));
	return res;
}

/*�����û���ַ��*/
static inline long KFreeAddr(DWORD addr)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x130000), "b"(addr));
	return res;
}

#endif

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
typedef unsigned long long	QWORD;	/*64λ*/
typedef long long			SQWORD;	/*�з���64λ*/

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
#define ERROR_INVALID_ADDR			-21	/*��Ч�ĵ�ַ*/
#define ERROR_INVALID_MAPADDR		-22	/*�Ƿ���ӳ���ַ*/
#define ERROR_INVALID_MAPSIZE		-23	/*�Ƿ���ӳ���С*/

#define ERROR_OUT_OF_TIME			-24	/*��ʱ����*/
#define ERROR_PROC_EXCEP			-25	/*�����쳣*/
#define ERROR_THED_KILLED			-26	/*�̱߳�ɱ��*/

#define MSG_DATA_LEN		8			/*��Ϣ������˫����*/

#define MSG_ATTR_ISR		0x00010000	/*Ӳ��������Ϣ*/
#define MSG_ATTR_IRQ		0x00020000	/*Ӳ���ж���Ϣ*/
#define MSG_ATTR_THEDEXIT	0x00030000	/*�߳��˳���Ϣ*/
#define MSG_ATTR_PROCEXIT	0x00040000	/*�����˳���Ϣ*/
#define MSG_ATTR_EXCEP		0x00050000	/*�쳣�˳���Ϣ*/
#define MSG_ATTR_MAP		0x00060000	/*ҳӳ����Ϣ*/
#define MSG_ATTR_UNMAP		0x00070000	/*���ҳӳ����Ϣ*/
#define MSG_ATTR_CNLMAP		0x00080000	/*ȡ��ҳӳ����Ϣ*/

#define MSG_ATTR_USER		0x01000000	/*�û��Զ�����Ϣ��Сֵ*/
#define MSG_ATTR_EXITREQ	0x01010000	/*����:�˳�������Ϣ*/

/**********��������**********/

/*�ڴ�����*/
static inline void memset8(void *dest, BYTE b, DWORD n)
{
	void *_dest;
	DWORD _n;
	__asm__ __volatile__("cld;rep stosb": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(b), "1"(n): "flags", "memory");
}

/*�ڴ渴��*/
static inline void memcpy8(void *dest, const void *src, DWORD n)
{
	void *_dest;
	const void *_src;
	DWORD _n;
	__asm__ __volatile__("cld;rep movsb": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

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

/*�ַ�������*/
static inline DWORD strlen(const char *str)
{
	DWORD d0;
	register DWORD _res;
	__asm__ __volatile__
	(
		"cld\n"
		"repne\n"
		"scasb\n"
		"notl %0\n"
		"decl %0"
		: "=c"(_res), "=&D"(d0): "1"(str), "a"(0), "0"(0xFFFFFFFFU): "flags"
	);
	return _res;
}

/*�ַ�������*/
static inline char *strcpy(char *dest, const char *src)
{
	char *_dest;
	const char *_src;
	__asm__ __volatile__
	(
		"cld\n"
		"1:\tlodsb\n"
		"stosb\n"
		"testb %%al, %%al\n"
		"jne 1b"
		: "=&D"(_dest), "=&S"(_src): "0"(dest), "1"(src): "flags", "al", "memory"
	);
	return _dest;
}

/*�ַ�����������*/
static inline void strncpy(char *dest, const char *src, DWORD n)
{
	char *_dest;
	const char *_src;
	DWORD _n;
	__asm__ __volatile__
	(
		"cld\n"
		"1:\tdecl %2\n"
		"js 2f\n"
		"lodsb\n"
		"stosb\n"
		"testb %%al, %%al\n"
		"jne 1b\n"
		"rep stosb\n"
		"2:"
		: "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "al", "memory"
	);
}

/*���ж�(����ר��)*/
static inline void cli()
{
	__asm__("cli");
}

/*���ж�(����ר��)*/
static inline void sti()
{
	__asm__("sti");
}

/*�˿�����ֽ�(����ר��)*/
static inline void outb(WORD port, BYTE b)
{
	__asm__ __volatile__("outb %1, %w0":: "d"(port), "a"(b));
}

/*�˿������ֽ�(����ר��)*/
static inline BYTE inb(WORD port)
{
	register BYTE b;
	__asm__ __volatile__("inb %w1, %0": "=a"(b): "d"(port));
	return b;
}

/**********ϵͳ���ýӿ�**********/

/*ȡ���߳�ID*/
static inline long KGetPtid(THREAD_ID *ptid, DWORD *ParThreadID, DWORD *ParProcessID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid), "=c"(*ParThreadID), "=d"(*ParProcessID): "0"(0x000000));
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
static inline long KCreateThread(void (*ThreadProc)(void *data), DWORD StackSize, void *data, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x030000), "1"(ThreadProc), "c"(StackSize), "d"(data));
	return res;
}

/*�˳��߳�*/
static inline void KExitThread(long ExitCode)
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
static inline long KCreateProcess(DWORD attr, const char *exec, const char *args, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x060000), "1"(attr), "D"(exec), "S"(args));
	return res;
}

/*�˳�����*/
static inline void KExitProcess(long ExitCode)
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
static inline long KGetKptThed(DWORD KnlPort, THREAD_ID *ptid)
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
static inline long KSendMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0E0000), "1"(*ptid), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*������Ϣ*/
static inline long KRecvMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0F0000), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*����ָ�����̵���Ϣ*/
static inline long KRecvProcMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x100000), "1"(*ptid), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*ӳ�������ַ*/
static inline long KMapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=S"(*addr): "0"(0x110000), "b"(PhyAddr), "c"(siz): "memory");
	return res;
}

/*ӳ���û���ַ*/
static inline long KMapUserAddr(void **addr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=S"(*addr): "0"(0x120000), "c"(siz));
	return res;
}

/*�����û���ַ��*/
static inline long KFreeAddr(void *addr)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x130000), "S"(addr): "memory");
	return res;
}

/*ӳ����̵�ַ��ȡ*/
static inline long KReadProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x140000), "1"(*ptid), "c"(siz), "d"(CentiSeconds), "S"(data), "D"(addr): "memory");
	return res;
}

/*ӳ����̵�ַд��*/
static inline long KWriteProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x150000), "1"(*ptid), "c"(siz), "d"(CentiSeconds), "S"(data), "D"(addr): "memory");
	return res;
}

/*����ӳ����̵�ַ*/
static inline long KUnmapProcAddr(void *addr, const DWORD data[MSG_DATA_LEN - 2])
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x160000), "S"(data), "D"(addr): "memory");
	return res;
}

/*ȡ��ӳ����̵�ַ*/
static inline long KCnlmapProcAddr(void *addr, const DWORD data[MSG_DATA_LEN - 2])
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x170000), "S"(data), "D"(addr): "memory");
	return res;
}

/*ȡ�ÿ���������ʱ��*/
static inline long KGetClock(DWORD *clock)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*clock): "0"(0x180000));
	return res;
}

/**********����**********/

/*����������(����ר��)*/
static inline void lock(volatile DWORD *l)
{
	cli();
	while (*l)
		KGiveUp();
	*l = TRUE;
	sti();
}

/*����������(����ר��)*/
static inline void ulock(volatile DWORD *l)
{
	*l = FALSE;
}

/*�����˳�����*/
static inline long SendExitReq(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_EXITREQ;
	return KSendMsg(&ptid, data, 0);
}

#endif

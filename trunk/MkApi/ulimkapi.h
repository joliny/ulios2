/*	ulimkapi.h for ulios program
	作者：孙亮
	功能：ulios微内核API接口定义，开发应用程序需要包含此文件
	最后修改日期：2009-07-28
*/

#ifndef _ULIMKAPI_H_
#define _ULIMKAPI_H_

/**********数据类型**********/
typedef unsigned char		BYTE;	/*8位*/
typedef unsigned short		WORD;	/*16位*/
typedef unsigned long		DWORD;	/*32位*/
typedef unsigned long		BOOL;
typedef unsigned long long	QWORD;	/*64位*/
typedef long long			SQWORD;	/*有符号64位*/

typedef struct _THREAD_ID
{
	WORD ProcID;
	WORD ThedID;
}THREAD_ID;	/*进程线程ID*/

/**********常量**********/
#define TRUE	1
#define FALSE	0
#define NULL	((void *)0)
#define INVALID	(~0)

/**********错误代码**********/
#define NO_ERROR					0	/*无错*/

#define ERROR_WRONG_APIN			-1	/*错误的系统调用号*/
#define ERROR_WRONG_THEDID			-2	/*错误的线程ID*/
#define ERROR_WRONG_PROCID			-3	/*错误的进程ID*/
#define ERROR_WRONG_KPTN			-4	/*错误的内核端口号*/
#define ERROR_WRONG_IRQN			-5	/*错误的IRQ号*/
#define ERROR_WRONG_APPMSG			-6	/*错误的应用程序消息*/

#define ERROR_HAVENO_KMEM			-7	/*没有内核内存*/
#define ERROR_HAVENO_PMEM			-8	/*没有物理内存*/
#define ERROR_HAVENO_THEDID			-9	/*没有线程管理节点*/
#define ERROR_HAVENO_PROCID			-10	/*没有进程管理节点*/
#define ERROR_HAVENO_EXECID			-11	/*没有可执行体描述符*/
#define ERROR_HAVENO_MSGDESC		-12	/*没有消息描述符*/
#define ERROR_HAVENO_LINEADDR		-13	/*没有线性地址空间*/

#define ERROR_KPT_ISENABLED			-14	/*内核端口已经被注册*/
#define ERROR_KPT_ISDISABLED		-15	/*内核端口没有被注册*/
#define ERROR_KPT_WRONG_CURPROC		-16	/*当前进程无法改动内核端口*/
#define ERROR_IRQ_ISENABLED			-17	/*IRQ已经开启*/
#define ERROR_IRQ_ISDISABLED		-18	/*IRQ已经关闭*/
#define ERROR_IRQ_WRONG_CURPROC		-19	/*当前线程无法改动IRQ*/

#define ERROR_NOT_DRIVER			-20	/*非法执行驱动API*/
#define ERROR_INVALID_ADDR			-21	/*无效的地址*/
#define ERROR_INVALID_MAPADDR		-22	/*非法的映射地址*/
#define ERROR_INVALID_MAPSIZE		-23	/*非法的映射大小*/

#define ERROR_OUT_OF_TIME			-24	/*超时错误*/
#define ERROR_PROC_EXCEP			-25	/*程序异常*/
#define ERROR_THED_KILLED			-26	/*线程被杀死*/

#define MSG_DATA_LEN		8			/*消息数据总双字数*/

#define MSG_ATTR_ISR		0x00010000	/*硬件陷阱消息*/
#define MSG_ATTR_IRQ		0x00020000	/*硬件中断消息*/
#define MSG_ATTR_THEDEXIT	0x00030000	/*线程退出消息*/
#define MSG_ATTR_PROCEXIT	0x00040000	/*进程退出消息*/
#define MSG_ATTR_EXCEP		0x00050000	/*异常退出消息*/
#define MSG_ATTR_MAP		0x00060000	/*页映射消息*/
#define MSG_ATTR_UNMAP		0x00070000	/*解除页映射消息*/
#define MSG_ATTR_CNLMAP		0x00080000	/*取消页映射消息*/

#define MSG_ATTR_USER		0x01000000	/*用户自定义消息最小值*/
#define MSG_ATTR_EXITREQ	0x01010000	/*建议:退出请求消息*/

/**********基本操作**********/

/*内存设置*/
static inline void memset8(void *dest, BYTE b, DWORD n)
{
	void *_dest;
	DWORD _n;
	__asm__ __volatile__("cld;rep stosb": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(b), "1"(n): "flags", "memory");
}

/*内存复制*/
static inline void memcpy8(void *dest, const void *src, DWORD n)
{
	void *_dest;
	const void *_src;
	DWORD _n;
	__asm__ __volatile__("cld;rep movsb": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

/*32位内存设置*/
static inline void memset32(void *dest, DWORD d, DWORD n)
{
	void *_dest;
	DWORD _n;
	__asm__ __volatile__("cld;rep stosl": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(d), "1"(n): "flags", "memory");
}

/*32位内存复制*/
static inline void memcpy32(void *dest, const void *src, DWORD n)
{
	void *_dest;
	const void *_src;
	DWORD _n;
	__asm__ __volatile__("cld;rep movsl": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

/*字符串长度*/
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

/*字符串复制*/
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

/*字符串限量复制*/
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

/*关中断(驱动专用)*/
static inline void cli()
{
	__asm__("cli");
}

/*开中断(驱动专用)*/
static inline void sti()
{
	__asm__("sti");
}

/*端口输出字节(驱动专用)*/
static inline void outb(WORD port, BYTE b)
{
	__asm__ __volatile__("outb %1, %w0":: "d"(port), "a"(b));
}

/*端口输入字节(驱动专用)*/
static inline BYTE inb(WORD port)
{
	register BYTE b;
	__asm__ __volatile__("inb %w1, %0": "=a"(b): "d"(port));
	return b;
}

/**********系统调用接口**********/

/*取得线程ID*/
static inline long KGetPtid(THREAD_ID *ptid, DWORD *ParThreadID, DWORD *ParProcessID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid), "=c"(*ParThreadID), "=d"(*ParProcessID): "0"(0x000000));
	return res;
}

/*主动放弃处理机*/
static inline long KGiveUp()
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x010000));
	return res;
}

/*睡眠*/
static inline long KSleep(DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x020000), "b"(CentiSeconds));
	return res;
}

/*创建线程*/
static inline long KCreateThread(void (*ThreadProc)(void *data), DWORD StackSize, void *data, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x030000), "1"(ThreadProc), "c"(StackSize), "d"(data));
	return res;
}

/*退出线程*/
static inline void KExitThread(long ExitCode)
{
	__asm__ __volatile__("int $0xF0":: "a"(0x040000), "b"(ExitCode));
}

/*杀死线程*/
static inline long KKillThread(DWORD ThreadID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x050000), "b"(ThreadID));
	return res;
}

/*创建进程*/
static inline long KCreateProcess(DWORD attr, const char *exec, const char *args, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x060000), "1"(attr), "D"(exec), "S"(args));
	return res;
}

/*退出进程*/
static inline void KExitProcess(long ExitCode)
{
	__asm__ __volatile__("int $0xF0":: "a"(0x070000), "b"(ExitCode));
}

/*杀死进程*/
static inline long KKillProcess(DWORD ProcessID)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x080000), "b"(ProcessID));
	return res;
}

/*注册内核端口对应线程*/
static inline long KRegKnlPort(DWORD KnlPort)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x090000), "b"(KnlPort));
	return res;
}

/*注销内核端口对应线程*/
static inline long KUnregKnlPort(DWORD KnlPort)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0A0000), "b"(KnlPort));
	return res;
}

/*取得内核端口对应线程*/
static inline long KGetKptThed(DWORD KnlPort, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0B0000), "1"(KnlPort));
	return res;
}

/*注册IRQ信号的响应线程*/
static inline long KRegIrq(DWORD irq)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0C0000), "b"(irq));
	return res;
}

/*注销IRQ信号的响应线程*/
static inline long KUnregIrq(DWORD irq)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0D0000), "b"(irq));
	return res;
}

/*发送消息*/
static inline long KSendMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0E0000), "1"(*ptid), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*接收消息*/
static inline long KRecvMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x0F0000), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*接收指定进程的消息*/
static inline long KRecvProcMsg(THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x100000), "1"(*ptid), "c"(CentiSeconds), "S"(data): "memory");
	return res;
}

/*映射物理地址*/
static inline long KMapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=S"(*addr): "0"(0x110000), "b"(PhyAddr), "c"(siz): "memory");
	return res;
}

/*映射用户地址*/
static inline long KMapUserAddr(void **addr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=S"(*addr): "0"(0x120000), "c"(siz));
	return res;
}

/*回收用户地址块*/
static inline long KFreeAddr(void *addr)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x130000), "S"(addr): "memory");
	return res;
}

/*映射进程地址读取*/
static inline long KReadProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x140000), "1"(*ptid), "c"(siz), "d"(CentiSeconds), "S"(data), "D"(addr): "memory");
	return res;
}

/*映射进程地址写入*/
static inline long KWriteProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, DWORD data[MSG_DATA_LEN], DWORD CentiSeconds)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x150000), "1"(*ptid), "c"(siz), "d"(CentiSeconds), "S"(data), "D"(addr): "memory");
	return res;
}

/*撤销映射进程地址*/
static inline long KUnmapProcAddr(void *addr, const DWORD data[MSG_DATA_LEN - 2])
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x160000), "S"(data), "D"(addr): "memory");
	return res;
}

/*取消映射进程地址*/
static inline long KCnlmapProcAddr(void *addr, const DWORD data[MSG_DATA_LEN - 2])
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x170000), "S"(data), "D"(addr): "memory");
	return res;
}

/*取得开机经过的时钟*/
static inline long KGetClock(DWORD *clock)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*clock): "0"(0x180000));
	return res;
}

/**********其他**********/

/*锁变量锁定(驱动专用)*/
static inline void lock(volatile DWORD *l)
{
	cli();
	while (*l)
		KGiveUp();
	*l = TRUE;
	sti();
}

/*锁变量解锁(驱动专用)*/
static inline void ulock(volatile DWORD *l)
{
	*l = FALSE;
}

/*发送退出请求*/
static inline long SendExitReq(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_EXITREQ;
	return KSendMsg(&ptid, data, 0);
}

#endif

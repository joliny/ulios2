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
#define ERROR_INVALID_MAPADDR		-21	/*非法的映射地址*/
#define ERROR_INVALID_MAPSIZE		-22	/*非法的映射大小*/

#define MSG_ATTR_ISR		0x00010000	/*异常信号消息*/
#define MSG_ATTR_IRQ		0x00020000	/*中断信号消息*/
#define MSG_ATTR_SYS		0x00030000	/*系统消息*/
#define MSG_ATTR_READ		0x00040000	/*读数据消息,页映射方式读*/
#define MSG_ATTR_WRITE		0x00050000	/*写数据消息,页映射方式写*/
#define MSG_ATTR_PROC		0x01000000	/*进程自定义消息最小值*/

/**********基本操作**********/

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

/*关中断*/
static inline void cli()
{
	__asm__("cli");
}

/*开中断*/
static inline void sti()
{
	__asm__("sti");
}

/*端口输出字节*/
static inline void outb(WORD port, BYTE b)
{
	__asm__ __volatile__("outb %1, %0":: "Nd"(port), "a"(b));
}

/*端口输入字节*/
static inline BYTE inb(WORD port)
{
	register BYTE b;
	__asm__ __volatile__("inb %1, %0": "=a"(b): "Nd"(port));
	return b;
}

/**********系统调用接口**********/

/*调试输出*/
static inline long KPrintf(char *str, DWORD num)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x000000), "b"(num), "S"(str));
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
static inline long KCreateThread(void (*ThreadProc)(), DWORD StackSize, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x030000), "1"(ThreadProc), "c"(StackSize));
	return res;
}

/*退出线程*/
static inline void KExitThread(DWORD ExitCode)
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
static inline long KCreateProcess(DWORD attr, char *command, THREAD_ID *ptid)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid): "0"(0x060000), "1"(attr), "S"(command));
	return res;
}

/*退出进程*/
static inline void KExitProcess(DWORD ExitCode)
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
static inline long KGetKpToThed(DWORD KnlPort, THREAD_ID *ptid)
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
static inline long KSendMsg(THREAD_ID ptid, DWORD c, DWORD d, DWORD src, DWORD dest)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x0E0000), "b"(ptid), "c"(c), "d"(d), "S"(src), "D"(dest));
	return res;
}

/*接收消息*/
static inline long KRecvMsg(THREAD_ID *ptid, DWORD *c, DWORD *d, DWORD *src, DWORD *dest)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid), "=c"(*c), "=d"(*d), "=S"(*src), "=D"(*dest): "0"(0x0F0000));
	return res;
}

/*阻塞并等待消息*/
static inline long KWaitMsg(DWORD CentiSeconds, THREAD_ID *ptid, DWORD *c, DWORD *d, DWORD *src, DWORD *dest)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*ptid), "=c"(*c), "=d"(*d), "=S"(*src), "=D"(*dest): "0"(0x100000), "1"(CentiSeconds));
	return res;
}

/*映射物理地址*/
static inline long KMapPhyAddr(DWORD *addr, DWORD PhyAddr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*addr): "0"(0x110000), "1"(PhyAddr), "c"(siz));
	return res;
}

/*映射用户地址*/
static inline long KMapUserAddr(DWORD *addr, DWORD siz)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res), "=b"(*addr): "0"(0x120000), "1"(siz));
	return res;
}

/*回收用户地址块*/
static inline long KFreeAddr(DWORD addr)
{
	register long res;
	__asm__ __volatile__("int $0xF0": "=a"(res): "0"(0x130000), "b"(addr));
	return res;
}

#endif

/*	ulidef.h for ulios
	作者：孙亮
	功能：ulios内核代码基本宏定义
	最后修改日期：2009-05-26
*/

#ifndef	_ULIDEF_H_
#define	_ULIDEF_H_

typedef unsigned char		BYTE;	/*8位*/
typedef unsigned short		WORD;	/*16位*/
typedef unsigned long		DWORD;	/*32位*/
typedef unsigned long		BOOL;

#define TRUE	1
#define FALSE	0
#define NULL	((void*)0)
#define INVALID	(~0)

/*32位内存设置*/
static inline void memset32(void *dest, DWORD d, DWORD n)
{
	register void *_dest;
	register DWORD _n;
	__asm__ __volatile__("cld;rep stosl": "=&D"(_dest), "=&c"(_n): "0"(dest), "a"(d), "1"(n): "flags", "memory");
}

/*32位内存复制*/
static inline void memcpy32(void *dest, const void *src, DWORD n)
{
	register void *_dest;
	register const void *_src;
	register DWORD _n;
	__asm__ __volatile__("cld;rep movsl": "=&D"(_dest), "=&S"(_src), "=&c"(_n): "0"(dest), "1"(src), "2"(n): "flags", "memory");
}

/*8位字符串限长复制*/
static inline void strncpy8(void *dest, const void *src, DWORD n)
{
	register void *_dest;
	register const void *_src;
	register DWORD _n;
	__asm__ __volatile__
	(
		"cld\n"
		"1:\n"
		"decl %2\n"
		"js 2f\n"
		"lodsb\n"
		"stosb\n"
		"testb %%al, %%al\n"
		"jne 1b\n"
		"2:"
		: "=&D"(_dest), "=&S"(_src), "=&c"(_n)
		: "0"(dest), "1"(src), "2"(n)
		: "flags", "memory"
	);
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

typedef struct _THREAD_ID
{
	WORD ProcID;
	WORD ThedID;
}THREAD_ID;	/*进程线程ID*/

/*进程调度*/
void schedul();

/*锁变量锁定*/
static inline void lock(volatile DWORD *l)
{
	cli();
	while (*l)
		schedul();
	*l = TRUE;
	sti();
}

/*锁变量解锁*/
static inline void ulock(volatile DWORD *l)
{
	*l = FALSE;
}

/*检查锁变量并关中断*/
#define clilock(l) \
({\
	cli();\
	while (l)\
		schedul();\
})

#endif

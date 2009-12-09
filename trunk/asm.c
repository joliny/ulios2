#include"ulidef.h"
#define sti() __asm__("sti"::)
#define cli() __asm__("cli"::)
#define nop() __asm__("nop"::)
#define iret() __asm__("iret"::)
//I/O指令
#define outportb(port,value) __asm__("outb %%al,%%dx"::"d"(port),"a"(value))
#define outportw(port,value) __asm__("outw %%ax,%%dx"::"d"(port),"a"(value))
#define inportb(port) ({byte _v;__asm__ volatile("inb %%dx,%%al":"=a"(_v):"d"(port));_v;})
//进程相关
#define move_to_user_mode() \
__asm__ ("movl %%esp,%%eax\n\t" \
	"pushl $0x17\n\t" \
	"pushl %%eax\n\t" \
	"pushfl\n\t" \
	"pushl $0x0f\n\t" \
	"pushl $1f\n\t" \
	"iret\n" \
	"1:\tmovl $0x17,%%eax\n\t" \
	"movw %%ax,%%ds\n\t" \
	"movw %%ax,%%es\n\t" \
	"movw %%ax,%%fs\n\t" \
	"movw %%ax,%%gs" \
	:::"ax")
#define _set_gate(gate_addr,type,dpl,addr) \
__asm__ ("movw %%dx,%%ax\n\t" \
	"movw %0,%%dx\n\t" \
	"movl %%eax,%1\n\t" \
	"movl %%edx,%2" \
	: \
	: "i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	"o" (*((char *) (gate_addr))), \
	"o" (*(4+(char *) (gate_addr))), \
	"d" ((char *) (addr)),"a" (0x00080000))
#define set_intr_gate(n,addr) _set_gate(&idt[n],14,0,addr)
#define set_trap_gate(n,addr) _set_gate(&idt[n],15,0,addr)
#define set_system_gate(n,addr) _set_gate(&idt[n],15,3,addr)
#define _set_seg_desc(gate_addr,type,dpl,base,limit) {\
	*(gate_addr) = ((base) & 0xff000000) | \
		(((base) & 0x00ff0000)>>16) | \
		((limit) & 0xf0000) | \
		((dpl)<<13) | \
		(0x00408000) | \
		((type)<<8); \
	*((gate_addr)+1) = (((base) & 0x0000ffff)<<16) | \
		((limit) & 0x0ffff); }
#define _set_tssldt_desc(n,addr,type) \
__asm__ ("movw $104,%1\n\t" \
	"movw %%ax,%2\n\t" \
	"rorl $16,%%eax\n\t" \
	"movb %%al,%3\n\t" \
	"movb $" type ",%4\n\t" \
	"movb $0x00,%5\n\t" \
	"movb %%ah,%6\n\t" \
	"rorl $16,%%eax" \
	::"a" (addr), "m" (*(n)), "m" (*(n+2)), "m" (*(n+4)), \
	 "m" (*(n+5)), "m" (*(n+6)), "m" (*(n+7)) \
	)
#define set_tss_desc(n,addr) _set_tssldt_desc(((char *) (n)),((int)(addr)),"0x89")
#define set_ldt_desc(n,addr) _set_tssldt_desc(((char *) (n)),((int)(addr)),"0x82")
//字符串操作
extern inline char *strcpy(char *dest,const char *src)
{	__asm__
	(	"cld\n"
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b"
		::"S" (src),"D" (dest)
	);
	return dest;
}
static inline char *strncpy(char *dest,const char *src,int count)
{	__asm__
	(	"cld\n"
		"1:\tdecl %2\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"rep\n\t"
		"stosb\n"
		"2:"
		::"S" (src),"D" (dest),"c" (count)
	);
	return dest;
}
extern inline char *strcat(char *dest,const char *src)
{	__asm__
	(	"cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"decl %1\n"
		"1:\tlodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b"
		::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff)
	);
	return dest;
}
static inline char *strncat(char *dest,const char *src,int count)
{	__asm__
	(	"cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"decl %1\n\t"
		"movl %4,%3\n"
		"1:\tdecl %3\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"stosb\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n"
		"2:\txorl %2,%2\n\t"
		"stosb"
		::"S" (src),"D" (dest),"a" (0),"c" (0xffffffff),"g" (count)
	);
	return dest;
}
extern inline int strcmp(const char *cs,const char *ct)
{	register int __res;
	__asm__
	(	"cld\n"
		"1:\tlodsb\n\t"
		"scasb\n\t"
		"jne 2f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"xorl %%eax,%%eax\n\t"
		"jmp 3f\n"
		"2:\tmovl $1,%%eax\n\t"
		"jl 3f\n\t"
		"negl %%eax\n"
		"3:"
		:"=a" (__res):"D" (cs),"S" (ct)
	);
	return __res;
}
static inline int strncmp(const char *cs,const char *ct,int count)
{	register int __res;
	__asm__
	(	"cld\n"
		"1:\tdecl %3\n\t"
		"js 2f\n\t"
		"lodsb\n\t"
		"scasb\n\t"
		"jne 3f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n"
		"2:\txorl %%eax,%%eax\n\t"
		"jmp 4f\n"
		"3:\tmovl $1,%%eax\n\t"
		"jl 4f\n\t"
		"negl %%eax\n"
		"4:"
		:"=a" (__res):"D" (cs),"S" (ct),"c" (count)
	);
	return __res;
}
static inline char *strchr(const char *s,char c)
{	register char *__res;
	__asm__
	(	"cld\n\t"
		"movb %%al,%%ah\n"
		"1:\tlodsb\n\t"
		"cmpb %%ah,%%al\n\t"
		"je 2f\n\t"
		"testb %%al,%%al\n\t"
		"jne 1b\n\t"
		"movl $1,%1\n"
		"2:\tmovl %1,%0\n\t"
		"decl %0"
		:"=a" (__res):"S" (s),"0" (c)
	);
	return __res;
}
static inline char *strrchr(const char *s,char c)
{	register char *__res;
	__asm__
	(	"cld\n\t"
		"movb %%al,%%ah\n"
		"1:\tlodsb\n\t"
		"cmpb %%ah,%%al\n\t"
		"jne 2f\n\t"
		"movl %%esi,%0\n\t"
		"decl %0\n"
		"2:\ttestb %%al,%%al\n\t"
		"jne 1b"
		:"=d" (__res):"0" (0),"S" (s),"a" (c)
	);
	return __res;
}
extern inline int strspn(const char *cs, const char *ct)
{	register char *__res;
	__asm__
	(	"cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"
		"movl %%ecx,%%edx\n"
		"1:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 2f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"je 1b\n"
		"2:\tdecl %0"
		:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	);
	return __res-cs;
}
extern inline int strcspn(const char *cs, const char *ct)
{	register char *__res;
	__asm__
	(	"cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"
		"movl %%ecx,%%edx\n"
		"1:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 2f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"jne 1b\n"
		"2:\tdecl %0"
		:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	);
	return __res-cs;
}
extern inline char *strpbrk(const char *cs,const char *ct)
{	register char *__res;
	__asm__
	(	"cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"
		"movl %%ecx,%%edx\n"
		"1:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 2f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"jne 1b\n\t"
		"decl %0\n\t"
		"jmp 3f\n"
		"2:\txorl %0,%0\n"
		"3:"
		:"=S" (__res):"a" (0),"c" (0xffffffff),"0" (cs),"g" (ct)
	);
	return __res;
}
extern inline char *strstr(const char *cs,const char *ct)
{	register char *__res;
	__asm__
	(	"cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
		"movl %%ecx,%%edx\n"
		"1:\tmovl %4,%%edi\n\t"
		"movl %%esi,%%eax\n\t"
		"movl %%edx,%%ecx\n\t"
		"repe\n\t"
		"cmpsb\n\t"
		"je 2f\n\t"		/* also works for empty string, see above */
		"xchgl %%eax,%%esi\n\t"
		"incl %%esi\n\t"
		"cmpb $0,-1(%%eax)\n\t"
		"jne 1b\n\t"
		"xorl %%eax,%%eax\n\t"
		"2:"
		:"=a" (__res):"0" (0),"c" (0xffffffff),"S" (cs),"g" (ct)
	);
	return __res;
}
extern inline int strlen(const char *s)
{	register int __res;
	__asm__
	(	"cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %0\n\t"
		"decl %0"
		:"=c" (__res):"D" (s),"a" (0),"0" (0xffffffff)
	);
	return __res;
}
extern char *___strtok;
extern inline char *strtok(char *s,const char *ct)
{	register char *__res;
	__asm__
	(	"testl %1,%1\n\t"
		"jne 1f\n\t"
		"testl %0,%0\n\t"
		"je 8f\n\t"
		"movl %0,%1\n"
		"1:\txorl %0,%0\n\t"
		"movl $-1,%%ecx\n\t"
		"xorl %%eax,%%eax\n\t"
		"cld\n\t"
		"movl %4,%%edi\n\t"
		"repne\n\t"
		"scasb\n\t"
		"notl %%ecx\n\t"
		"decl %%ecx\n\t"
		"je 7f\n\t"			/* empty delimeter-string */
		"movl %%ecx,%%edx\n"
		"2:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 7f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"je 2b\n\t"
		"decl %1\n\t"
		"cmpb $0,(%1)\n\t"
		"je 7f\n\t"
		"movl %1,%0\n"
		"3:\tlodsb\n\t"
		"testb %%al,%%al\n\t"
		"je 5f\n\t"
		"movl %4,%%edi\n\t"
		"movl %%edx,%%ecx\n\t"
		"repne\n\t"
		"scasb\n\t"
		"jne 3b\n\t"
		"decl %1\n\t"
		"cmpb $0,(%1)\n\t"
		"je 5f\n\t"
		"movb $0,(%1)\n\t"
		"incl %1\n\t"
		"jmp 6f\n"
		"5:\txorl %1,%1\n"
		"6:\tcmpb $0,(%0)\n\t"
		"jne 7f\n\t"
		"xorl %0,%0\n"
		"7:\ttestl %0,%0\n\t"
		"jne 8f\n\t"
		"movl %0,%1\n"
		"8:"
		:"=b" (__res),"=S" (___strtok)
		:"0" (___strtok),"1" (s),"g" (ct)
	);
	return __res;
}
static inline void *memcpy(void *dest,const void *src, int n)
{	__asm__
	(	"cld\n\t"
		"rep\n\t"
		"movsb"
		::"c" (n),"S" (src),"D" (dest)
	);
	return dest;
}
extern inline void *memmove(void *dest,const void *src, int n)
{	if(dest<src)
	__asm__
	(	"cld\n\t"
		"rep\n\t"
		"movsb"
		::"c" (n),"S" (src),"D" (dest)
	);
	else
	__asm__
	(	"std\n\t"
		"rep\n\t"
		"movsb"
		::"c" (n),"S" (src+n-1),"D" (dest+n-1)
	);
	return dest;
}
static inline int memcmp(const void *cs,const void *ct,int count)
{	register int __res;
	__asm__
	(	"cld\n\t"
		"repe\n\t"
		"cmpsb\n\t"
		"je 1f\n\t"
		"movl $1,%%eax\n\t"
		"jl 1f\n\t"
		"negl %%eax\n"
		"1:"
		:"=a" (__res):"0" (0),"D" (cs),"S" (ct),"c" (count)
	);
	return __res;
}
extern inline void *memchr(const void *cs,char c,int count)
{	register void *__res;
	if(!count)return NULL;
	__asm__
	(	"cld\n\t"
		"repne\n\t"
		"scasb\n\t"
		"je 1f\n\t"
		"movl $1,%0\n"
		"1:\tdecl %0"
		:"=D" (__res):"a" (c),"D" (cs),"c" (count)
	);
	return __res;
}
static inline void *memset(void *s,char c,int count)
{	__asm__
	(	"cld\n\t"
		"rep\n\t"
		"stosb"
		::"a" (c),"D" (s),"c" (count)
	);
	return s;
}
/**********公用函数**********/

/*设置伪随机数种子*/
void SetRand(DWORD seed)
{
	RandSeed = seed;
}

/*伪随机数发生器*/
DWORD rand()
{
	return RandSeed = RandSeed * 1103515245 + 12345;
}

/**********工具函数**********/

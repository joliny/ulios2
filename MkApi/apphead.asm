;apphead.asm for ulios program
;作者：孙亮
;功能：应用程序的入口代码,调用程序的C语言main函数
;最后修改日期：2009-09-18

[BITS 32]
global _start
extern main
_start:		;初始化段寄存器
	mov	ax,	ss	;设置数据段选择子
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax

	call	main
	mov	eax,	0x070000	;退出进程
	int	0xF0

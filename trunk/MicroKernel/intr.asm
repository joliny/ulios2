;intr.asm for ulios
;作者：孙亮
;功能：中段/异常/系统调用发生时的汇编堆栈操作
;最后修改日期：2009-06-30

[BITS 32]
global AsmIsr00
global AsmIsr01
global AsmIsr02
global AsmIsr03
global AsmIsr04
global AsmIsr05
global AsmIsr06
global AsmIsr07
global AsmIsr08
global AsmIsr09
global AsmIsr0A
global AsmIsr0B
global AsmIsr0C
global AsmIsr0D
global AsmIsr0E
global AsmIsr0F
global AsmIsr10
global AsmIsr11
global AsmIsr12
global AsmIsr13
extern IsrCallTable

AsmIsr00:	;除法错(故障)
	push	byte	0
	push	byte	0
	jmp	short	AsmIsrProc
AsmIsr01:	;调试异常(故障或陷阱)
	push	byte	0
	push	byte	1
	jmp	short	AsmIsrProc
AsmIsr02:	;不可屏蔽中断
	push	byte	0
	push	byte	2
	jmp	short	AsmIsrProc
AsmIsr03:	;单字节INT3调试断点(陷阱)
	push	byte	0
	push	byte	3
	jmp	short	AsmIsrProc
AsmIsr04:	;INTO异常(陷阱)
	push	byte	0
	push	byte	4
	jmp	short	AsmIsrProc
AsmIsr05:	;边界检查越界(故障)
	push	byte	0
	push	byte	5
	jmp	short	AsmIsrProc
AsmIsr06:	;非法操作码(故障)
	push	byte	0
	push	byte	6
	jmp	short	AsmIsrProc
AsmIsr07:	;协处理器不可用(故障)
	push	byte	0
	push	byte	7
	jmp	short	AsmIsrProc
AsmIsr08:	;双重故障(异常中止)

	push	byte	8
	jmp	short	AsmIsrProc
AsmIsr09:	;协处理器段越界(异常中止)
	push	byte	0
	push	byte	9
	jmp	short	AsmIsrProc
AsmIsr0A:	;无效TSS异常(故障)

	push	byte	10
	jmp	short	AsmIsrProc
AsmIsr0B:	;段不存在(故障)

	push	byte	11
	jmp	short	AsmIsrProc
AsmIsr0C:	;堆栈段异常(故障)

	push	byte	12
	jmp	short	AsmIsrProc
AsmIsr0D:	;通用保护异常(故障)

	push	byte	13
	jmp	short	AsmIsrProc
AsmIsr0E:	;页异常(故障)

	push	byte	14
	jmp	short	AsmIsrProc
AsmIsr0F:	;Intel保留
	push	byte	0
	push	byte	15
	jmp	short	AsmIsrProc
AsmIsr10:	;协处理器浮点运算出错(故障)
	push	byte	0
	push	byte	16
	jmp	short	AsmIsrProc
AsmIsr11:	;对齐检验(486+)(故障)

	push	byte	17
	jmp	short	AsmIsrProc
AsmIsr12:	;Machine Check(奔腾+)(异常中止)
	push	byte	0
	push	byte	18
	jmp	short	AsmIsrProc
AsmIsr13:	;SIMD浮点异常(奔腾III+)(故障)
	push	byte	0
	push	byte	19

AsmIsrProc:
	push	ds
	push	es
	push	fs
	push	gs
	pushad

	mov	ax,	ss
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	eax,	[esp + 48]
	call	[IsrCallTable + eax * 4]

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	add	esp,	8
	iretd

global AsmIrq0
global AsmIrq1
global AsmIrq2
global AsmIrq3
global AsmIrq4
global AsmIrq5
global AsmIrq6
global AsmIrq7
global AsmIrq8
global AsmIrq9
global AsmIrqA
global AsmIrqB
global AsmIrqC
global AsmIrqD
global AsmIrqE
global AsmIrqF
extern IrqProc

AsmIrq0:	;时钟
	push	byte	0
	jmp	short	AsmIrqProc
AsmIrq1:	;键盘
	push	byte	1
	jmp	short	AsmIrqProc
AsmIrq2:	;从8259A
	push	byte	2
	jmp	short	AsmIrqProc
AsmIrq3:	;串口2
	push	byte	3
	jmp	short	AsmIrqProc
AsmIrq4:	;串口1
	push	byte	4
	jmp	short	AsmIrqProc
AsmIrq5:	;LPT2
	push	byte	5
	jmp	short	AsmIrqProc
AsmIrq6:	;软盘
	push	byte	6
	jmp	short	AsmIrqProc
AsmIrq7:	;LPT1
	push	byte	7
	jmp	short	AsmIrqProc
AsmIrq8:	;实时钟
	push	byte	8
	jmp	short	AsmIrqProc
AsmIrq9:	;重定向IRQ2
	push	byte	9
	jmp	short	AsmIrqProc
AsmIrqA:	;保留
	push	byte	10
	jmp	short	AsmIrqProc
AsmIrqB:	;保留
	push	byte	11
	jmp	short	AsmIrqProc
AsmIrqC:	;PS2鼠标
	push	byte	12
	jmp	short	AsmIrqProc
AsmIrqD:	;FPU异常
	push	byte	13
	jmp	short	AsmIrqProc
AsmIrqE:	;AT温盘
	push	byte	14
	jmp	short	AsmIrqProc
AsmIrqF:	;保留
	push	byte	15

AsmIrqProc:
	push	ds
	push	es
	push	fs
	push	gs
	pushad

	mov	ax,	ss
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	al,	0x20	;主片发送EOI
	out	0x20,	al
	cmp	byte[esp + 48],	8
	jb	CallIrqProc	;主片的IRQ信号，不给从片发送EOI
	out	0xA0,	al	;从片发送EOI
CallIrqProc:
	call	IrqProc

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	add	esp,	4
	iretd

global AsmApiCall
extern ApiCall

AsmApiCall:	;系统调用接口
	push	ds
	push	es
	push	fs
	push	gs
	pushad

	mov	ax,	ss
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	call	ApiCall

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	iretd

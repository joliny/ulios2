;intr.asm for ulios
;���ߣ�����
;���ܣ��ж�/�쳣/ϵͳ���÷���ʱ�Ļ���ջ����
;����޸����ڣ�2009-06-30

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

AsmIsr00:	;������(����)
	push	byte	0
	push	byte	0
	jmp	short	AsmIsrProc
AsmIsr01:	;�����쳣(���ϻ�����)
	push	byte	0
	push	byte	1
	jmp	short	AsmIsrProc
AsmIsr02:	;���������ж�
	push	byte	0
	push	byte	2
	jmp	short	AsmIsrProc
AsmIsr03:	;���ֽ�INT3���Զϵ�(����)
	push	byte	0
	push	byte	3
	jmp	short	AsmIsrProc
AsmIsr04:	;INTO�쳣(����)
	push	byte	0
	push	byte	4
	jmp	short	AsmIsrProc
AsmIsr05:	;�߽���Խ��(����)
	push	byte	0
	push	byte	5
	jmp	short	AsmIsrProc
AsmIsr06:	;�Ƿ�������(����)
	push	byte	0
	push	byte	6
	jmp	short	AsmIsrProc
AsmIsr07:	;Э������������(����)
	push	byte	0
	push	byte	7
	jmp	short	AsmIsrProc
AsmIsr08:	;˫�ع���(�쳣��ֹ)

	push	byte	8
	jmp	short	AsmIsrProc
AsmIsr09:	;Э��������Խ��(�쳣��ֹ)
	push	byte	0
	push	byte	9
	jmp	short	AsmIsrProc
AsmIsr0A:	;��ЧTSS�쳣(����)

	push	byte	10
	jmp	short	AsmIsrProc
AsmIsr0B:	;�β�����(����)

	push	byte	11
	jmp	short	AsmIsrProc
AsmIsr0C:	;��ջ���쳣(����)

	push	byte	12
	jmp	short	AsmIsrProc
AsmIsr0D:	;ͨ�ñ����쳣(����)

	push	byte	13
	jmp	short	AsmIsrProc
AsmIsr0E:	;ҳ�쳣(����)

	push	byte	14
	jmp	short	AsmIsrProc
AsmIsr0F:	;Intel����
	push	byte	0
	push	byte	15
	jmp	short	AsmIsrProc
AsmIsr10:	;Э�����������������(����)
	push	byte	0
	push	byte	16
	jmp	short	AsmIsrProc
AsmIsr11:	;�������(486+)(����)

	push	byte	17
	jmp	short	AsmIsrProc
AsmIsr12:	;Machine Check(����+)(�쳣��ֹ)
	push	byte	0
	push	byte	18
	jmp	short	AsmIsrProc
AsmIsr13:	;SIMD�����쳣(����III+)(����)
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

AsmIrq0:	;ʱ��
	push	byte	0
	jmp	short	AsmIrqProc
AsmIrq1:	;����
	push	byte	1
	jmp	short	AsmIrqProc
AsmIrq2:	;��8259A
	push	byte	2
	jmp	short	AsmIrqProc
AsmIrq3:	;����2
	push	byte	3
	jmp	short	AsmIrqProc
AsmIrq4:	;����1
	push	byte	4
	jmp	short	AsmIrqProc
AsmIrq5:	;LPT2
	push	byte	5
	jmp	short	AsmIrqProc
AsmIrq6:	;����
	push	byte	6
	jmp	short	AsmIrqProc
AsmIrq7:	;LPT1
	push	byte	7
	jmp	short	AsmIrqProc
AsmIrq8:	;ʵʱ��
	push	byte	8
	jmp	short	AsmIrqProc
AsmIrq9:	;�ض���IRQ2
	push	byte	9
	jmp	short	AsmIrqProc
AsmIrqA:	;����
	push	byte	10
	jmp	short	AsmIrqProc
AsmIrqB:	;����
	push	byte	11
	jmp	short	AsmIrqProc
AsmIrqC:	;PS2���
	push	byte	12
	jmp	short	AsmIrqProc
AsmIrqD:	;FPU�쳣
	push	byte	13
	jmp	short	AsmIrqProc
AsmIrqE:	;AT����
	push	byte	14
	jmp	short	AsmIrqProc
AsmIrqF:	;����
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
	mov	al,	0x20	;��Ƭ����EOI
	out	0x20,	al
	cmp	byte[esp + 48],	8
	jb	CallIrqProc	;��Ƭ��IRQ�źţ�������Ƭ����EOI
	out	0xA0,	al	;��Ƭ����EOI
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

AsmApiCall:	;ϵͳ���ýӿ�
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

;intr.asm for ulios
;���ߣ�����
;���ܣ��ж�/�쳣/ϵͳ���÷���ʱ�Ļ���ջ����
;����޸����ڣ�2009-06-30

[BITS 32]
global _AsmIsr00
global _AsmIsr01
global _AsmIsr02
global _AsmIsr03
global _AsmIsr04
global _AsmIsr05
global _AsmIsr06
global _AsmIsr07
global _AsmIsr08
global _AsmIsr09
global _AsmIsr0A
global _AsmIsr0B
global _AsmIsr0C
global _AsmIsr0D
global _AsmIsr0E
global _AsmIsr0F
global _AsmIsr10
global _AsmIsr11
global _AsmIsr12
global _AsmIsr13
extern _IsrCallTable

_AsmIsr00:	;������(����)
	push	byte	0
	push	byte	0
	jmp	short	AsmIsrProc
_AsmIsr01:	;�����쳣(���ϻ�����)
	push	byte	0
	push	byte	1
	jmp	short	AsmIsrProc
_AsmIsr02:	;���������ж�
	push	byte	0
	push	byte	2
	jmp	short	AsmIsrProc
_AsmIsr03:	;���ֽ�INT3���Զϵ�(����)
	push	byte	0
	push	byte	3
	jmp	short	AsmIsrProc
_AsmIsr04:	;INTO�쳣(����)
	push	byte	0
	push	byte	4
	jmp	short	AsmIsrProc
_AsmIsr05:	;�߽���Խ��(����)
	push	byte	0
	push	byte	5
	jmp	short	AsmIsrProc
_AsmIsr06:	;�Ƿ�������(����)
	push	byte	0
	push	byte	6
	jmp	short	AsmIsrProc
_AsmIsr07:	;Э������������(����)
	push	byte	0
	push	byte	7
	jmp	short	AsmIsrProc
_AsmIsr08:	;˫�ع���(�쳣��ֹ)

	push	byte	8
	jmp	short	AsmIsrProc
_AsmIsr09:	;Э��������Խ��(�쳣��ֹ)
	push	byte	0
	push	byte	9
	jmp	short	AsmIsrProc
_AsmIsr0A:	;��ЧTSS�쳣(����)

	push	byte	10
	jmp	short	AsmIsrProc
_AsmIsr0B:	;�β�����(����)

	push	byte	11
	jmp	short	AsmIsrProc
_AsmIsr0C:	;��ջ���쳣(����)

	push	byte	12
	jmp	short	AsmIsrProc
_AsmIsr0D:	;ͨ�ñ����쳣(����)

	push	byte	13
	jmp	short	AsmIsrProc
_AsmIsr0E:	;ҳ�쳣(����)

	push	byte	14
	jmp	short	AsmIsrProc
_AsmIsr0F:	;Intel����
	push	byte	0
	push	byte	15
	jmp	short	AsmIsrProc
_AsmIsr10:	;Э�����������������(����)
	push	byte	0
	push	byte	16
	jmp	short	AsmIsrProc
_AsmIsr11:	;�������(486+)(����)

	push	byte	17
	jmp	short	AsmIsrProc
_AsmIsr12:	;Machine Check(����+)(�쳣��ֹ)
	push	byte	0
	push	byte	18
	jmp	short	AsmIsrProc
_AsmIsr13:	;SIMD�����쳣(����III+)(����)
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
	call	[_IsrCallTable + eax * 4]

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	add	esp,	8
	iretd

global _AsmIrq0
global _AsmIrq1
global _AsmIrq2
global _AsmIrq3
global _AsmIrq4
global _AsmIrq5
global _AsmIrq6
global _AsmIrq7
global _AsmIrq8
global _AsmIrq9
global _AsmIrqA
global _AsmIrqB
global _AsmIrqC
global _AsmIrqD
global _AsmIrqE
global _AsmIrqF
extern _IrqProc

_AsmIrq0:	;ʱ��
	push	byte	0
	jmp	short	AsmIrqProc
_AsmIrq1:	;����
	push	byte	1
	jmp	short	AsmIrqProc
_AsmIrq2:	;��8259A
	push	byte	2
	jmp	short	AsmIrqProc
_AsmIrq3:	;����2
	push	byte	3
	jmp	short	AsmIrqProc
_AsmIrq4:	;����1
	push	byte	4
	jmp	short	AsmIrqProc
_AsmIrq5:	;LPT2
	push	byte	5
	jmp	short	AsmIrqProc
_AsmIrq6:	;����
	push	byte	6
	jmp	short	AsmIrqProc
_AsmIrq7:	;LPT1
	push	byte	7
	jmp	short	AsmIrqProc
_AsmIrq8:	;ʵʱ��
	push	byte	8
	jmp	short	AsmIrqProc
_AsmIrq9:	;�ض���IRQ2
	push	byte	9
	jmp	short	AsmIrqProc
_AsmIrqA:	;����
	push	byte	10
	jmp	short	AsmIrqProc
_AsmIrqB:	;����
	push	byte	11
	jmp	short	AsmIrqProc
_AsmIrqC:	;PS2���
	push	byte	12
	jmp	short	AsmIrqProc
_AsmIrqD:	;FPU�쳣
	push	byte	13
	jmp	short	AsmIrqProc
_AsmIrqE:	;AT����
	push	byte	14
	jmp	short	AsmIrqProc
_AsmIrqF:	;����
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
	call	_IrqProc

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	add	esp,	4
	iretd

global _AsmApiCall
extern _ApiCall

_AsmApiCall:	;ϵͳ���ýӿ�
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
	call	_ApiCall

	popad
	pop	gs
	pop	fs
	pop	es
	pop	ds
	iretd

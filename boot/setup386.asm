;setup386.asm for ulios
;���ߣ�����
;���ܣ����б���ģʽǰ��׼�������ñ���ģʽ���ս���32λ�ںˣ���setup�汾ֻ����80386 CPU��û�д�pentium pro�߼����ܵĲ���
;����޸����ڣ�2009-06-25
;��ע��ʹ��NASM�����������BIN�ļ�

[BITS 16]
[ORG 0x1A00]
;�ں˱��ַ��С
GDTsize		equ	0x0800	;GDT�ֽ���
GDTseg		equ	0	;GDT��
GDToff		equ	0	;GDTƫ��
IDTsize		equ	0x0800	;IDT�ֽ���
IDTseg		equ	0	;IDT��
IDToff		equ	0x0800	;IDTƫ��
KPDTsize	equ	0x2000	;ҳĿ¼���ֽ���(����һ��PDDT)
KPDTseg		equ	0	;ҳĿ¼���
KPDToff		equ	0x1000	;ҳĿ¼��ƫ��
KNLoff		equ	0x10000	;�ں���ʼ��ַƫ��
;��������λ��
MemEnd		equ	0x00FC	;4�ֽ��ڴ�����
ARDStruct	equ	0x0100	;20 * 19�ֽ��ڴ�ṹ��
VesaMode	equ	0x02FC	;4�ֽ�����VESAģʽ��
VbeInfo		equ	0x0300	;512�ֽ�VESA��Ϣ
ModeInfo	equ	0x0500	;256�ֽ�ģʽ��Ϣ
HdInfo		equ	0x0600	;32�ֽ�����Ӳ�̲���

setup:
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	0xF000
;----------------------------------------
;ȡ���ڴ�ֲ���
	xor	ebx,	ebx
	mov	[MemEnd],	ebx
	mov	di,	ARDStruct
MemChkLoop:
	mov	eax,	0xE820
	mov	cx,	20
	mov	edx,	'PAMS'
	int	0x15
	jc	short	MemChkFail
	cmp	dword[di + 16],	1		;�������
	jne	short	MenChkGoon
	mov	eax,	[di]
	add	eax,	[di + 8]
	cmp	[MemEnd],	eax		;�������
	jae	short	MenChkGoon
	mov	[MemEnd],	eax
MenChkGoon:
	add	di,	20
	cmp	ebx,	0
	jne	short	MemChkLoop
	cmp	dword[MemEnd],	0x00800000	;֧����С�ڴ���8M
	jae	short	MemChkOk
MemTooSmall:
	mov	si,	MemSmallMsg
	call	Print
	jmp	short	$
MemSmallMsg	DB	"OUT OF MEMARY!",0
MemChkFail:
	mov	si,	MemChkMsg
	call	Print
	jmp	short	$
MemChkMsg	DB	"MEMARY CHECK FAIL!",0
MemChkOk:
	mov	dword[di],	0xFFFFFFFF	;ARDStruct������־
;----------------------------------------
;��ʼ��VESA
	mov	ax,	0x4F00		;���VESA��Ϣ
	mov	di,	VbeInfo
	int	0x10
	cmp	ax,	0x004F
	jne	VesaFail
	mov	cx,	[VesaMode]	;�����ʾģʽ��
	cmp	cx,	0
	je	VesaOk			;��������
	mov	ax,	0x4F01		;���ģʽ��Ϣ
	mov	di,	ModeInfo
	int	0x10
	mov	ax,	0x4F02		;������ʾģʽ
	mov	bx,	[VesaMode]
	or	bx,	0x4000
	int	0x10
	cmp	ax,	0x004F
	je	VesaOk
VesaFail:
	mov	si,	VesaMsg
	call	Print
	jmp	short	$
VesaMsg	DB	"VESA INIT FAIL!",0
VesaOk:
;----------------------------------------
;ȡ��Ӳ�̲���,�ο�linux 0.11
	mov	di,	HdInfo
	xor	ax,	ax
	mov	ds,	ax
	lds	si,	[0x41 * 4]	;�ж�����0x41��Ӳ��0������ĵ�ַ
	mov	cx,	4
	rep	movsd
	mov	ds,	ax
	lds	si,	[0x46 * 4]	;�ж�����0x46��Ӳ��1������ĵ�ַ
	mov	cx,	4
	rep	movsd

	mov	ax,	0x1500
	mov	dl,	0x81
	int	0x13
	jc	short	Disk1None
	cmp	ah,	3		;��Ӳ����?
	je	short	Disk1Ok
Disk1None:
	xor	eax,	eax
	mov	di,	HdInfo + 16
	mov	cx,	4
	rep	stosd
Disk1Ok:
;----------------------------------------
;��A20��ַ��
	in	al,	0x92		;Fast����
	or	al,	0x02
	out	0x92,	al
	mov	ax,	0xFFFF		;����Ƿ��Ѵ�
	mov	ds,	ax
	mov	word[0xFFFE],	0xABCD
	xor	ax,	ax
	mov	ds,	ax
	cmp	word[0xFFEE],	0xABCD
	jne	A20LineOk
	call	Wait8042		;8042��������
	mov	al,	0xD1
	out	0x64,	al
	call	Wait8042
	mov	al,	0xDF
	out	0x60,	al
	call	Wait8042
A20LineOk:
;----------------------------------------
	cli			;��ֹ�жϣ��Ӵ˲���ʹ���жϣ�ֱ���ں˴�
	cld			;�������
;----------------------------------------
;����GDT
	mov	ax,	cs	;����GDT
	mov	ds,	ax
	mov	si,	GDT
	mov	ax,	GDTseg
	mov	es,	ax
	mov	di,	GDToff
	mov	cx,	GDTN / 4
	rep	movsd
	xor	eax,	eax	;����������0
	mov	cx,	(GDTsize - GDTN) / 4
	rep	stosd
;----------------------------------------
;����IDT
	mov	ax,	IDTseg	;���б�����0
	mov	es,	ax
	mov	di,	IDToff
	xor	eax,	eax
	mov	cx,	IDTsize / 4
	rep	stosd
;----------------------------------------
;����ҳĿ¼��
	mov	ax,	KPDTseg
	mov	es,	ax
	mov	di,	KPDToff
	mov	eax,	0x0000D063	;�ں�ҳ0
	stosd
	mov	eax,	0x0000E063	;�ں�ҳ1
	stosd
	mov	eax,	KPDTseg * 0x10 + KPDToff + 0x1063	;PDDT(�ں˷��ʵ�4K�ֲ�ҳ��)
	stosd
	xor	eax,	eax	;����������0
	mov	cx,	KPDTsize / 4 - 3
	rep	stosd
;----------------------------------------
;����ҳ��
	mov	di,	0xD000
	mov	cx,	0x800	;��2048ҳ �Ե�ӳ��
	mov	eax,	0x0063
PteLop:	stosd
	add	eax,	0x1000
	loop	PteLop
;----------------------------------------
;����idt,gdt,pde
	mov	ax,	cs
	mov	ds,	ax
	lgdt	[GDTaddr]
	lidt	[IDTaddr]
	mov	eax,	KPDTseg * 0x10 + KPDToff
	mov	cr3,	eax
;----------------------------------------
;����32λ��ҳ����ģʽ
	mov	eax,	cr0
	or	eax,	0x80000001
	mov	cr0,	eax
;----------------------------------------
;���뱣��ģʽ
	jmp	dword	8:KNLoff
;----------------------------------------
Print:		;��ʾ��Ϣ����
	mov	ah,	0x0E		;��ʾģʽ
	mov	bx,	0x0007		;��������
PrintNext:
	lodsb
	or	al,	al
	jz	short	PrintEnd
	int	0x10			;��ʾ
	jmp	short	PrintNext
PrintEnd:
	ret
;----------------------------------------
Wait8042:	;�ȴ����̿��������е��ӳ���
	in	al,	0x64
	test	al,	0x2
	jnz	short	Wait8042
	ret
;----------------------------------------
;GDT,IDT
GDT:;0
	DD	0
	DD	0
KNLcode:;8
	DW	0x001F	;�ں˴����0��128KB
	DW	0
	DB	0
	DB	10011001b
	DB	11000000b
	DB	0
KNLdata:;16
	DW	0xFFFF	;�ں����ݶ�0��4GB
	DW	0
	DB	0
	DB	10010011b
	DB	11001111b
	DB	0
GDTN	equ	$ - GDT	;����GDT���ֽ���
GDTaddr:
	DW	GDTsize - 1	;GDT��С
	DD	GDTseg * 0x10 +	GDToff	;GDT���Ե�ַ
IDTaddr:
	DW	IDTsize - 1	;IDT��С
	DD	IDTseg * 0x10 +	IDToff	;IDT���Ե�ַ

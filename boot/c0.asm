;	C0.ASM for ulios
;	���ߣ�����
;	���ܣ�TC������Ԥ����ͷ����
;	����޸����ڣ�2009-05-19
;	��ע��ʹ��TASM��������C0T.OBJ��������TC��д16λ��������
;	TASM C0,C0T /D__TINY__ /MX;

	.186
	EXTRN	_main:	NEAR
_TEXT	SEGMENT	BYTE	PUBLIC	'CODE'
_TEXT	ENDS
_DATA	SEGMENT	PARA	PUBLIC	'DATA'
_DATA	ENDS
_BSS	SEGMENT	WORD	PUBLIC	'BSS'
_BSS	ENDS
DGROUP	GROUP	_TEXT,	_DATA,	_BSS	;����κ����ݶι���һ����
ASSUME	CS:_TEXT

_TEXT	SEGMENT
	ORG	100h
START	PROC	NEAR
	jmp	START2
	DB	3837	DUP(0)
START2:
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	-4096		;��ջ����0xF000
	xor	ax,	ax
	push	ax
	call	_main
START	ENDP
_TEXT	ENDS
	END	START

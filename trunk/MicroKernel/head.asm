;head.asm for ulios
;���ߣ�����
;���ܣ��ں˾�����ڴ���,�����ں�C����main����
;����޸����ڣ�2009-05-28

[BITS 32]
global _start
extern main
_start:		;��ʼ���μĴ���
	mov	ax,	0x10	;���ݶ�ѡ����
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax
	mov	ss,	ax
	mov	esp,	0x0009F000

	call	main
HltLoop:
	hlt
	jmp	short	HltLoop
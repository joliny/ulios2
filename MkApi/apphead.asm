;apphead.asm for ulios program
;���ߣ�����
;���ܣ�Ӧ�ó������ڴ���,���ó����C����main����
;����޸����ڣ�2009-09-18

[BITS 32]
global _start
extern main
_start:		;��ʼ���μĴ���
	mov	ax,	ss	;�������ݶ�ѡ����
	mov	ds,	ax
	mov	es,	ax
	mov	fs,	ax
	mov	gs,	ax

	call	main
	mov	eax,	0x070000	;�˳�����
	int	0xF0

;	C0.ASM for ulios
;	���ߣ�����
;	���ܣ�TC������Ԥ����ͷ����
;	����޸����ڣ�2009-05-19
;	��ע��ʹ��TASM��������C0T.OBJ��������TC��д16λ��������
;	TASM C0,C0T /D__TINY__ /MX;

	.186
	EXTRN	_main:	NEAR
	ASSUME	CS:_TEXT
_TEXT	SEGMENT
	ORG	100h
START	PROC	NEAR
	mov	ax,	cs
	mov	ds,	ax
	mov	es,	ax
	mov	ss,	ax
	mov	sp,	-4096
	xor	ax,	ax
	push	ax
	call	_main
START	ENDP
_TEXT	ENDS
	END	START

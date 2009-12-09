;	C0.ASM for ulios
;	作者：孙亮
;	功能：TC编译器预编译头代码
;	最后修改日期：2009-05-19
;	备注：使用TASM编译生成C0T.OBJ，用于用TC编写16位启动程序
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

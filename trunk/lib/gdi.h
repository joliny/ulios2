/*	gdi.h for ulios
	���ߣ�����
	���ܣ�ͼ���豸�ӿڿⶨ��
	����޸����ڣ�2010-06-10
*/

#ifndef	_GDI_H_
#define	_GDI_H_

#include "../driver/basesrv.h"

#define GDI_ERR_LOCATION	-1408	/*�������*/
#define GDI_ERR_SIZE		-1409	/*�ߴ����*/

extern void *GDIvm;
extern const BYTE *GDIfont;
extern DWORD GDIwidth, GDIheight, GDIPixBits;
extern DWORD GDICharWidth, GDICharHeight;
extern THREAD_ID GDIVesaPtid;

/*��ʼ��GDI��*/
long GDIinit();

/*����GDI��*/
void GDIrelease();

/*����*/
long GDIPutPixel(DWORD x, DWORD y, DWORD c);

/*ȡ��*/
long GDIGetPixel(DWORD x, DWORD y, DWORD *c);

/*��ͼ*/
long GDIPutImage(long x, long y, DWORD *img, long w, long h);

/*ȥ����ɫ��ͼ*/
long GDIPutBCImage(long x, long y, DWORD *img, long w, long h, DWORD bc);

/*������ͼ*/
long GDIBitBlt(long x, long y, DWORD *img, long w, long h, long subx, long suby, long subw, long subh);

/*��ͼ*/
long GDIGetImage(long x, long y, DWORD *img, long w, long h);

/*������*/
long GDIFillRect(long x, long y, long w, long h, DWORD c);

/*���Ϲ���*/
long GDIMoveUp(DWORD pix);

/*Cohen-Sutherland�㷨�ü�Bresenham�Ľ��㷨����*/
long GDIDrawLine(long x1, long y1, long x2, long y2, DWORD c);

/*Bresenham�㷨��Բ*/
long GDIcircle(long cx, long cy, long r, DWORD c);

/*��ʾ����*/
long GDIDrawHz(long x, long y, DWORD hz, DWORD c);

/*��ʾASCII�ַ�*/
long GDIDrawAscii(long x, long y, DWORD ch, DWORD c);

/*����ַ���*/
long GDIDrawStr(long x, long y, const char *str, DWORD c);

#endif

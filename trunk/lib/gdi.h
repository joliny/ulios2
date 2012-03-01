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
#define GDI_ERR_TEXTMODE	-1410	/*�ı�ģʽ*/

extern void *GDIvm;
extern const BYTE *GDIfont;
extern DWORD GDIwidth, GDIheight, GDIPixBits, GDImode;
extern DWORD GDICharWidth, GDICharHeight;

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

/*��ͼ*/
long GDIGetImage(long x, long y, DWORD *img, long w, long h);

/*������*/
long GDIFillRect(long x, long y, long w, long h, DWORD c);

/*����*/
long GDIDrawLine(long x1, long y1, long x2, long y2, DWORD c);

/*��Բ*/
long GDIcircle(long cx, long cy, long r, DWORD c);

/*���ƺ���*/
long GDIDrawHz(long x, long y, DWORD hz, DWORD c);

/*����ASCII�ַ�*/
long GDIDrawAscii(long x, long y, DWORD ch, DWORD c);

/*�����ַ���*/
long GDIDrawStr(long x, long y, const char *str, DWORD c);

#endif

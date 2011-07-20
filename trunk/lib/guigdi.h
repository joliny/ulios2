/*	guigdi.h for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û������ͼ�ⶨ��
	����޸����ڣ�2010-06-10
*/

#ifndef	_GUIGDI_H_
#define	_GUIGDI_H_

#include "../driver/basesrv.h"

#define UDI_ERR_TEXTMODE	-1408	/*�ı�ģʽ*/
#define UDI_ERR_LOCATION	-1409	/*�������*/
#define UDI_ERR_SIZE		-1410	/*�ߴ����*/

extern const BYTE *GDIfont;
extern DWORD GDICharWidth, GDICharHeight;
extern THREAD_ID UDIGuiPtid, GDIFontPtid;

/*��ʼ��UDI��*/
long UDIinit();

/*����UDI��*/
void UDIrelease();

/*����*/
long UDIPutPixel(DWORD x, DWORD y, DWORD c);

/*ȡ��*/
long UDIGetPixel(DWORD x, DWORD y, DWORD *c);

/*��ͼ*/
long UDIPutImage(long x, long y, DWORD *img, long w, long h);

/*ȥ����ɫ��ͼ*/
long UDIPutBCImage(long x, long y, DWORD *img, long w, long h, DWORD bc);

/*��ͼ*/
long UDIGetImage(long x, long y, DWORD *img, long w, long h);

/*������*/
long UDIFillRect(long x, long y, long w, long h, DWORD c);

/*���Ϲ���*/
long UDIMoveUp(DWORD pix);

/*Cohen-Sutherland�㷨�ü�Bresenham�Ľ��㷨����*/
long UDIDrawLine(long x1, long y1, long x2, long y2, DWORD c);

/*Bresenham�㷨��Բ*/
long UDIcircle(long cx, long cy, long r, DWORD c);

/*��ʾ����*/
long UDIDrawHz(long x, long y, DWORD hz, DWORD c);

/*��ʾASCII�ַ�*/
long UDIDrawAscii(long x, long y, DWORD ch, DWORD c);

/*����ַ���*/
long UDIDrawStr(long x, long y, const char *str, DWORD c);

#endif

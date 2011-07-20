/*	guigdi.h for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面绘图库定义
	最后修改日期：2010-06-10
*/

#ifndef	_GUIGDI_H_
#define	_GUIGDI_H_

#include "../driver/basesrv.h"

#define UDI_ERR_TEXTMODE	-1408	/*文本模式*/
#define UDI_ERR_LOCATION	-1409	/*坐标错误*/
#define UDI_ERR_SIZE		-1410	/*尺寸错误*/

extern const BYTE *GDIfont;
extern DWORD GDICharWidth, GDICharHeight;
extern THREAD_ID UDIGuiPtid, GDIFontPtid;

/*初始化UDI库*/
long UDIinit();

/*撤销UDI库*/
void UDIrelease();

/*画点*/
long UDIPutPixel(DWORD x, DWORD y, DWORD c);

/*取点*/
long UDIGetPixel(DWORD x, DWORD y, DWORD *c);

/*贴图*/
long UDIPutImage(long x, long y, DWORD *img, long w, long h);

/*去背景色贴图*/
long UDIPutBCImage(long x, long y, DWORD *img, long w, long h, DWORD bc);

/*截图*/
long UDIGetImage(long x, long y, DWORD *img, long w, long h);

/*填充矩形*/
long UDIFillRect(long x, long y, long w, long h, DWORD c);

/*向上滚屏*/
long UDIMoveUp(DWORD pix);

/*Cohen-Sutherland算法裁剪Bresenham改进算法画线*/
long UDIDrawLine(long x1, long y1, long x2, long y2, DWORD c);

/*Bresenham算法画圆*/
long UDIcircle(long cx, long cy, long r, DWORD c);

/*显示汉字*/
long UDIDrawHz(long x, long y, DWORD hz, DWORD c);

/*显示ASCII字符*/
long UDIDrawAscii(long x, long y, DWORD ch, DWORD c);

/*输出字符串*/
long UDIDrawStr(long x, long y, const char *str, DWORD c);

#endif

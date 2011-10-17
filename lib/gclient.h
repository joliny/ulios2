/*	gclient.h for ulios graphical user interface
	作者：孙亮
	功能：GUI客户端功能库定义
	最后修改日期：2011-08-15
*/

#ifndef	_GCLIENT_H_
#define	_GCLIENT_H_

#include "../gui/guiapi.h"

/**********绘图功能定义**********/

#define GC_ERR_LOCATION			-2688	/*坐标错误*/
#define GC_ERR_AREASIZE			-2689	/*尺寸错误*/
#define GC_ERR_OUT_OF_MEM		-2690	/*内存不足*/
#define GC_ERR_INVALID_GUIMSG	-2691	/*非法GUI消息*/

typedef struct _UDI_AREA
{
	DWORD width, height;	/*尺寸*/
	DWORD *vbuf;			/*绘图缓冲*/
	struct _UDI_AREA *root;	/*根绘图区*/
}UDI_AREA;	/*GUI绘图区域*/

extern const BYTE *GCfont;
extern DWORD GCwidth, GCheight;
extern DWORD GCCharWidth, GCCharHeight;
extern THREAD_ID GCGuiPtid, GCFontPtid;

/*初始化GC库*/
long GCinit();

/*撤销GC库*/
void GCrelease();

/*设置绘图区域的参数和缓存,第一次设置无根绘图区时先清空uda->vbuf*/
long GCSetArea(UDI_AREA *uda, DWORD width, DWORD height, const UDI_AREA *par, long x, long y);

/*回收绘图区域缓存*/
void GCFreeArea(UDI_AREA *uda);

/*画点*/
long GCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c);

/*取点*/
long GCGetPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD *c);

/*贴图*/
long GCPutImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h);

/*去背景色贴图*/
long GCPutBCImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h, DWORD bc);

/*截图*/
long GCGetImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h);

/*填充矩形*/
long GCFillRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c);

/*画线*/
long GCDrawLine(UDI_AREA *uda, long x1, long y1, long x2, long y2, DWORD c);

/*画圆*/
long GCcircle(UDI_AREA *uda, long cx, long cy, long r, DWORD c);

/*绘制汉字*/
long GCDrawHz(UDI_AREA *uda, long x, long y, DWORD hz, DWORD c);

/*绘制ASCII字符*/
long GCDrawAscii(UDI_AREA *uda, long x, long y, DWORD ch, DWORD c);

/*绘制字符串*/
long GCDrawStr(UDI_AREA *uda, long x, long y, const char *str, DWORD c);

/*加载BMP图像文件*/
long GCLoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height);

/**********GUI客户端自定义消息**********/

#define GM_CLOSE		0x8000	/*关闭窗体消息*/

/**********控件基类**********/

/*控件类型代码*/
#define GC_CTRL_TYPE_DESKTOP	0		/*桌面*/
#define GC_CTRL_TYPE_WINDOW		1		/*窗口*/
#define GC_CTRL_TYPE_BUTTON		2		/*按钮*/
#define GC_CTRL_TYPE_TEXT		3		/*文本框*/

typedef long (*MSGPROC)(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);	/*窗体消息处理函数*/

typedef struct _CTRL_ARGS
{
	DWORD width, height;	/*窗体尺寸*/
	long x, y;				/*窗体位置*/
	WORD type, style;		/*类型,样式*/
	MSGPROC MsgProc;		/*消息处理函数*/
}CTRL_ARGS;	/*控件参数*/

typedef struct _CTRL_GOBJ
{
	UDI_AREA uda;			/*窗体绘图区域*/
	DWORD gid;				/*GUI服务端对象ID*/
	struct _CTRL_GOBJ *pre, *nxt;	/*兄/弟控件链指针*/
	struct _CTRL_GOBJ *par, *chl;	/*父/子控件链指针*/
	long x, y;				/*窗体位置*/
	WORD type, style;		/*类型,样式*/
	MSGPROC MsgProc;		/*消息处理函数*/
	void (*DrawProc)(struct _CTRL_GOBJ *gobj);	/*绘制处理函数*/
}CTRL_GOBJ;	/*控件基类*/

typedef void (*DRAWPROC)(CTRL_GOBJ *gobj);							/*窗体绘制处理函数*/

/*初始化CTRL_GOBJ结构*/
long GCGobjInit(CTRL_GOBJ *gobj, const CTRL_ARGS *args, MSGPROC MsgProc, DRAWPROC DrawProc, DWORD pid, CTRL_GOBJ *ParGobj);

/*删除窗体树*/
void GCGobjDelete(CTRL_GOBJ *gobj);

/*绘制窗体树*/
void GCGobjDraw(CTRL_GOBJ *gobj);

/*GUI客户端消息调度*/
long GCDispatchMsg(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

/**********桌面**********/

typedef struct _CTRL_DSK
{
	CTRL_GOBJ obj;
}CTRL_DSK;	/*桌面类*/

long GCDskCreate(CTRL_DSK **dsk, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj);

long GCDskDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCDskDefDrawProc(CTRL_DSK *dsk);

/**********按钮**********/

#define BTN_TXT_LEN			64

typedef struct _CTRL_BTN
{
	CTRL_GOBJ obj;
	char text[BTN_TXT_LEN];			/*按钮文本*/
	BOOL isPressDown;				/*是否已按下*/
	void (*PressProc)(struct _CTRL_BTN *btn);	/*点击处理函数*/
}CTRL_BTN;	/*按钮类*/

long GCBtnCreate(CTRL_BTN **btn, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, void (*PressProc)(CTRL_BTN *btn));

long GCBtnDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCBtnDefDrawProc(CTRL_BTN *btn);

/*设置窗口标题文本*/
void GCBtnSetText(CTRL_BTN *btn, const char *text);

/**********窗口**********/

#define WND_CAP_LEN			128

#define WND_STYLE_CAPTION	0x0001	/*窗体标题栏*/
#define WND_STYLE_BORDER	0x0002	/*窗体边框*/
#define WND_STYLE_CLOSEBTN	0x0004	/*关闭按钮*/
#define WND_STYLE_MAXBTN	0x0008	/*最大化按钮*/
#define WND_STYLE_MINBTN	0x0010	/*最小化按钮*/
#define WND_STYLE_SIZEBTN	0x0020	/*拖动缩放按钮*/

typedef struct _CTRL_WND
{
	CTRL_GOBJ obj;
	char caption[WND_CAP_LEN];		/*标题文本*/
	CTRL_BTN *close;				/*关闭按钮*/
	CTRL_BTN *max;					/*最大化按钮*/
	CTRL_BTN *min;					/*最小化按钮*/
	CTRL_BTN *size;					/*缩放按钮*/
	UDI_AREA client;				/*客户绘图区*/
	long x0, y0;					/*正常位置,大小*/
	DWORD width0, height0;
}CTRL_WND;	/*窗口类*/

long GCWndCreate(CTRL_WND **wnd, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *caption);

long GCWndDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCWndDefDrawProc(CTRL_WND *wnd);

/*设置窗口标题文本*/
void GCWndSetCaption(CTRL_WND *wnd, const char *caption);

/*取得窗口客户区位置*/
void GCWndGetClientLoca(CTRL_WND *wnd, long *x, long *y);

/**********文本框**********/

#define TXT_TEXT_LEN		128

#endif

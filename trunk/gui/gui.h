/*	gui.h for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面相关结构、常量定义
	最后修改日期：2010-10-03
*/

#ifndef	_GUI_H_
#define	_GUI_H_

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "guiapi.h"

/**********图形用户界面结构定义**********/

typedef struct _RECT
{
	long xpos, ypos, xend, yend;
}RECT;	/*矩形结构*/

#define CLIPRECTT_LEN	0x2000		/*剪切矩形管理表长度*/

typedef struct _CLIPRECT
{
	RECT rect;						/*剪切矩形*/
	struct _CLIPRECT *nxt;			/*后续指针*/
}CLIPRECT;	/*可视剪切矩形节点,用于窗体的重叠控制*/

#define GOBJT_LEN		0x4000		/*窗体描述符管理表长度*/

typedef struct _GOBJ_DESC
{
	THREAD_ID ptid;					/*所属线程ID*/
	DWORD attr;						/*属性*/
	RECT rect;						/*相对父窗体的位置*/
	DWORD *vbuf;					/*可视内存缓冲,NULL使用父辈窗体的缓冲*/
	struct _GOBJ_DESC *pre, *nxt;	/*兄/弟对象链指针*/
	struct _GOBJ_DESC *par, *chl;	/*父/子对象链指针*/
	CLIPRECT *ClipList;				/*自身剪切矩形列表*/
}GOBJ_DESC;	/*GUI对象(窗体)描述符*/

/**********剪切矩形管理相关**********/

/*掩盖窗体的矩形外部*/
long CoverRectExter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*掩盖窗体的矩形内部*/
long CoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*显露窗体的矩形内部*/
long DiscoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*被祖父和祖伯父窗体覆盖*/
void CoverRectByPar(GOBJ_DESC *gobj);

/*删除剪切矩形链表*/
long DeleteClipList(GOBJ_DESC *gobj);

/*绘制窗体矩形内部*/
void DrawGobj(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend, long AbsXpos, long AbsYpos, GOBJ_DESC *ExcludeGobj);

/**********窗体管理相关**********/

/*根据绝对坐标查找窗体*/
GOBJ_DESC *FindGobj(long *AbsXpos, long *AbsYpos);

/*创建主桌面*/
long CreateDesktop(THREAD_ID ptid, DWORD attr, long width, long height, DWORD *vbuf);

/*创建窗体*/
long CreateGobj(THREAD_ID ptid, DWORD pid, DWORD attr, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len, DWORD *gid);

/*删除窗体*/
long DeleteGobj(THREAD_ID ptid, DWORD gid);

/*设置窗体的位置大小*/
long MoveGobj(THREAD_ID ptid, DWORD gid, long xpos, long ypos);

/*设置窗体的大小*/
long SizeGobj(THREAD_ID ptid, DWORD gid, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len);

/*设置窗体为活动*/
long ActiveGobj(GOBJ_DESC *gobj);

/*设置窗体显示缓冲*/
long SetGobjVbuf(GOBJ_DESC *gobj, DWORD *vbuf);

/**********功能库相关**********/

/*GUI矩形块贴图*/
void GuiPutImage(long x, long y, DWORD *img, long memw, long w, long h);

/*加载BMP图像文件*/
long LoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height);

/**********桌面线程相关**********/

/*桌面消息处理线程*/
void DesktopThread(GOBJ_DESC *gobj);

/**********鼠标功能相关**********/

/*鼠标初始化*/
long InitMouse();

/*计算鼠标指针与区域是否重叠*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend);

/*隐藏鼠标指针*/
void HidMouse();

/*显示鼠标指针*/
void ShowMouse();

/*鼠标消息处理*/
void MouseProc(DWORD key, long x, long y, long z);

#endif

/*	desktop.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面桌面处理线程
	最后修改日期：2011-05-21
*/

#include "gui.h"
#include "../lib/gdi.h"

#define DESKTOP_MAXLEN	0x140000	/*桌面背景数组大小1280*1024*/

void DesktopThread(void *args)
{
	DWORD DesktopPic[DESKTOP_MAXLEN];
	THREAD_ID ptid;
	long res;

	KGetPtid(&ptid, (DWORD*)&res, (DWORD*)&res);
	if (GDIwidth * GDIheight <= DESKTOP_MAXLEN && LoadBmp("desktop.bmp", DesktopPic, DESKTOP_MAXLEN, NULL, NULL) == NO_ERROR)
	{
		if ((res = CreateDesktop(ptid, 0, GDIwidth, GDIheight, DesktopPic)) != NO_ERROR)	/*创建有背景桌面*/
			return;
	}
	else
	{
		if ((res = CreateDesktop(ptid, 0, GDIwidth, GDIheight, NULL)) != NO_ERROR)	/*创建无背景桌面*/
			return;
	}

	for (;;)
		KSleep(1000);
}

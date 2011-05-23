/*	desktop.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û��������洦���߳�
	����޸����ڣ�2011-05-21
*/

#include "gui.h"
#include "../lib/gdi.h"

#define DESKTOP_MAXLEN	0x140000	/*���汳�������С1280*1024*/

void DesktopThread(void *args)
{
	DWORD DesktopPic[DESKTOP_MAXLEN];
	THREAD_ID ptid;
	long res;

	KGetPtid(&ptid, (DWORD*)&res, (DWORD*)&res);
	if (GDIwidth * GDIheight <= DESKTOP_MAXLEN && LoadBmp("desktop.bmp", DesktopPic, DESKTOP_MAXLEN, NULL, NULL) == NO_ERROR)
	{
		if ((res = CreateDesktop(ptid, 0, GDIwidth, GDIheight, DesktopPic)) != NO_ERROR)	/*�����б�������*/
			return;
	}
	else
	{
		if ((res = CreateDesktop(ptid, 0, GDIwidth, GDIheight, NULL)) != NO_ERROR)	/*�����ޱ�������*/
			return;
	}

	for (;;)
		KSleep(1000);
}

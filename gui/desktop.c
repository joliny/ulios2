/*	desktop.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面桌面处理线程
	最后修改日期：2011-05-21
*/

#include "gui.h"
#include "../lib/gdi.h"

#define DESKTOP_MAXLEN	0x1A0000	/*桌面背景数组大小*/

/*桌面消息处理线程*/
void DesktopThread(void *args)
{
	DWORD data[MSG_DATA_LEN];
	THREAD_ID ptid;
	DWORD tid, pid;
	DWORD DesktopPic[DESKTOP_MAXLEN];

	LoadBmp("desktop.bmp", DesktopPic, DESKTOP_MAXLEN, NULL, NULL);
	KGetPtid(&ptid, &tid, &pid);
	CreateDesktop(ptid, 0, GDIwidth, GDIheight, DesktopPic);
	ptid.ThedID = tid;	/*通知主线程初始化完成*/
	data[MSG_ATTR_ID] = MSG_ATTR_USER;
	KSendMsg(&ptid, data, 0);

	for (;;)
	{
		if (KRecvMsg(&ptid, data, INVALID) != NO_ERROR)	/*等待消息*/
			break;
	}
}

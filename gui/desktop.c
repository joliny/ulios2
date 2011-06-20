/*	desktop.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面桌面处理线程
	最后修改日期：2011-05-21
*/

#include "gui.h"

#define DESKTOP_MAXLEN	0x140000	/*桌面背景数组大小1280*1024*/

/*桌面消息处理线程*/
void DesktopThread(GOBJ_DESC *gobj)
{
	DWORD DesktopPic[DESKTOP_MAXLEN];

	if (LoadBmp("desktop.bmp", DesktopPic, DESKTOP_MAXLEN, NULL, NULL) == NO_ERROR)
		SetGobjVbuf(gobj, DesktopPic);

	for (;;)
	{
		DWORD data[MSG_DATA_LEN];
		THREAD_ID ptid;

		if (KRecvMsg(&ptid, data, INVALID) != NO_ERROR)	/*等待消息*/
			break;
	}
}

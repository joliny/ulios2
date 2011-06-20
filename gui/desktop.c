/*	desktop.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û��������洦���߳�
	����޸����ڣ�2011-05-21
*/

#include "gui.h"

#define DESKTOP_MAXLEN	0x140000	/*���汳�������С1280*1024*/

/*������Ϣ�����߳�*/
void DesktopThread(GOBJ_DESC *gobj)
{
	DWORD DesktopPic[DESKTOP_MAXLEN];

	if (LoadBmp("desktop.bmp", DesktopPic, DESKTOP_MAXLEN, NULL, NULL) == NO_ERROR)
		SetGobjVbuf(gobj, DesktopPic);

	for (;;)
	{
		DWORD data[MSG_DATA_LEN];
		THREAD_ID ptid;

		if (KRecvMsg(&ptid, data, INVALID) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
	}
}

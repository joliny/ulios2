/*	desktop.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û��������洦���߳�
	����޸����ڣ�2011-05-21
*/

#include "gui.h"
#include "../lib/gdi.h"

#define DESKTOP_MAXLEN	0x1A0000	/*���汳�������С*/

/*������Ϣ�����߳�*/
void DesktopThread(void *args)
{
	DWORD data[MSG_DATA_LEN];
	THREAD_ID ptid;
	DWORD tid, pid;
	DWORD DesktopPic[DESKTOP_MAXLEN];

	LoadBmp("desktop.bmp", DesktopPic, DESKTOP_MAXLEN, NULL, NULL);
	KGetPtid(&ptid, &tid, &pid);
	CreateDesktop(ptid, 0, GDIwidth, GDIheight, DesktopPic);
	ptid.ThedID = tid;	/*֪ͨ���̳߳�ʼ�����*/
	data[MSG_ATTR_ID] = MSG_ATTR_USER;
	KSendMsg(&ptid, data, 0);

	for (;;)
	{
		if (KRecvMsg(&ptid, data, INVALID) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
	}
}

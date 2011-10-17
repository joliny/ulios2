/*	guitest.c for ulios application
	���ߣ�����
	���ܣ�GUI���Գ���
	����޸����ڣ�2010-12-11
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

long CliX, CliY, x0, y0;

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		GCWndGetClientLoca(wnd, &CliX, &CliY);
		break;
	case GM_LBUTTONDOWN:
		x0 = (data[5] & 0xFFFF) - CliX;
		y0 = (data[5] >> 16) - CliY;
		break;
	case GM_MOUSEMOVE:
		if (data[1] & MUS_STATE_LBUTTON)
		{
			GCDrawLine(&wnd->client, x0, y0, (data[5] & 0xFFFF) - CliX, (data[5] >> 16) - CliY, 0);
			x0 = (data[5] & 0xFFFF) - CliX;
			y0 = (data[5] >> 16) - CliY;
			GUIpaint(GCGuiPtid, wnd->obj.gid, 0, 0, wnd->obj.uda.width, wnd->obj.uda.height);
		}
		break;
	case GM_LBUTTONDBCLK:
		GCWndSetCaption(wnd, "������ţ�");
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_WND *wnd;
	CTRL_ARGS args;
	long res;

	InitMallocTab(0x1000000);	/*����16MB���ڴ�*/
	GCinit();
	args.width = 256;
	args.height = 128;
	args.x = 100;
	args.y = 100;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN | WND_STYLE_MAXBTN | WND_STYLE_MINBTN | WND_STYLE_SIZEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "hello����");

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (GCDispatchMsg(ptid, data) == GC_ERR_INVALID_GUIMSG)	/*��GUI��Ϣ���д���*/
			;
		else if ((data[MSG_ATTR_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*����������,�˳�����*/
			break;
	}
	return NO_ERROR;
}

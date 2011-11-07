/*	guitest.c for ulios application
	作者：孙亮
	功能：GUI测试程序
	最后修改日期：2010-12-11
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

CTRL_TXT *txt;
CTRL_SEDT *edt;
long CliX, CliY, x0, y0;

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		GCWndGetClientLoca(wnd, &CliX, &CliY);
		{
			CTRL_ARGS args;
			args.width = 100;
			args.height = 16;
			args.x = CliX + (wnd->client.width - 100) / 2;
			args.y = CliY + (wnd->client.height - 16) / 2 - 10;
			args.style = 0;
			args.MsgProc = NULL;
			GCTxtCreate(&txt, &args, wnd->obj.gid, &wnd->obj, "hello ulios2 gui");
			args.y += 20;
			GCSedtCreate(&edt, &args, wnd->obj.gid, &wnd->obj, "hello", NULL);
		}
		break;
	case GM_SIZE:
		txt->obj.x = CliX + (wnd->client.width - 100) / 2;
		txt->obj.y = CliY + (wnd->client.height - 16) / 2 - 10;
		GCSetArea(&txt->obj.uda, 100, 16, &wnd->obj.uda, txt->obj.x, txt->obj.y);
		GCTxtDefDrawProc(txt);
		GUImove(GCGuiPtid, txt->obj.gid, txt->obj.x, txt->obj.y);
		edt->obj.x = CliX + (wnd->client.width - 100) / 2;
		edt->obj.y = CliY + (wnd->client.height - 16) / 2 + 10;
		GCSetArea(&edt->obj.uda, 100, 16, &wnd->obj.uda, edt->obj.x, edt->obj.y);
		GCSedtDefDrawProc(edt);
		GUImove(GCGuiPtid, edt->obj.gid, edt->obj.x, edt->obj.y);
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
		GCWndSetCaption(wnd, "哈喽老婆！");
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_WND *wnd;
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = 128;
	args.height = 80;
	args.x = 100;
	args.y = 100;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN | WND_STYLE_MAXBTN | WND_STYLE_MINBTN | WND_STYLE_SIZEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "hello窗口");

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
		{
			if ((data[MSG_ATTR_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*销毁主窗体,退出程序*/
				break;
		}
	}
	return NO_ERROR;
}

/*	guiapi.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面接口，响应应用程序的请求，执行服务
	最后修改日期：2010-10-05
*/

#include "gui.h"
#include "../lib/gdi.h"

extern GOBJ_DESC gobjt[];	/*窗体描述符管理表*/
extern GOBJ_DESC *FstGobj;	/*窗体描述符管理表指针*/
extern CLIPRECT ClipRectt[];	/*剪切矩形管理表*/
extern CLIPRECT *FstClipRect;	/*剪切矩形管理表指针*/

THREAD_ID KbdMusPtid;	/*键盘鼠标服务ID*/

/*初始化GUI,如果不成功必须退出*/
long InitGUI()
{
	long res;
	GOBJ_DESC *gobj;
	CLIPRECT *clip;
	THREAD_ID ptid;

	if ((res = KRegKnlPort(SRV_GUI_PORT)) != NO_ERROR)	/*注册服务端口*/
		return res;
	if ((res = GDIinit()) != NO_ERROR)	/*初始化GDI*/
		return res;
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &KbdMusPtid)) != NO_ERROR)	/*取得键盘鼠标服务线程*/
		return res;
	if ((res = KMSetRecv(KbdMusPtid)) != NO_ERROR)	/*取得键盘鼠标消息*/
		return res;

	for (gobj = FstGobj = gobjt; gobj < &gobjt[GOBJT_LEN - 1]; gobj++)
	{
		gobj->attr = INVALID;
		gobj->nxt = gobj + 1;
	}
	gobj->attr = INVALID;
	gobj->nxt = NULL;
	for (clip = FstClipRect = ClipRectt; clip < &FstClipRect[CLIPRECTT_LEN - 1]; clip++)
		clip->nxt = clip + 1;
	clip->nxt = NULL;

	KCreateThread(DesktopThread, 0x700000, 0, &ptid);
	KSleep(200);
	InitMouse();
	return NO_ERROR;
}

/*鼠标键5种状态0放松1按下2按住3松开4单击*/
#define MOUK_FREE	0
#define MOUK_DOWN	1
#define MOUK_DRAG	2
#define MOUK_UP		3
#define MOUK_PRESS	4
BYTE mouk[4];

void chkmou(DWORD k)	/*鼠标状态设置*/
{
	DWORD i;

	for (i = 0; i < 3; i++)
		if ((k >> i) & 1)	/*检查3个键的状态*/
			switch (mouk[i])
			{
			case MOUK_DOWN:
				mouk[i] = MOUK_DRAG;
				break;
			case MOUK_FREE:
			case MOUK_UP:
			case MOUK_PRESS:
				mouk[i] = MOUK_DOWN;
				break;
			}
		else	/*没按键*/
			switch (mouk[i])
			{
			case MOUK_DOWN:
				mouk[i] = MOUK_PRESS;
				break;
			case MOUK_DRAG:
				mouk[i] = MOUK_UP;
				break;
			case MOUK_UP:
			case MOUK_PRESS:
				mouk[i] = MOUK_FREE;
				break;
			}
}

/*鼠标消息处理*/
void MouseProc(DWORD key, long x, long y, long z)
{
	if (MouX != x || MouY != y)	/*位置移动*/
	{

	}
}

int main()
{
	long res;

	if ((res = InitGUI()) != NO_ERROR)
		return res;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];
		THREAD_ID ptid;

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (ptid.ProcID == KbdMusPtid.ProcID && data[0] == MSG_ATTR_MUS)	/*鼠标消息*/
		{
			if (MouX == data[2] && MouY == data[3])
				continue;
			SetMousePos(data[2], data[3]);
			{
				static GOBJ_DESC *gobj;
				static long DownX, DownY, WndX, WndY;
				DWORD id;

				chkmou(data[1]);
				switch (mouk[0])
				{
				case MOUK_DOWN:	/*左键按下*/
					WndX = DownX = MouX;
					WndY = DownY = MouY;
					gobj = FindGobj(&WndX, &WndY);
					break;
				case MOUK_DRAG:	/*左键拖动*/
					MoveGobj(gobj, gobj->rect.xpos + MouX - DownX, gobj->rect.ypos + MouY - DownY, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos);
					DownX = MouX;
					DownY = MouY;
					break;
				}
				switch (mouk[1])
				{
				case MOUK_DOWN:	/*右键按下*/
					WndX = DownX = MouX;
					WndY = DownY = MouY;
					gobj = FindGobj(&WndX, &WndY);
					break;
				case MOUK_UP:	/*右键抬起*/
					if (MouX <= DownX || MouY <= DownY)
						DeleteGobj(gobj);
					else
						CreateGobj(KbdMusPtid, 0, gobj - gobjt, WndX, WndY, MouX - DownX, MouY - DownY, &id);
					break;
				}
				switch (mouk[2])
				{
				case MOUK_DOWN:	/*中键按下*/
					WndX = MouX;
					WndY = MouY;
					gobj = FindGobj(&WndX, &WndY);
					ActiveGobj(gobj);
					break;
				}
			}
		}
	}
	GDIrelease();
	KUnregKnlPort(SRV_GUI_PORT);
	return NO_ERROR;
}

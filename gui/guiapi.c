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

#define PTID_ID	MSG_DATA_LEN

void ApiCreate(DWORD *argv)
{
	argv[MSG_RES_ID] = CreateGobj(*(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID], argv[3], (short)argv[4], (short)(argv[4] >> 16), (short)argv[5], (short)(argv[5] >> 16), (argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_GUI ? NULL : (DWORD*)argv[MSG_ADDR_ID], argv[MSG_SIZE_ID] / sizeof(DWORD), &argv[GUIMSG_GOBJ_ID]);
	if (argv[MSG_RES_ID] != NO_ERROR && (argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
	else
	{
		argv[MSG_API_ID] = MSG_ATTR_GUI | GM_CREATE;
		KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
	}
}

void ApiDestroy(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if ((argv[MSG_RES_ID] = DeleteGobj(*(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID])) != NO_ERROR)
		KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiMove(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	if ((argv[MSG_RES_ID] = MoveGobj(*(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID], argv[1], argv[2])) != NO_ERROR)
		KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
}

void ApiSize(DWORD *argv)
{
	if ((argv[MSG_RES_ID] = SizeGobj(*(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID], (short)argv[3], (short)(argv[3] >> 16), (short)argv[4], (short)(argv[4] >> 16), (argv[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_GUI ? NULL : (DWORD*)argv[MSG_ADDR_ID], argv[MSG_SIZE_ID] / sizeof(DWORD))) != NO_ERROR)
	{
		if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
			KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		else
			KSendMsg((THREAD_ID*)&argv[PTID_ID], argv, 0);
	}
}

void ApiPaint(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	PaintGobj(*(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID], (short)argv[1], (short)(argv[1] >> 16), (short)argv[2], (short)(argv[2] >> 16));
}

void ApiActive(DWORD *argv)
{
	if ((argv[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP)
	{
		argv[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
		KUnmapProcAddr((void*)argv[MSG_ADDR_ID], argv);
		return;
	}
	ActiveGobj(*(THREAD_ID*)&argv[PTID_ID], argv[GUIMSG_GOBJ_ID]);
}

/*系统调用表*/
void (*ApiTable[])(DWORD *argv) = {
	ApiCreate, ApiDestroy, ApiMove, ApiSize, ApiPaint, ApiActive
};

/*初始化GUI,如果不成功必须退出*/
long InitGUI()
{
	long res;
	GOBJ_DESC *gobj;
	CLIPRECT *clip;
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];

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

	InitMouse();
	KCreateThread(DesktopThread, 0x700000, NULL, &ptid);	/*创建桌面线程*/
	return KRecvProcMsg(&ptid, data, SRV_OUT_TIME);	/*等待桌面线程初始化完成*/
}

int main()
{
	long res;

	if ((res = InitGUI()) != NO_ERROR)
		return res;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN + 1];

		if ((res = KRecvMsg((THREAD_ID*)&data[PTID_ID], data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_CNLMAP)	/*窗口进程异常退出的处理*/
		{
			GOBJ_DESC *gobj;

			for (gobj = gobjt; gobj < &gobjt[GOBJT_LEN]; gobj++)
				if (gobj->attr != INVALID && (DWORD)gobj->vbuf == data[MSG_ADDR_ID])	/*查找所属窗体*/
				{
					data[MSG_API_ID] = MSG_ATTR_GUI | GM_DESTROY;
					data[GUIMSG_GOBJ_ID] = gobj - gobjt;
					break;
				}
		}
		if (data[MSG_ATTR_ID] == MSG_ATTR_KBD)	/*键盘驱动消息*/
			KeyboardProc(data[1]);
		else if (data[MSG_ATTR_ID] == MSG_ATTR_MUS)	/*鼠标驱动消息*/
			MouseProc(data[1], (long)data[2], (long)data[3], (long)data[4]);
		else if ((data[MSG_ATTR_ID] & MSG_MAP_MASK) == MSG_ATTR_ROMAP || (data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_GUI)
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) >= sizeof(ApiTable) / sizeof(void*))
			{
				data[MSG_RES_ID] = GUI_ERR_WRONG_ARGS;
				if (data[MSG_ATTR_ID] < MSG_ATTR_USER)
					KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
				else
					KSendMsg((THREAD_ID*)&data[PTID_ID], data, 0);
			}
			else
				ApiTable[data[MSG_API_ID] & MSG_API_MASK](data);
		}
		else if (data[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)
			break;
	}
	GDIrelease();
	KUnregKnlPort(SRV_GUI_PORT);
	return NO_ERROR;
}

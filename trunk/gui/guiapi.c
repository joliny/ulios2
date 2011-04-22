/*	guiapi.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面接口，响应应用程序的请求，执行服务
	最后修改日期：2010-10-05
*/

#include "gui.h"

extern GOBJ_DESC *gobjt[];	/*窗体管理表*/
extern GOBJ_DESC **FstGobj;	/*第一个空对象描述符指针*/
extern CLIPRECT *FstClipRect;	/*剪切矩形管理表指针*/

THREAD_ID KbdMusPtid;	/*键盘鼠标服务ID*/
long MouX, MouY, MpicWidth, MpicHeight;	/*鼠标位置,背景图像备份*/
DWORD *MpicBak;	/*鼠标背景图像*/

/*初始化GUI,如果不成功必须退出*/
long InitGUI()
{
	long res;
	void *addr;
	CLIPRECT *clip;

	if ((res = KRegKnlPort(SRV_GUI_PORT)) != NO_ERROR)	/*注册服务端口*/
		return res;
	if ((res = GDIinit()) != NO_ERROR)	/*初始化GDI*/
		return res;
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &KbdMusPtid)) != NO_ERROR)	/*取得键盘鼠标服务线程*/
		return res;
	if ((res = KMSetRecv(KbdMusPtid)) != NO_ERROR)	/*取得键盘鼠标消息*/
		return res;
	if ((res = KMapUserAddr(&addr, FDAT_SIZ + VDAT_SIZ)) != NO_ERROR)	/*申请自由内存和可视内存空间*/
		return res;
	InitFbt(fmt, FMT_LEN, addr, FDAT_SIZ);
	InitFbt(vmt, VMT_LEN, addr + FDAT_SIZ, VDAT_SIZ);
	memset32(gobjt, 0, GOBJT_LEN * sizeof(GOBJ_DESC*) / sizeof(DWORD));	/*窗体管理表*/
	FstGobj = gobjt;
	if ((FstClipRect = (CLIPRECT*)falloc(CLIPRECTT_LEN * sizeof(CLIPRECT))) == NULL)	/*剪切矩形管理表*/
		return GUI_ERR_HAVENO_MEMORY;
	for (clip = FstClipRect; clip < &FstClipRect[CLIPRECTT_LEN - 1]; clip++)
		clip->nxt = clip + 1;
	clip->nxt = NULL;
	if ((res = CreateDesktop(GDIwidth, GDIheight)) != NO_ERROR)	/*创建桌面*/
		return res;
	MpicHeight = MpicWidth = 32;	/*鼠标图像*/
	if ((MpicBak = valloc(MpicWidth * MpicHeight * sizeof(DWORD))) == NULL)
		return GUI_ERR_HAVENO_MEMORY;
	GDIGetImage(MouX, MouY, MpicBak, MpicWidth, MpicHeight);
	return NO_ERROR;
}

int main()
{
	long res;

	if ((res = InitGUI()) != NO_ERROR)
		return res;
	{
		WORD id;
		CreateGobj(KbdMusPtid, 0, GUI_TYPE_WINDOW, 200, 200, 400, 300, 0, &id);
		CreateGobj(KbdMusPtid, id, GUI_TYPE_WINDOW, 300, 100, 350, 100, 0, &id);
	}
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
			GDIPutImage(MouX, MouY, MpicBak, MpicWidth, MpicHeight);
			MouX = data[2];
			MouY = data[3];
			GDIGetImage(MouX, MouY, MpicBak, MpicWidth, MpicHeight);
			GDIDrawLine(MouX, MouY, MouX, MouY + 31, 0xFFFFFF);	/*画鼠标*/
			GDIDrawLine(MouX, MouY, MouX + 23, MouY + 23, 0xFFFFFF);
			GDIDrawLine(MouX, MouY + 31, MouX + 23, MouY + 23, 0xFFFFFF);
		}
	}
	GDIrelease();
	KUnregKnlPort(SRV_GUI_PORT);
	return NO_ERROR;
}

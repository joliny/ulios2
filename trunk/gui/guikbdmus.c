/*	guikbdmus.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面键盘鼠标功能实现
	最后修改日期：2011-05-22
*/

#include "gui.h"

#define MUSSTATE_MOVED	0x01	/*已移动*/

#define DBCLK_CLOCK		40		/*双击延迟*/
#define MUSPIC_MAXLEN	500
DWORD MusBak[MUSPIC_MAXLEN], MusPic[MUSPIC_MAXLEN];	/*鼠标背景图像*/
DWORD ClickClock[3];	/*单击时的时钟*/
DWORD MusKey, MusState;	/*鼠标键,状态*/
long MusX, MusY, MusPicWidth, MusPicHeight;	/*鼠标位置,背景图像备份*/
GOBJ_DESC *MusInGobj;	/*鼠标移入的窗体*/

GOBJ_DESC *DraggedGobj;	/*被拖拽的窗体*/
long DragX, DragY;		/*拖拽位置差*/
DWORD DragMode;			/*拖拽模式*/

GOBJ_DESC *FocusGobj;	/*输入焦点所在窗体*/

/*键盘消息处理*/
void KeyboardProc(DWORD key)
{
	DWORD data[MSG_DATA_LEN];

	if (FocusGobj)
	{
		data[MSG_API_ID] = MSG_ATTR_GUI | GM_KEY;	/*按键消息*/
		data[1] = key;
		data[GUIMSG_GOBJ_ID] = FocusGobj->ClientSign;
		data[MSG_RES_ID] = NO_ERROR;
		KSendMsg(&FocusGobj->ptid, data, 0);
	}
}

/*鼠标初始化*/
long InitMouse()
{
	long res;

	if ((res = LoadBmp("guimouse.bmp", MusPic, MUSPIC_MAXLEN, (DWORD*)&MusPicWidth, (DWORD*)&MusPicHeight)) != NO_ERROR)
		return res;
	MusX = GDIwidth / 2;
	MusY = GDIheight / 2;
	ShowMouse();
	return NO_ERROR;
}

/*计算鼠标指针与区域是否重叠*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend)
{
	return MusX < xend && MusX + MusPicWidth > xpos && MusY < yend && MusY + MusPicHeight > ypos;
}

/*隐藏鼠标指针*/
void HideMouse()
{
	GDIPutImage(MusX, MusY, MusBak, MusPicWidth, MusPicHeight);
}

/*显示鼠标指针*/
void ShowMouse()
{
	GDIGetImage(MusX, MusY, MusBak, MusPicWidth, MusPicHeight);
	GDIPutBCImage(MusX, MusY, MusPic, MusPicWidth, MusPicHeight, 0xFFFFFFFF);
}

/*鼠标消息处理*/
void MouseProc(DWORD key, long x, long y, long z)
{
	DWORD data[MSG_DATA_LEN];
	GOBJ_DESC *gobj;
	DWORD i;

	x += MusX;	/*鼠标位置越界计算*/
	if (x < 0)
		x = 0;
	else if (x >= (long)GDIwidth)
		x = GDIwidth - 1;
	y += MusY;
	if (y < 0)
		y = 0;
	else if (y >= (long)GDIheight)
		y = GDIheight - 1;

	data[5] = x;
	data[6] = y;
	gobj = FindGobj((long*)&data[5], (long*)&data[6]);	/*查找窗体*/
	if (gobj == NULL)
		return;
	data[1] = key;
	data[2] = x;
	data[3] = y;
	data[4] = z;
	data[5] = (data[5] & 0xFFFF) | (data[6] << 16);	/*坐标合成*/
	data[GUIMSG_GOBJ_ID] = gobj->ClientSign;
	data[MSG_RES_ID] = NO_ERROR;

	if (x != MusX || y != MusY)	/*鼠标位置移动*/
	{
		MusState |= MUSSTATE_MOVED;
		GDIPutImage(MusX, MusY, MusBak, MusPicWidth, MusPicHeight);	/*绘制鼠标指针*/
		MusX = x;
		MusY = y;
		GDIGetImage(MusX, MusY, MusBak, MusPicWidth, MusPicHeight);
		GDIPutBCImage(MusX, MusY, MusPic, MusPicWidth, MusPicHeight, 0xFFFFFFFF);
		if (DraggedGobj)
		{
			switch (DragMode)
			{
			case GM_DRAGMOD_MOVE:
				if (MoveGobj(DraggedGobj, MusX - DragX, MusY - DragY) == NO_ERROR)
				{
					data[MSG_API_ID] = MSG_ATTR_GUI | GM_MOVE;	/*被移动消息*/
					data[1] = DraggedGobj->rect.xpos;
					data[2] = DraggedGobj->rect.ypos;
					data[GUIMSG_GOBJ_ID] = DraggedGobj->ClientSign;
					KSendMsg(&DraggedGobj->ptid, data, 0);
					data[GUIMSG_GOBJ_ID] = gobj->ClientSign;
				}
				break;
			case GM_DRAGMOD_SIZE:
				data[MSG_API_ID] = MSG_ATTR_GUI | GM_DRAG;	/*拖拽缩放消息*/
				data[1] = GM_DRAGMOD_SIZE;
				data[2] = MusX - DragX;
				data[3] = MusY - DragY;
				data[GUIMSG_GOBJ_ID] = DraggedGobj->ClientSign;
				KSendMsg(&DraggedGobj->ptid, data, 0);
				data[GUIMSG_GOBJ_ID] = gobj->ClientSign;
				break;
			}
		}
		else
		{
			if (gobj != MusInGobj)	/*移到了其他窗体*/
			{
				if (MusInGobj && *(DWORD*)(&MusInGobj->ptid) != INVALID)	/*从此有效窗体移开*/
				{
					data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSELEAVE;	/*移出消息*/
					data[GUIMSG_GOBJ_ID] = MusInGobj->ClientSign;
					KSendMsg(&MusInGobj->ptid, data, 0);
					data[GUIMSG_GOBJ_ID] = gobj->ClientSign;
				}
				MusInGobj = gobj;
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSEENTER;	/*移入消息*/
				KSendMsg(&gobj->ptid, data, 0);
			}
			else
			{
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSEMOVE;	/*移动消息*/
				KSendMsg(&gobj->ptid, data, 0);
			}
		}
	}
	if (z)
	{
		data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSEWHEEL;	/*滚轮消息*/
		KSendMsg(&gobj->ptid, data, 0);
	}
	for (i = 0; i < 3; i++)	/*检查鼠标按键*/
	{
		DWORD key0, key1;

		key0 = (MusKey >> i) & 1;
		key1 = (key >> i) & 1;
		if (key1 != key0)	/*按键状态发生变化*/
		{
			if (key1)	/*按下*/
			{
				MusState &= (~MUSSTATE_MOVED);
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONDOWN + i);	/*按下消息*/
				KSendMsg(&gobj->ptid, data, 0);
			}
			else	/*抬起*/
			{
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONUP + i);	/*抬起消息*/
				KSendMsg(&gobj->ptid, data, 0);
				if (!(MusState & MUSSTATE_MOVED))	/*按下后没有移动*/
				{
					DWORD CurClock;

					KGetClock(&CurClock);
					if (CurClock - ClickClock[i] > DBCLK_CLOCK)	/*单击后超时,还是单击*/
					{
						ClickClock[i] = CurClock;
						data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONCLICK + i);	/*单击消息*/
						KSendMsg(&gobj->ptid, data, 0);
					}
					else
					{
						ClickClock[i] = 0;
						data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONDBCLK + i);	/*双击消息*/
						KSendMsg(&gobj->ptid, data, 0);
					}
				}
				DraggedGobj = NULL;	/*鼠标键抬起,停止拖拽*/
			}
			break;
		}
	}
	MusKey = key;
}

/*处理拖拽消息*/
long DragGobj(GOBJ_DESC *gobj, DWORD mode)
{
	if (mode != GM_DRAGMOD_NONE)
	{
		if (DraggedGobj)	/*已有窗体被拖拽*/
			return GUI_ERR_BEING_DRAGGED;
		GetGobjPos(gobj, &DragX, &DragY);
		if (MusX < DragX || MusX > gobj->rect.xend - gobj->rect.xpos + DragX ||	/*非法的拖拽位置*/
			MusY < DragY || MusY > gobj->rect.yend - gobj->rect.ypos + DragY)
			return GUI_ERR_WRONG_ARGS;

		switch (mode)
		{
		case GM_DRAGMOD_MOVE:
			DragX = MusX - gobj->rect.xpos;	/*记录拖拽位置差*/
			DragY = MusY - gobj->rect.ypos;
			break;
		case GM_DRAGMOD_SIZE:
			DragX = MusX - (gobj->rect.xend - gobj->rect.xpos);	/*记录拖拽位置差*/
			DragY = MusY - (gobj->rect.yend - gobj->rect.ypos);
			break;
		}
		DragMode = mode;
		DraggedGobj = gobj;
	}
	else
		DraggedGobj = NULL;

	return NO_ERROR;
}

/*	mouse.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面鼠标功能实现
	最后修改日期：2011-05-22
*/

#include "gui.h"
#include "../lib/gdi.h"

#define MOUPIC_MAXLEN	500
DWORD MouBak[MOUPIC_MAXLEN], MouPic[MOUPIC_MAXLEN];	/*鼠标背景图像*/
DWORD MouKey;
long MouX, MouY, MpicWidth, MpicHeight;	/*鼠标位置,背景图像备份*/

/*鼠标初始化*/
long InitMouse()
{
	long res;

	if ((res = LoadBmp("guimouse.bmp", MouPic, MOUPIC_MAXLEN, &MpicWidth, &MpicHeight)) != NO_ERROR)
		return res;
	ShowMouse();
	return NO_ERROR;
}

/*计算鼠标指针与区域是否重叠*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend)
{
	return MouX < xend && MouX + MpicWidth > xpos && MouY < yend && MouY + MpicHeight > ypos;
}

/*隐藏鼠标指针*/
void HidMouse()
{
	GDIPutImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
}

/*显示鼠标指针*/
void ShowMouse()
{
	GDIGetImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
	GDIPutBCImage(MouX, MouY, MouPic, MpicWidth, MpicHeight, 0xFFFFFFFF);
}

/*设置鼠标位置*/
void SetMousePos(long x, long y)
{
	GDIPutImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
	MouX = x;
	MouY = y;
	GDIGetImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
	GDIPutBCImage(MouX, MouY, MouPic, MpicWidth, MpicHeight, 0xFFFFFFFF);
}

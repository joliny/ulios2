/*	mouse.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�������깦��ʵ��
	����޸����ڣ�2011-05-22
*/

#include "gui.h"
#include "../lib/gdi.h"

#define MOUPIC_MAXLEN	500
DWORD MouBak[MOUPIC_MAXLEN], MouPic[MOUPIC_MAXLEN];	/*��걳��ͼ��*/
DWORD MouKey;
long MouX, MouY, MpicWidth, MpicHeight;	/*���λ��,����ͼ�񱸷�*/

/*����ʼ��*/
long InitMouse()
{
	long res;

	if ((res = LoadBmp("guimouse.bmp", MouPic, MOUPIC_MAXLEN, &MpicWidth, &MpicHeight)) != NO_ERROR)
		return res;
	ShowMouse();
	return NO_ERROR;
}

/*�������ָ���������Ƿ��ص�*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend)
{
	return MouX < xend && MouX + MpicWidth > xpos && MouY < yend && MouY + MpicHeight > ypos;
}

/*�������ָ��*/
void HidMouse()
{
	GDIPutImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
}

/*��ʾ���ָ��*/
void ShowMouse()
{
	GDIGetImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
	GDIPutBCImage(MouX, MouY, MouPic, MpicWidth, MpicHeight, 0xFFFFFFFF);
}

/*�������λ��*/
void SetMousePos(long x, long y)
{
	GDIPutImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
	MouX = x;
	MouY = y;
	GDIGetImage(MouX, MouY, MouBak, MpicWidth, MpicHeight);
	GDIPutBCImage(MouX, MouY, MouPic, MpicWidth, MpicHeight, 0xFFFFFFFF);
}

/*	guikbdmus.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û����������깦��ʵ��
	����޸����ڣ�2011-05-22
*/

#include "gui.h"

#define MUSSTATE_MOVED	0x01	/*���ƶ�*/

#define DBCLK_CLOCK		40		/*˫���ӳ�*/
#define MUSPIC_MAXLEN	500
DWORD MusBak[MUSPIC_MAXLEN], MusPic[MUSPIC_MAXLEN];	/*��걳��ͼ��*/
DWORD ClickClock[3];	/*����ʱ��ʱ��*/
DWORD MusKey, MusState;	/*����,״̬*/
long MusX, MusY, MusPicWidth, MusPicHeight;	/*���λ��,����ͼ�񱸷�*/
GOBJ_DESC *MusInGobj;	/*�������Ĵ���*/

GOBJ_DESC *DraggedGobj;	/*����ק�Ĵ���*/
long DragX, DragY;		/*��קλ�ò�*/
DWORD DragMode;			/*��קģʽ*/

GOBJ_DESC *FocusGobj;	/*���뽹�����ڴ���*/

/*������Ϣ����*/
void KeyboardProc(DWORD key)
{
	DWORD data[MSG_DATA_LEN];

	if (FocusGobj)
	{
		data[MSG_API_ID] = MSG_ATTR_GUI | GM_KEY;	/*������Ϣ*/
		data[1] = key;
		data[GUIMSG_GOBJ_ID] = FocusGobj->ClientSign;
		data[MSG_RES_ID] = NO_ERROR;
		KSendMsg(&FocusGobj->ptid, data, 0);
	}
}

/*����ʼ��*/
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

/*�������ָ���������Ƿ��ص�*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend)
{
	return MusX < xend && MusX + MusPicWidth > xpos && MusY < yend && MusY + MusPicHeight > ypos;
}

/*�������ָ��*/
void HideMouse()
{
	GDIPutImage(MusX, MusY, MusBak, MusPicWidth, MusPicHeight);
}

/*��ʾ���ָ��*/
void ShowMouse()
{
	GDIGetImage(MusX, MusY, MusBak, MusPicWidth, MusPicHeight);
	GDIPutBCImage(MusX, MusY, MusPic, MusPicWidth, MusPicHeight, 0xFFFFFFFF);
}

/*�����Ϣ����*/
void MouseProc(DWORD key, long x, long y, long z)
{
	DWORD data[MSG_DATA_LEN];
	GOBJ_DESC *gobj;
	DWORD i;

	x += MusX;	/*���λ��Խ�����*/
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
	gobj = FindGobj((long*)&data[5], (long*)&data[6]);	/*���Ҵ���*/
	if (gobj == NULL)
		return;
	data[1] = key;
	data[2] = x;
	data[3] = y;
	data[4] = z;
	data[5] = (data[5] & 0xFFFF) | (data[6] << 16);	/*����ϳ�*/
	data[GUIMSG_GOBJ_ID] = gobj->ClientSign;
	data[MSG_RES_ID] = NO_ERROR;

	if (x != MusX || y != MusY)	/*���λ���ƶ�*/
	{
		MusState |= MUSSTATE_MOVED;
		GDIPutImage(MusX, MusY, MusBak, MusPicWidth, MusPicHeight);	/*�������ָ��*/
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
					data[MSG_API_ID] = MSG_ATTR_GUI | GM_MOVE;	/*���ƶ���Ϣ*/
					data[1] = DraggedGobj->rect.xpos;
					data[2] = DraggedGobj->rect.ypos;
					data[GUIMSG_GOBJ_ID] = DraggedGobj->ClientSign;
					KSendMsg(&DraggedGobj->ptid, data, 0);
					data[GUIMSG_GOBJ_ID] = gobj->ClientSign;
				}
				break;
			case GM_DRAGMOD_SIZE:
				data[MSG_API_ID] = MSG_ATTR_GUI | GM_DRAG;	/*��ק������Ϣ*/
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
			if (gobj != MusInGobj)	/*�Ƶ�����������*/
			{
				if (MusInGobj && *(DWORD*)(&MusInGobj->ptid) != INVALID)	/*�Ӵ���Ч�����ƿ�*/
				{
					data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSELEAVE;	/*�Ƴ���Ϣ*/
					data[GUIMSG_GOBJ_ID] = MusInGobj->ClientSign;
					KSendMsg(&MusInGobj->ptid, data, 0);
					data[GUIMSG_GOBJ_ID] = gobj->ClientSign;
				}
				MusInGobj = gobj;
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSEENTER;	/*������Ϣ*/
				KSendMsg(&gobj->ptid, data, 0);
			}
			else
			{
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSEMOVE;	/*�ƶ���Ϣ*/
				KSendMsg(&gobj->ptid, data, 0);
			}
		}
	}
	if (z)
	{
		data[MSG_ATTR_ID] = MSG_ATTR_GUI | GM_MOUSEWHEEL;	/*������Ϣ*/
		KSendMsg(&gobj->ptid, data, 0);
	}
	for (i = 0; i < 3; i++)	/*�����갴��*/
	{
		DWORD key0, key1;

		key0 = (MusKey >> i) & 1;
		key1 = (key >> i) & 1;
		if (key1 != key0)	/*����״̬�����仯*/
		{
			if (key1)	/*����*/
			{
				MusState &= (~MUSSTATE_MOVED);
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONDOWN + i);	/*������Ϣ*/
				KSendMsg(&gobj->ptid, data, 0);
			}
			else	/*̧��*/
			{
				data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONUP + i);	/*̧����Ϣ*/
				KSendMsg(&gobj->ptid, data, 0);
				if (!(MusState & MUSSTATE_MOVED))	/*���º�û���ƶ�*/
				{
					DWORD CurClock;

					KGetClock(&CurClock);
					if (CurClock - ClickClock[i] > DBCLK_CLOCK)	/*������ʱ,���ǵ���*/
					{
						ClickClock[i] = CurClock;
						data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONCLICK + i);	/*������Ϣ*/
						KSendMsg(&gobj->ptid, data, 0);
					}
					else
					{
						ClickClock[i] = 0;
						data[MSG_ATTR_ID] = MSG_ATTR_GUI | (GM_LBUTTONDBCLK + i);	/*˫����Ϣ*/
						KSendMsg(&gobj->ptid, data, 0);
					}
				}
				DraggedGobj = NULL;	/*����̧��,ֹͣ��ק*/
			}
			break;
		}
	}
	MusKey = key;
}

/*������ק��Ϣ*/
long DragGobj(GOBJ_DESC *gobj, DWORD mode)
{
	if (mode != GM_DRAGMOD_NONE)
	{
		if (DraggedGobj)	/*���д��屻��ק*/
			return GUI_ERR_BEING_DRAGGED;
		GetGobjPos(gobj, &DragX, &DragY);
		if (MusX < DragX || MusX > gobj->rect.xend - gobj->rect.xpos + DragX ||	/*�Ƿ�����קλ��*/
			MusY < DragY || MusY > gobj->rect.yend - gobj->rect.ypos + DragY)
			return GUI_ERR_WRONG_ARGS;

		switch (mode)
		{
		case GM_DRAGMOD_MOVE:
			DragX = MusX - gobj->rect.xpos;	/*��¼��קλ�ò�*/
			DragY = MusY - gobj->rect.ypos;
			break;
		case GM_DRAGMOD_SIZE:
			DragX = MusX - (gobj->rect.xend - gobj->rect.xpos);	/*��¼��קλ�ò�*/
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

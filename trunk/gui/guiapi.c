/*	guiapi.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�����ӿڣ���ӦӦ�ó��������ִ�з���
	����޸����ڣ�2010-10-05
*/

#include "gui.h"
#include "../lib/gdi.h"

extern GOBJ_DESC gobjt[];	/*���������������*/
extern GOBJ_DESC *FstGobj;	/*���������������ָ��*/
extern CLIPRECT ClipRectt[];	/*���о��ι����*/
extern CLIPRECT *FstClipRect;	/*���о��ι����ָ��*/

THREAD_ID KbdMusPtid;	/*����������ID*/

/*��ʼ��GUI,������ɹ������˳�*/
long InitGUI()
{
	long res;
	GOBJ_DESC *gobj;
	CLIPRECT *clip;
	THREAD_ID ptid;

	if ((res = KRegKnlPort(SRV_GUI_PORT)) != NO_ERROR)	/*ע�����˿�*/
		return res;
	if ((res = GDIinit()) != NO_ERROR)	/*��ʼ��GDI*/
		return res;
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &KbdMusPtid)) != NO_ERROR)	/*ȡ�ü����������߳�*/
		return res;
	if ((res = KMSetRecv(KbdMusPtid)) != NO_ERROR)	/*ȡ�ü��������Ϣ*/
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

/*����5��״̬0����1����2��ס3�ɿ�4����*/
#define MOUK_FREE	0
#define MOUK_DOWN	1
#define MOUK_DRAG	2
#define MOUK_UP		3
#define MOUK_PRESS	4
BYTE mouk[4];

void chkmou(DWORD k)	/*���״̬����*/
{
	DWORD i;

	for (i = 0; i < 3; i++)
		if ((k >> i) & 1)	/*���3������״̬*/
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
		else	/*û����*/
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

/*�����Ϣ����*/
void MouseProc(DWORD key, long x, long y, long z)
{
	if (MouX != x || MouY != y)	/*λ���ƶ�*/
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

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (ptid.ProcID == KbdMusPtid.ProcID && data[0] == MSG_ATTR_MUS)	/*�����Ϣ*/
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
				case MOUK_DOWN:	/*�������*/
					WndX = DownX = MouX;
					WndY = DownY = MouY;
					gobj = FindGobj(&WndX, &WndY);
					break;
				case MOUK_DRAG:	/*����϶�*/
					MoveGobj(gobj, gobj->rect.xpos + MouX - DownX, gobj->rect.ypos + MouY - DownY, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos);
					DownX = MouX;
					DownY = MouY;
					break;
				}
				switch (mouk[1])
				{
				case MOUK_DOWN:	/*�Ҽ�����*/
					WndX = DownX = MouX;
					WndY = DownY = MouY;
					gobj = FindGobj(&WndX, &WndY);
					break;
				case MOUK_UP:	/*�Ҽ�̧��*/
					if (MouX <= DownX || MouY <= DownY)
						DeleteGobj(gobj);
					else
						CreateGobj(KbdMusPtid, 0, gobj - gobjt, WndX, WndY, MouX - DownX, MouY - DownY, &id);
					break;
				}
				switch (mouk[2])
				{
				case MOUK_DOWN:	/*�м�����*/
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

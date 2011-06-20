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
}

/*ϵͳ���ñ�*/
void (*ApiTable[])(DWORD *argv) = {
	ApiCreate, ApiDestroy, ApiMove, ApiSize
};

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

	InitMouse();
	*((DWORD*)&ptid) = INVALID;
	CreateDesktop(ptid, 0, GDIwidth, GDIheight, GDIvm);
	KCreateThread((void(*)(void*))DesktopThread, 0x700000, gobjt, &ptid);
	gobjt[0].ptid = ptid;

	return NO_ERROR;
}

int main()
{
	long res;

	if ((res = InitGUI()) != NO_ERROR)
		return res;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN + 1];

		if ((res = KRecvMsg((THREAD_ID*)&data[PTID_ID], data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) == MSG_ATTR_CNLMAP)	/*���ڽ����쳣�˳��Ĵ���*/
		{
			GOBJ_DESC *gobj;

			for (gobj = gobjt; gobj < &gobjt[GOBJT_LEN]; gobj++)
				if (gobj->attr != INVALID && (DWORD)gobj->vbuf == data[MSG_ADDR_ID])	/*������������*/
				{
					data[MSG_API_ID] = MSG_ATTR_GUI | GM_DESTROY;
					data[GUIMSG_GOBJ_ID] = gobj - gobjt;
					break;
				}
		}
		if (data[MSG_ATTR_ID] == MSG_ATTR_MUS)	/*���������Ϣ*/
			MouseProc(data[1], (long)data[2], (long)data[3], (long)data[4]);
		else if (data[MSG_ATTR_ID] == MSG_ATTR_KBD)	/*����������Ϣ*/
			;//KeyBoardProc(data[1], (long)data[2], (long)data[3], (long)data[4]);
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

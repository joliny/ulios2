/*	guiapi.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�����ӿڣ���ӦӦ�ó��������ִ�з���
	����޸����ڣ�2010-10-05
*/

#include "gui.h"

extern GOBJ_DESC *gobjt[];	/*��������*/
extern GOBJ_DESC **FstGobj;	/*��һ���ն���������ָ��*/
extern CLIPRECT *FstClipRect;	/*���о��ι����ָ��*/

THREAD_ID KbdMusPtid;	/*����������ID*/
long MouX, MouY, MpicWidth, MpicHeight;	/*���λ��,����ͼ�񱸷�*/
DWORD *MpicBak;	/*��걳��ͼ��*/

/*��ʼ��GUI,������ɹ������˳�*/
long InitGUI()
{
	long res;
	void *addr;
	CLIPRECT *clip;

	if ((res = KRegKnlPort(SRV_GUI_PORT)) != NO_ERROR)	/*ע�����˿�*/
		return res;
	if ((res = GDIinit()) != NO_ERROR)	/*��ʼ��GDI*/
		return res;
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &KbdMusPtid)) != NO_ERROR)	/*ȡ�ü����������߳�*/
		return res;
	if ((res = KMSetRecv(KbdMusPtid)) != NO_ERROR)	/*ȡ�ü��������Ϣ*/
		return res;
	if ((res = KMapUserAddr(&addr, FDAT_SIZ + VDAT_SIZ)) != NO_ERROR)	/*���������ڴ�Ϳ����ڴ�ռ�*/
		return res;
	InitFbt(fmt, FMT_LEN, addr, FDAT_SIZ);
	InitFbt(vmt, VMT_LEN, addr + FDAT_SIZ, VDAT_SIZ);
	memset32(gobjt, 0, GOBJT_LEN * sizeof(GOBJ_DESC*) / sizeof(DWORD));	/*��������*/
	FstGobj = gobjt;
	if ((FstClipRect = (CLIPRECT*)falloc(CLIPRECTT_LEN * sizeof(CLIPRECT))) == NULL)	/*���о��ι����*/
		return GUI_ERR_HAVENO_MEMORY;
	for (clip = FstClipRect; clip < &FstClipRect[CLIPRECTT_LEN - 1]; clip++)
		clip->nxt = clip + 1;
	clip->nxt = NULL;
	if ((res = CreateDesktop(GDIwidth, GDIheight)) != NO_ERROR)	/*��������*/
		return res;
	MpicHeight = MpicWidth = 32;	/*���ͼ��*/
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

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (ptid.ProcID == KbdMusPtid.ProcID && data[0] == MSG_ATTR_MUS)	/*�����Ϣ*/
		{
			if (MouX == data[2] && MouY == data[3])
				continue;
			GDIPutImage(MouX, MouY, MpicBak, MpicWidth, MpicHeight);
			MouX = data[2];
			MouY = data[3];
			GDIGetImage(MouX, MouY, MpicBak, MpicWidth, MpicHeight);
			GDIDrawLine(MouX, MouY, MouX, MouY + 31, 0xFFFFFF);	/*�����*/
			GDIDrawLine(MouX, MouY, MouX + 23, MouY + 23, 0xFFFFFF);
			GDIDrawLine(MouX, MouY + 31, MouX + 23, MouY + 23, 0xFFFFFF);
		}
	}
	GDIrelease();
	KUnregKnlPort(SRV_GUI_PORT);
	return NO_ERROR;
}

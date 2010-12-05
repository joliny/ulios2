/*	guiapi.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�����ӿڣ���ӦӦ�ó��������ִ�з���
	����޸����ڣ�2010-10-05
*/

#include "gui.h"

THREAD_ID KbdMusPtid;	/*����������ID*/
long MouX, MouY, MpicWidth, MpicHeight;	/*���λ��,����ͼ�񱸷�*/
DWORD *MpicBak;	/*��걳��ͼ��*/

/*��ʼ��GUI,������ɹ������˳�*/
long InitGUI()
{
	long res;
	void *addr;

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
	fmtl = FALSE;
	InitFbt(vmt, VMT_LEN, addr + FDAT_SIZ, VDAT_SIZ);
	vmtl = FALSE;
	MpicHeight = MpicWidth = 32;	/*���ͼ��*/
	if ((MpicBak = valloc(MpicWidth * MpicHeight * sizeof(DWORD))) == NULL)
		return res;
	GDIGetImage(MouX, MouY, MpicBak, MpicWidth, MpicHeight);
	return NO_ERROR;
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

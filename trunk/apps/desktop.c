/*	desktop.c for ulios application
	���ߣ�����
	���ܣ�����Ӧ�ý���
	����޸����ڣ�2011-05-21
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

/*˫���Բ�ֵ24λͼ������,����:Դ����,Դ�ߴ�,Ŀ�Ļ���,Ŀ�ĳߴ�*/
void ZoomImage(DWORD *src, DWORD sw, DWORD sh, DWORD *dst, DWORD dw, DWORD dh)
{
	DWORD ScaleX, ScaleY, x, y, IdxX, IdxY, DecX, DecY;	// ����,ѭ������,Դ��������,���ƫ��С��
	ScaleX = ((sw - 1) << 16) / dw;
	ScaleY = ((sh - 1) << 16) / dh;
	IdxY = 0;
	y = dh;
	do
	{
		DecY = (IdxY >> 8) & 0xFF;
		IdxX = 0;
		x = dw;
		do
		{
			DecX = (IdxX >> 8) & 0xFF;

			DWORD tr1, tg1, tb1, tr2, tg2, tb2;	// ��ԭɫ��ʱ����
			DWORD a, b, c, d;
			DWORD *psrc = &src[(IdxX >> 16) + (IdxY >> 16) * sw];
			a = *(psrc);	// ȡ��Դ����Ŀ�����ص��ĸ���
			b = *(psrc + 1);
			c = *(psrc + sw);
			d = *(psrc + sw + 1);

			tr1 = DecX * (long)((b & 0x0000FF) - (a & 0x0000FF)) + ((a & 0x0000FF) << 8);	// a��b c��d�������
			tg1 = DecX * (long)((b & 0x00FF00) - (a & 0x00FF00)) + ((a & 0x00FF00) << 8);
			tb1 = DecX * (long)((b & 0xFF0000) - (a & 0xFF0000)) + ((a & 0xFF0000) << 8);
			tr2 = DecX * (long)((d & 0x0000FF) - (c & 0x0000FF)) + ((c & 0x0000FF) << 8);
			tg2 = DecX * (long)((d & 0x00FF00) - (c & 0x00FF00)) + ((c & 0x00FF00) << 8);
			tb2 = DecX * (long)((d & 0xFF0000) - (c & 0xFF0000)) + ((c & 0xFF0000) << 8);
			tr1 += DecY * (long)((tr2 >> 8) - (tr1 >> 8));	// �������
			tg1 += DecY * (long)((tg2 >> 8) - (tg1 >> 8));
			tb1 += DecY * (long)((tb2 >> 8) - (tb1 >> 8));
			*dst++ = ((tr1 & 0x0000FF00) | (tg1 & 0x00FF0000) | (tb1 & 0xFF000000)) >> 8;	// ��ԭɫ�ϳ�
			IdxX += ScaleX;
		}
		while (--x > 0);
		IdxY += ScaleY;
	}
	while (--y > 0);
}

void CmdProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	KCreateProcess(0, "cmd.bin", NULL, &ptid);
}

void GmgrProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	KCreateProcess(0, "gmgr.bin", NULL, &ptid);
}

/*������Ϣ������*/
long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_DSK *dsk = (CTRL_DSK*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			DWORD *bmp, BmpWidth, BmpHeight;
			if (dsk->obj.gid)	/*��������,�����˳�*/
			{
				GUIdestroy(dsk->obj.gid);
				break;
			}
			GCLoadBmp("desktop.bmp", NULL, 0, &BmpWidth, &BmpHeight);
			bmp = (DWORD*)malloc(BmpWidth * BmpHeight * sizeof(DWORD));
			GCLoadBmp("desktop.bmp", bmp, BmpWidth * BmpHeight, NULL, NULL);
			ZoomImage(bmp, BmpWidth, BmpHeight, dsk->obj.uda.vbuf, dsk->obj.uda.width, dsk->obj.uda.height);
			free(bmp);
		}
		{
			CTRL_ARGS args;
			args.width = 80;
			args.height = 20;
			args.x = 16;
			args.y = 16;
			args.style = 0;
			args.MsgProc = NULL;
			GCBtnCreate(NULL, &args, dsk->obj.gid, &dsk->obj, "������ʾ��", NULL, CmdProc);
			args.y = 40;
			GCBtnCreate(NULL, &args, dsk->obj.gid, &dsk->obj, "��Դ������", NULL, GmgrProc);
		}
		break;
	}
	return GCDskDefMsgProc(ptid, data);
}

int main()
{
	CTRL_DSK *dsk;
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*����16MB���ڴ�*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = GCwidth;
	args.height = GCheight;
	args.x = 0;
	args.y = 0;
	args.style = 0;
	args.MsgProc = MainMsgProc;
	GCDskCreate(&dsk, &args, 0, NULL);

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*����GUI��Ϣ*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)dsk)	/*����������,�˳�����*/
				break;
		}
	}
	return NO_ERROR;
}

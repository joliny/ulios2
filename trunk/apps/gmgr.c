/*	gmgr.c for ulios application
	���ߣ�����
	���ܣ�ͼ�λ���Դ����������
	����޸����ڣ�2012-02-13
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

THREAD_ID FsPtid;
CTRL_LST *PartList;	/*�����б�*/
CTRL_LST *FileList;	/*�ļ��б�*/
CTRL_BTN *ParBtn;	/*��Ŀ¼��ť*/
CTRL_BTN *CutBtn;	/*���а�ť*/
CTRL_BTN *CopyBtn;	/*���ư�ť*/
CTRL_BTN *PasteBtn;	/*ճ����ť*/
CTRL_BTN *DelBtn;	/*ɾ����ť*/
CTRL_BTN *DirBtn;	/*����Ŀ¼��ť*/
CTRL_BTN *FileBtn;	/*�����ļ���ť*/
CTRL_BTN *RenamBtn;	/*��������ť*/

/*˫��ת��Ϊ����*/
char *Itoa(char *buf, DWORD n, DWORD r)
{
	static const char num[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	char *p, *q;

	q = p = buf;
	do
	{
		*p++ = num[n % r];
		n /= r;
	}
	while (n);
	buf = p;	/*ȷ���ַ���β��*/
	*p-- = '\0';
	while (p > q)	/*��ת�ַ���*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	return buf;
}

/*��ʽ�����*/
void Sprintf(char *buf, const char *fmtstr, ...)
{
	long num;
	const DWORD *args = (DWORD*)(&fmtstr);

	while (*fmtstr)
	{
		if (*fmtstr == '%')
		{
			fmtstr++;
			switch (*fmtstr)
			{
			case 'd':
				num = *((long*)++args);
				if (num < 0)
				{
					*buf++ = '-';
					buf = Itoa(buf, -num, 10);
				}
				else
					buf = Itoa(buf, num, 10);
				break;
			case 'u':
				buf = Itoa(buf, *((DWORD*)++args), 10);
				break;
			case 'x':
			case 'X':
				buf = Itoa(buf, *((DWORD*)++args), 16);
				break;
			case 'o':
				buf = Itoa(buf, *((DWORD*)++args), 8);
				break;
			case 's':
				buf = strcpy(buf, *((const char**)++args)) - 1;
				break;
			case 'c':
				*buf++ = *((char*)++args);
				break;
			default:
				*buf++ = *fmtstr;
			}
		}
		else
			*buf++ = *fmtstr;
		fmtstr++;
	}
	*buf = '\0';
}

#define WND_WIDTH	400	/*������С���,�߶�*/
#define WND_HEIGHT	300
#define SIDE		2	/*�ؼ��߾�*/
#define PART_WIDTH	64	/*�����б���*/
#define BTN_WIDTH	56	/*��ť���,�߶�*/
#define BTN_HEIGHT	20

void PartListSelProc(CTRL_LST *lst)
{
	if (lst->SelItem)	/*ѡ���̷�*/
	{
		FILE_INFO fi;
		long dh;
		LIST_ITEM *item;
		FSChDir(FsPtid, lst->SelItem->text);
		if ((dh = FSOpenDir(FsPtid, "")) < 0)
			return;
		GCLstDelAllItem(FileList);
		item = NULL;
		while (FSReadDir(FsPtid, dh, &fi) == NO_ERROR)
		{
			char buf[MAX_PATH];
			Sprintf(buf, "%s %s", (fi.attr & FILE_ATTR_DIREC) ? "[]" : "==", fi.name);
			GCLstInsertItem(FileList, item, buf, &item);
		}
		FSclose(FsPtid, dh);
	}
}

long PartListMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_LST *lst = (CTRL_LST*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			DWORD pid;
			LIST_ITEM *item;

			pid = 0;
			item = NULL;
			while (FSEnumPart(FsPtid, &pid) == NO_ERROR)
			{
				char buf[4];
				Sprintf(buf, "/%u", pid);
				GCLstInsertItem(lst, item, buf, &item);
				pid++;
			}
		}
		break;
	}
	return GCLstDefMsgProc(ptid, data);
}

void FileListSelProc(CTRL_LST *lst)
{
	if (lst->SelItem)	/*ѡ��Ŀ¼���ļ�*/
	{
		if (lst->SelItem->text[0] == '[')	/*��Ŀ¼*/
		{
			FILE_INFO fi;
			long dh;
			LIST_ITEM *item;
			FSChDir(FsPtid, lst->SelItem->text + 3);
			if ((dh = FSOpenDir(FsPtid, "")) < 0)
				return;
			GCLstDelAllItem(FileList);
			item = NULL;
			while (FSReadDir(FsPtid, dh, &fi) == NO_ERROR)
			{
				char buf[MAX_PATH];
				Sprintf(buf, "%s %s", (fi.attr & FILE_ATTR_DIREC) ? "[]" : "==", fi.name);
				GCLstInsertItem(FileList, item, buf, &item);
			}
			FSclose(FsPtid, dh);
		}
		else	/*ִ�г���*/
		{
			THREAD_ID ptid;
			KCreateProcess(0, lst->SelItem->text + 3, NULL, &ptid);
		}
	}
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			CTRL_ARGS args;
			long CliX, CliY;

			GCWndGetClientLoca(wnd, &CliX, &CliY);
			args.width = PART_WIDTH;
			args.height = wnd->client.height - SIDE * 2;
			args.x = CliX + SIDE;
			args.y = CliY + SIDE;
			args.style = 0;
			args.MsgProc = PartListMsgProc;
			GCLstCreate(&PartList, &args, wnd->obj.gid, &wnd->obj, PartListSelProc);
			args.width = wnd->client.width - PART_WIDTH - BTN_WIDTH - SIDE * 4;
			args.x = CliX + PART_WIDTH + SIDE * 2;
			args.MsgProc = NULL;
			GCLstCreate(&FileList, &args, wnd->obj.gid, &wnd->obj, FileListSelProc);
			args.width = BTN_WIDTH;
			args.height = BTN_HEIGHT;
			args.x = CliX + wnd->client.width - BTN_WIDTH - SIDE;
			GCBtnCreate(&ParBtn, &args, wnd->obj.gid, &wnd->obj, "�ϼ�Ŀ¼", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&CutBtn, &args, wnd->obj.gid, &wnd->obj, "����", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&CopyBtn, &args, wnd->obj.gid, &wnd->obj, "����", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&PasteBtn, &args, wnd->obj.gid, &wnd->obj, "ճ��", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&DelBtn, &args, wnd->obj.gid, &wnd->obj, "ɾ��", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&DirBtn, &args, wnd->obj.gid, &wnd->obj, "����Ŀ¼", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&FileBtn, &args, wnd->obj.gid, &wnd->obj, "�����ļ�", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&RenamBtn, &args, wnd->obj.gid, &wnd->obj, "������", NULL);
		}
		break;
	case GM_SIZE:
		{
			long x, y;
			DWORD width, height;

			GCWndGetClientLoca(wnd, &x, &y);
			width = PART_WIDTH;
			height = wnd->client.height - SIDE * 2;
			x += SIDE;
			y += SIDE;
			GCLstSetSize(PartList, x, y, width, height);
			width = wnd->client.width - PART_WIDTH - BTN_WIDTH - SIDE * 4;
			x += PART_WIDTH + SIDE;
			GCLstSetSize(FileList, x, y, width, height);
			x += width + SIDE;
			GCGobjMove(&ParBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&CutBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&CopyBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&PasteBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&DelBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&DirBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&FileBtn->obj, x, y);
			y += BTN_HEIGHT + SIDE;
			GCGobjMove(&RenamBtn->obj, x, y);
		}
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_WND *wnd;
	CTRL_ARGS args;
	long res;

	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)
		return res;
	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*����16MB���ڴ�*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = WND_WIDTH;
	args.height = WND_HEIGHT;
	args.x = (GCwidth - args.width) / 2;	/*����*/
	args.y = (GCheight - args.height) / 2;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN | WND_STYLE_MAXBTN | WND_STYLE_MINBTN | WND_STYLE_SIZEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "��Դ������");
	wnd->MinWidth = WND_WIDTH;
	wnd->MinHeight = WND_HEIGHT;

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*����GUI��Ϣ*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*����������,�˳�����*/
				break;
		}
	}
	return NO_ERROR;
}

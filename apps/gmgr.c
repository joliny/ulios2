/*	gmgr.c for ulios application
	���ߣ�����
	���ܣ�ͼ�λ���Դ����������
	����޸����ڣ�2012-02-13
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

CTRL_SEDT *DirSedt;	/*��ַ��*/
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
DWORD op;	/*����*/
char PathBuf[MAX_PATH];	/*���ƻ����·������*/

#define OP_NONE	0
#define OP_CUT	1
#define OP_COPY	2

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
#define EDT_HEIGHT	16	/*��ַ���߶�*/
#define PART_WIDTH	64	/*�����б���*/
#define BTN_WIDTH	56	/*��ť���,�߶�*/
#define BTN_HEIGHT	20

/*����ļ��б�*/
void FillFileList()
{
	FILE_INFO fi;
	long dh;
	LIST_ITEM *item;
	char buf[MAX_PATH];

	if ((dh = FSOpenDir("")) < 0)
		return;
	FSGetCwd(buf, MAX_PATH);
	GCSedtSetText(DirSedt, buf);
	GCLstDelAllItem(FileList);
	item = NULL;
	while (FSReadDir(dh, &fi) == NO_ERROR)
	{
		char buf[MAX_PATH];
		Sprintf(buf, "%s %s", (fi.attr & FILE_ATTR_DIREC) ? "[]" : "==", fi.name);
		GCLstInsertItem(FileList, item, buf, &item);
	}
	FSclose(dh);
}

/*��ַ���س�����*/
void DirSedtEnterProc(CTRL_SEDT *edt)
{
	if (FSChDir(edt->text) == NO_ERROR)
		FillFileList();
}

/*�����б�ѡȡ����*/
void PartListSelProc(CTRL_LST *lst)
{
	if (lst->SelItem)	/*ѡ���̷�*/
	{
		if (FSChDir(lst->SelItem->text) == NO_ERROR)
			FillFileList();
	}
}

/*�����б���Ϣ����*/
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
			while (FSEnumPart(&pid) == NO_ERROR)
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

/*�ļ��б�ѡȡ����*/
void FileListSelProc(CTRL_LST *lst)
{
	if (lst->SelItem)	/*ѡ���ļ�*/
	{
		GCBtnSetDisable(CutBtn, FALSE);
		GCBtnSetDisable(CopyBtn, FALSE);
		GCBtnSetDisable(DelBtn, FALSE);
		GCBtnSetDisable(RenamBtn, FALSE);
	}
	else
	{
		GCBtnSetDisable(CutBtn, TRUE);
		GCBtnSetDisable(CopyBtn, TRUE);
		GCBtnSetDisable(DelBtn, TRUE);
		GCBtnSetDisable(RenamBtn, TRUE);
	}
}

/*�ļ��б���Ϣ����*/
long FileListMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_LST *lst = (CTRL_LST*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		FillFileList();
		break;
	case GM_LBUTTONDBCLK:
		if (lst->SelItem)	/*ѡ��Ŀ¼���ļ�*/
		{
			if (lst->SelItem->text[0] == '[')	/*��Ŀ¼*/
			{
				if (FSChDir(lst->SelItem->text + 3) == NO_ERROR)
					FillFileList();
			}
			else	/*ִ�г���*/
			{
				THREAD_ID ptid;
				KCreateProcess(0, lst->SelItem->text + 3, NULL, &ptid);
			}
		}
		break;
	}
	return GCLstDefMsgProc(ptid, data);
}

/*�ϼ�Ŀ¼��ť����*/
void ParBtnPressProc(CTRL_BTN *btn)
{
	if (FSChDir("..") == NO_ERROR)
		FillFileList();
}

/*���а�ť����*/
void CutBtnPressProc(CTRL_BTN *btn)
{
	if (FileList->SelItem)	/*ѡ���ļ�*/
	{
		char buf[MAX_PATH];
		FSGetCwd(buf, MAX_PATH);
		Sprintf(PathBuf, "%s/%s", buf, FileList->SelItem->text + 3);
		op = OP_CUT;
		GCBtnSetDisable(PasteBtn, FALSE);
	}
}

/*���ư�ť����*/
void CopyBtnPressProc(CTRL_BTN *btn)
{
	if (FileList->SelItem)	/*ѡ���ļ�*/
	{
		char buf[MAX_PATH];
		FSGetCwd(buf, MAX_PATH);
		Sprintf(PathBuf, "%s/%s", buf, FileList->SelItem->text + 3);
		op = OP_COPY;
		GCBtnSetDisable(PasteBtn, FALSE);
	}
}

/*�ݹ�ɾ��Ŀ¼*/
void DelTree(char *path)
{
	if (FSChDir(path) == NO_ERROR)
	{
		FILE_INFO fi;
		long dh;
		if ((dh = FSOpenDir("")) < 0)
		{
			FSChDir("..");
			return;
		}
		while (FSReadDir(dh, &fi) == NO_ERROR)
			DelTree(fi.name);
		FSclose(dh);
		FSChDir("..");
	}
	FSremove(path);
}

/*�ݹ鸴��Ŀ¼����ǰĿ¼*/
void CopyTree(char *path)
{
	char *end, *name;
	FILE_INFO fi;
	long inh;

	end = path;	/*����·��������*/
	while (*end)
		end++;
	name = end;
	while (name > path && *name != '/')
		name--;
	if (*name == '/')
		name++;
	if ((inh = FSOpenDir(path)) >= 0)	/*���Դ�Ŀ¼*/
	{
		if (FSMkDir(name) == NO_ERROR)	/*������Ŀ¼*/
		{
			FSChDir(name);
			while (FSReadDir(inh, &fi) == NO_ERROR)
				if (!(fi.name[0] == '.' && (fi.name[1] == '\0' || (fi.name[1] == '.' && fi.name[2] == '\0'))))	/*�����˫��Ŀ¼������*/
				{
					*end = '/';
					strcpy(end + 1, fi.name);
					CopyTree(path);
					*end = '\0';
				}
			FSChDir("..");
		}
		FSclose(inh);
	}
	else if ((inh = FSopen(path, FS_OPEN_READ)) >= 0)	/*���Դ��ļ�*/
	{
		char buf[4096];
		long outh, siz;

		if ((outh = FScreat(name)) >= 0)	/*�������ļ�*/
		{
			while ((siz = FSread(inh, buf, sizeof(buf))) > 0)	/*��������*/
				FSwrite(outh, buf, siz);
			FSclose(outh);
		}
		FSclose(inh);
	}
}

/*ճ����ť����*/
void PasteBtnPressProc(CTRL_BTN *btn)
{
	if (op != OP_NONE)	/*�Ѽ��л����ļ�*/
	{
		CopyTree(PathBuf);
		if (op == OP_CUT)
		{
			char buf[MAX_PATH];
			FSGetCwd(buf, MAX_PATH);
			DelTree(PathBuf);
			FSChDir(buf);
		}
		op = OP_NONE;
		GCBtnSetDisable(PasteBtn, TRUE);
		FillFileList();
	}
}

/*ɾ����ť����*/
void DelBtnPressProc(CTRL_BTN *btn)
{
	if (FileList->SelItem)	/*ѡ���ļ�*/
	{
		DelTree(FileList->SelItem->text + 3);
		FillFileList();
	}
}

/*����Ŀ¼��ť����*/
void DirBtnPressProc(CTRL_BTN *btn)
{
}

/*�����ļ���ť����*/
void FileBtnPressProc(CTRL_BTN *btn)
{
}

/*��������ť����*/
void RenamBtnPressProc(CTRL_BTN *btn)
{
}

/*��������Ϣ����*/
long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			CTRL_ARGS args;

			GCWndGetClientLoca(wnd, &args.x, &args.y);
			args.width = wnd->client.width - SIDE * 2;
			args.height = EDT_HEIGHT;
			args.x += SIDE;
			args.y += SIDE;
			args.style = 0;
			args.MsgProc = NULL;
			GCSedtCreate(&DirSedt, &args, wnd->obj.gid, &wnd->obj, NULL, DirSedtEnterProc);
			args.width = PART_WIDTH;
			args.height = wnd->client.height - EDT_HEIGHT - SIDE * 3;
			args.y += EDT_HEIGHT + SIDE;
			args.style = 0;
			args.MsgProc = PartListMsgProc;
			GCLstCreate(&PartList, &args, wnd->obj.gid, &wnd->obj, PartListSelProc);
			args.width = wnd->client.width - PART_WIDTH - BTN_WIDTH - SIDE * 4;
			args.x += PART_WIDTH + SIDE;
			args.MsgProc = FileListMsgProc;
			GCLstCreate(&FileList, &args, wnd->obj.gid, &wnd->obj, FileListSelProc);
			args.x += args.width + SIDE;
			args.width = BTN_WIDTH;
			args.height = BTN_HEIGHT;
			args.MsgProc = NULL;
			GCBtnCreate(&ParBtn, &args, wnd->obj.gid, &wnd->obj, "�ϼ�Ŀ¼", NULL, ParBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			args.style = BTN_STYLE_DISABLED;
			GCBtnCreate(&CutBtn, &args, wnd->obj.gid, &wnd->obj, "����", NULL, CutBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&CopyBtn, &args, wnd->obj.gid, &wnd->obj, "����", NULL, CopyBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&PasteBtn, &args, wnd->obj.gid, &wnd->obj, "ճ��", NULL, PasteBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&DelBtn, &args, wnd->obj.gid, &wnd->obj, "ɾ��", NULL, DelBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			args.style = 0;
			GCBtnCreate(&DirBtn, &args, wnd->obj.gid, &wnd->obj, "����Ŀ¼", NULL, DirBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&FileBtn, &args, wnd->obj.gid, &wnd->obj, "�����ļ�", NULL, FileBtnPressProc);
			args.y += BTN_HEIGHT + SIDE;
			args.style = BTN_STYLE_DISABLED;
			GCBtnCreate(&RenamBtn, &args, wnd->obj.gid, &wnd->obj, "������", NULL, RenamBtnPressProc);
		}
		break;
	case GM_SIZE:
		{
			long x, y;
			DWORD width, height;

			GCWndGetClientLoca(wnd, &x, &y);
			width = wnd->client.width - SIDE * 2;
			height = EDT_HEIGHT;
			x += SIDE;
			y += SIDE;
			GCGobjSetSize(&DirSedt->obj, x, y, width, height);
			width = PART_WIDTH;
			height = wnd->client.height - EDT_HEIGHT - SIDE * 3;
			y += EDT_HEIGHT + SIDE;
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

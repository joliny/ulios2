/*	gmgr.c for ulios application
	作者：孙亮
	功能：图形化资源管理器程序
	最后修改日期：2012-02-13
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

THREAD_ID FsPtid;
CTRL_LST *PartList;	/*分区列表*/
CTRL_LST *FileList;	/*文件列表*/
CTRL_BTN *ParBtn;	/*父目录按钮*/
CTRL_BTN *CutBtn;	/*剪切按钮*/
CTRL_BTN *CopyBtn;	/*复制按钮*/
CTRL_BTN *PasteBtn;	/*粘贴按钮*/
CTRL_BTN *DelBtn;	/*删除按钮*/
CTRL_BTN *DirBtn;	/*创建目录按钮*/
CTRL_BTN *FileBtn;	/*创建文件按钮*/
CTRL_BTN *RenamBtn;	/*重命名按钮*/

/*双字转化为数字*/
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
	buf = p;	/*确定字符串尾部*/
	*p-- = '\0';
	while (p > q)	/*翻转字符串*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	return buf;
}

/*格式化输出*/
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

#define WND_WIDTH	400	/*窗口最小宽度,高度*/
#define WND_HEIGHT	300
#define SIDE		2	/*控件边距*/
#define PART_WIDTH	64	/*分区列表宽度*/
#define BTN_WIDTH	56	/*按钮宽度,高度*/
#define BTN_HEIGHT	20

void PartListSelProc(CTRL_LST *lst)
{
	if (lst->SelItem)	/*选中盘符*/
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
	if (lst->SelItem)	/*选中目录或文件*/
	{
		if (lst->SelItem->text[0] == '[')	/*打开目录*/
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
		else	/*执行程序*/
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
			GCBtnCreate(&ParBtn, &args, wnd->obj.gid, &wnd->obj, "上级目录", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&CutBtn, &args, wnd->obj.gid, &wnd->obj, "剪切", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&CopyBtn, &args, wnd->obj.gid, &wnd->obj, "复制", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&PasteBtn, &args, wnd->obj.gid, &wnd->obj, "粘贴", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&DelBtn, &args, wnd->obj.gid, &wnd->obj, "删除", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&DirBtn, &args, wnd->obj.gid, &wnd->obj, "创建目录", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&FileBtn, &args, wnd->obj.gid, &wnd->obj, "创建文件", NULL);
			args.y += BTN_HEIGHT + SIDE;
			GCBtnCreate(&RenamBtn, &args, wnd->obj.gid, &wnd->obj, "重命名", NULL);
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
	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = WND_WIDTH;
	args.height = WND_HEIGHT;
	args.x = (GCwidth - args.width) / 2;	/*居中*/
	args.y = (GCheight - args.height) / 2;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN | WND_STYLE_MAXBTN | WND_STYLE_MINBTN | WND_STYLE_SIZEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "资源管理器");
	wnd->MinWidth = WND_WIDTH;
	wnd->MinHeight = WND_HEIGHT;

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*处理GUI消息*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*销毁主窗体,退出程序*/
				break;
		}
	}
	return NO_ERROR;
}

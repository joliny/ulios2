/*	cmd.c for ulios application
	作者：孙亮
	功能：命令提示符程序
	最后修改日期：2010-06-14
*/

#include "../driver/basesrv.h"
#include "../fs/fsapi.h"

#define CMD_LEN		256
#define PROMPT		"命令:"

THREAD_ID TimePtid, FsPtid, CuiPtid;
char cmd[CMD_LEN], *cmdp;	/*输入命令缓冲*/

/*双字转化为数字*/
char *Itoa(char *buf, DWORD n, DWORD r)
{
	static char num[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	char *p, *q;

	q = p = buf;
	for (;;)
	{
		*p++ = num[n % r];
		n /= r;
		if (n == 0)
			break;
	}
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

/*16进制字符串转换为无符号整数*/
DWORD Atoi16(char *str)
{
	DWORD res;

	res = 0;
	for (;;)
	{
		if (*str >= '0' && *str <= '9')
			res = res * 16 + (*str - '0');
		else if (*str >= 'A' && *str <= 'F')
			res = res * 16 + (*str - 'A' + 10);
		else if (*str >= 'a' && *str <= 'f')
			res = res * 16 + (*str - 'a' + 10);
		else
			break;
		str++;
	}
	return res;
}

/*清屏*/
void cls(char *args)
{
	CUIClrScr(CuiPtid);
}

/*设置字符颜色和背景颜色*/
void SetColor(char *args)
{
	char *p;
	
	for (p = args;; p++)
	{
		if (*p == ' ')
		{
			*(p++) = '\0';
			break;
		}
		if (*p == '\0')
		{
			CUIPutS(CuiPtid, "参数: 前景色 背景色\n");
			return;
		}
	}
	CUISetCol(CuiPtid, Atoi16(args), Atoi16(p));
}

/*退出*/
void exitcmd(char *args)
{
	KExitProcess(NO_ERROR);
}

/*删除文件*/
void delfile(char *args)
{
	if (FSremove(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "删除文件出错！\n");
}

/*复制文件*/
void copy(char *args)
{
	char buf[4096], *bufp;
	long in, out, siz;

	for (bufp = args;; bufp++)
	{
		if (*bufp == ' ')
		{
			*(bufp++) = '\0';
			break;
		}
		if (*bufp == '\0')
		{
			CUIPutS(CuiPtid, "参数: 源文件路径 目的路径\n");
			return;
		}
	}
	if ((in = FSopen(FsPtid, args, 0)) < 0)
	{
		CUIPutS(CuiPtid, "源文件不存在！\n");
		return;
	}
	if ((out = FScreat(FsPtid, bufp)) < 0)
	{
		FSclose(FsPtid, in);
		CUIPutS(CuiPtid, "无法创建目的文件！\n");
		return;
	}
	while ((siz = FSread(FsPtid, in, buf, sizeof(buf))) > 0)
		FSwrite(FsPtid, out, buf, siz);
	FSclose(FsPtid, out);
	FSclose(FsPtid, in);
}

/*重命名*/
void ren(char *args)
{
	char *p;

	for (p = args;; p++)
	{
		if (*p == ' ')
		{
			*(p++) = '\0';
			break;
		}
		if (*p == '\0')
		{
			CUIPutS(CuiPtid, "参数: 目标路径 新名称\n");
			return;
		}
	}
	if (FSrename(FsPtid, args, p) != NO_ERROR)
		CUIPutS(CuiPtid, "重命名出错！\n");
}

/*显示目录列表*/
void dir(char *args)
{
	FILE_INFO fi;
	long dir;
	if ((dir = FSOpenDir(FsPtid, args)) < 0)
	{
		CUIPutS(CuiPtid, "目录不存在！\n");
		return;
	}
	while (FSReadDir(FsPtid, dir, &fi) == NO_ERROR)
	{
		TM tm;
		char buf[4096];

		TMLocalTime(TimePtid, fi.ModifyTime, &tm);
		Sprintf(buf, "%d-%d-%d\t%d:%d:%d   \t%s\t%d\t%s\n", tm.yer, tm.mon, tm.day, tm.hor, tm.min, tm.sec, (fi.attr & FILE_ATTR_DIREC) ? "目录" : "文件", (DWORD)fi.size, fi.name);
		CUIPutS(CuiPtid, buf);
	}
	FSclose(FsPtid, dir);
}

/*切换目录*/
void cd(char *args)
{
	if (FSChDir(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "目录不存在！\n");
}

/*创建目录*/
void md(char *args)
{
	if (FSMkDir(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "无法创建目录！\n");
}

/*删除空目录*/
void rd(char *args)
{
	if (FSremove(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "无法删除目录！\n");
}

/*显示时间*/
void showtim(char *args)
{
	TM tm;
	char buf[40];
	TMCurTime(TimePtid, &tm);
	Sprintf(buf, "现在时刻:%d年%d月%d日 %d时%d分%d秒\n", tm.yer, tm.mon, tm.day, tm.hor, tm.min, tm.sec);
	CUIPutS(CuiPtid, buf);
}

/*帮助*/
void help(char *args)
{
	CUIPutS(CuiPtid, 
		"cls:清屏\n"
		"color:设置显示颜色\n"
		"exit:退出\n"
		"del:删除文件\n"
		"copy:复制文件\n"
		"ren:重命名\n"
		"dir:目录列表\n"
		"cd:切换目录\n"
		"md:创建目录\n"
		"rd:删除空目录\n"
		"time:显示当前时间\n"
		"help:帮助\n");
}

/*命令匹配*/
char *cmdcmp(char *str1, char *str2)
{
	while (*str1)
		if (*str1++ != *str2++)
			return NULL;
	if (*str2 == '\0')
		return str2;
	if (*str2 == ' ')
		return ++str2;
	return NULL;
}

void CmdProc(char *str)
{
	struct
	{
		char *str;
		void (*cmd)(char *args);
	}CMD[] = {
		{"cls", cls},
		{"color", SetColor},
		{"exit", exitcmd},
		{"del", delfile},
		{"copy", copy},
		{"ren", ren},
		{"dir", dir},
		{"cd", cd},
		{"md", md},
		{"rd", rd},
		{"time", showtim},
		{"help", help}
	};
	long i;
	char *args;
	THREAD_ID ptid;
	char buf[40];
	for (i = 0; i < sizeof(CMD) / 8; i++)
		if ((args = cmdcmp(CMD[i].str, str)) != NULL)
		{
			CMD[i].cmd(args);	/*内部命令*/
			CUIPutS(CuiPtid, PROMPT);
			return;
		}
	for (args = str;; args++)
	{
		if (*args == ' ')
		{
			*args++ = '\0';
			break;
		}
		if (*args == '\0')
			break;
	}
	if (KCreateProcess(0, str, args, &ptid) != NO_ERROR)
		Sprintf(buf, "无效的命令或可执行文件!\n%s", PROMPT);
	else
		Sprintf(buf, "进程ID: %d\n%s", ptid.ProcID, PROMPT);
	CUIPutS(CuiPtid, buf);
}

/*键盘输入响应*/
void KeyProc(char c)
{
	switch (c)
	{
	case '\0':
		return;
	case '\b':
		if (cmdp != cmd)
		{
			cmdp--;	/*删除字符*/
			CUIPutC(CuiPtid, '\b');
		}
		break;
	case '\r':
		CUIPutC(CuiPtid, '\n');
		*cmdp = '\0';
		CmdProc(cmd);	/*执行命令*/
		cmdp = cmd;
		*cmdp = '\0';
		break;
	default:
		if (cmdp - cmd < CMD_LEN - 1)
		{
			if (c == '\t')
				c = ' ';
			*cmdp++ = c;
			CUIPutC(CuiPtid, c);
		}
		break;
	}
}

int main()
{
	THREAD_ID ptid;
	long res;

	if ((res = KGetKptThed(SRV_TIME_PORT, &TimePtid)) != NO_ERROR)
		return res;
	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)
		return res;
	if ((res = KGetKptThed(SRV_CUI_PORT, &CuiPtid)) != NO_ERROR)
		return res;
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &ptid)) != NO_ERROR)
		return res;
	if ((res = KMSetRecv(ptid)) != NO_ERROR)
		return res;
	CUIPutS(CuiPtid,
		"欢迎来到\n"
		"┏┓┏┓┏┓　　┏━━┓┏━━┓┏━━┓\n"
		"┃┃┃┃┃┃　　┗┓┏┛┃┏┓┃┃┏━┛\n"
		"┃┃┃┃┃┃　　　┃┃　┃┃┃┃┃┗━┓\n"
		"┃┃┃┃┃┃　　　┃┃　┃┃┃┃┗━┓┃\n"
		"┃┗┛┃┃┗━┓┏┛┗┓┃┗┛┃┏━┛┃\n"
		"┗━━┛┗━━┛┗━━┛┗━━┛┗━━┛\n"
		"输入help获得命令帮助!\n");
	CUIPutS(CuiPtid, PROMPT);
	cmdp = cmd;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
			break;
		if (data[0] == MSG_ATTR_KBD)	/*键盘消息*/
			KeyProc(data[1]);
	}
	return NO_ERROR;
}

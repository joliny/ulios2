/*	cmd.c for ulios application
	���ߣ�����
	���ܣ�������ʾ������
	����޸����ڣ�2010-06-14
*/

#include "../driver/basesrv.h"
#include "../fs/fsapi.h"
#include "../gui/guiapi.h"

#define CMD_LEN		256
#define PROMPT		"����:"

char cmd[CMD_LEN], *cmdp;	/*���������*/

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

/*10�����ַ���ת��Ϊ�޷�������*/
DWORD Atoi10(const char *str)
{
	DWORD res;

	res = 0;
	for (;;)
	{
		if (*str >= '0' && *str <= '9')
			res = res * 10 + (*str - '0');
		else
			break;
		str++;
	}
	return res;
}

/*16�����ַ���ת��Ϊ�޷�������*/
DWORD Atoi16(const char *str)
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

/*����*/
void cls(char *args)
{
	CUIClrScr();
}

/*�����ַ���ɫ�ͱ�����ɫ*/
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
			CUIPutS("����: ǰ��ɫ ����ɫ\n");
			return;
		}
	}
	CUISetCol(Atoi16(args), Atoi16(p));
}

/*�˳�*/
void exitcmd(char *args)
{
	KExitProcess(NO_ERROR);
}

/*ɾ���ļ�*/
void delfile(char *args)
{
	if (FSremove(args) != NO_ERROR)
		CUIPutS("�޷�ɾ����\n");
}

/*�����ļ�*/
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
			CUIPutS("����: Դ�ļ�·�� Ŀ��·��\n");
			return;
		}
	}
	if ((in = FSopen(args, FS_OPEN_READ)) < 0)
	{
		CUIPutS("Դ�ļ������ڣ�\n");
		return;
	}
	if ((out = FScreat(bufp)) < 0)
	{
		FSclose(in);
		CUIPutS("�޷�����Ŀ���ļ���\n");
		return;
	}
	while ((siz = FSread(in, buf, sizeof(buf))) > 0)
		FSwrite(out, buf, siz);
	FSclose(out);
	FSclose(in);
}

/*������*/
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
			CUIPutS("����: Ŀ��·�� ������\n");
			return;
		}
	}
	if (FSrename(args, p) != NO_ERROR)
		CUIPutS("����������\n");
}

/*��ʾ�����б�*/
void partlist(char *args)
{
	struct _PI_INFO
	{
		PART_INFO info;
		char fstype[8];
	}pi;
	DWORD pid;

	pid = 0;
	while (FSEnumPart(&pid) == NO_ERROR)
	{
		THREAD_ID ptid;
		char buf[4096];

		FSGetPart(pid, &pi.info);
		Sprintf(buf, "/%u\t����:%uMB\tʣ��:%uMB\t��ʽ:%s\t���:%s\n", pid, (DWORD)(pi.info.size / 0x100000), (DWORD)(pi.info.remain / 0x100000), pi.fstype, pi.info.label);
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*����ESC��*/
			{
				CUIPutS("�û�ȡ����\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*�˳�����*/
				KExitProcess(NO_ERROR);
		}
		pid++;
	}
}

/*��ʾĿ¼�б�*/
void dir(char *args)
{
	FILE_INFO fi;
	long dh;

	if ((dh = FSOpenDir(args)) < 0)
	{
		CUIPutS("Ŀ¼�����ڣ�\n");
		return;
	}
	while (FSReadDir(dh, &fi) == NO_ERROR)
	{
		THREAD_ID ptid;
		TM tm;
		char buf[4096];

		TMLocalTime(fi.ModifyTime, &tm);
		Sprintf(buf, "%d-%d-%d\t%d:%d:%d   \t%s\t%d\t%c%c%c%c%c%c\t%s\n", tm.yer, tm.mon, tm.day, tm.hor, tm.min, tm.sec, (fi.attr & FILE_ATTR_DIREC) ? "Ŀ¼" : "�ļ�", (DWORD)fi.size, (fi.attr & FILE_ATTR_RDONLY) ? 'R' : ' ', (fi.attr & FILE_ATTR_HIDDEN) ? 'H' : ' ', (fi.attr & FILE_ATTR_SYSTEM) ? 'S' : ' ', (fi.attr & FILE_ATTR_LABEL) ? 'L' : ' ', (fi.attr & FILE_ATTR_ARCH) ? 'A' : ' ', (fi.attr & FILE_ATTR_EXEC) ? 'X' : ' ', fi.name);
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*����ESC��*/
			{
				CUIPutS("�û�ȡ����\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*�˳�����*/
				KExitProcess(NO_ERROR);
		}
	}
	FSclose(dh);
}

/*�л�Ŀ¼*/
void cd(char *args)
{
	if (*args)
	{
		if (FSChDir(args) != NO_ERROR)
			CUIPutS("Ŀ¼�����ڣ�\n");
	}
	else
	{
		char path[MAX_PATH];
		long siz;
		if ((siz = FSGetCwd(path, MAX_PATH - 1)) < 0)
			CUIPutS("��ǰ·������\n");
		else
		{
			path[siz - 1] = '\n';
			path[siz] = '\0';
			CUIPutS(path);
		}
	}
}

/*����Ŀ¼*/
void md(char *args)
{
	if (FSMkDir(args) != NO_ERROR)
		CUIPutS("�޷�����Ŀ¼��\n");
}

void show(char *args)
{
	long fh, siz;
	char buf[4097];

	if ((fh = FSopen(args, FS_OPEN_READ)) < 0)
	{
		CUIPutS("�ļ������ڣ�\n");
		return;
	}
	while ((siz = FSread(fh, buf, 4096)) > 0)
	{
		THREAD_ID ptid;

		buf[siz] = '\0';
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*����ESC��*/
			{
				CUIPutS("�û�ȡ����\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*�˳�����*/
				KExitProcess(NO_ERROR);
		}
	}
	FSclose(fh);
}

/*��ʾʱ��*/
void showtim(char *args)
{
	static const char *WeekName[7] = {"��", "һ", "��", "��", "��", "��", "��"};
	TM tm;
	char buf[40];
	TMCurTime(&tm);
	Sprintf(buf, "����ʱ��:%d��%d��%d�� ����%s %dʱ%d��%d��\n", tm.yer, tm.mon, tm.day, WeekName[tm.wday], tm.hor, tm.min, tm.sec);
	CUIPutS(buf);
}

/*��ʾ�����б�*/
void proclist(char *args)
{
	FILE_INFO fi;
	DWORD pid;

	pid = 0;
	while (FSProcInfo(&pid, &fi) == NO_ERROR)
	{
		THREAD_ID ptid;
		char buf[4096];

		Sprintf(buf, "PID:%d\t%s\n", pid, fi.name);
		CUIPutS(buf);
		ptid.ProcID = SRV_CUI_PORT;
		ptid.ThedID = INVALID;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR)
		{
			if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_CUIKEY && buf[4] == 27)	/*����ESC��*/
			{
				CUIPutS("�û�ȡ����\n");
				break;
			}
			else if (((DWORD*)buf)[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)	/*�˳�����*/
				KExitProcess(NO_ERROR);
		}
		pid++;
	}
}

/*ɱ������*/
void killproc(char *args)
{
	if (KKillProcess(Atoi10(args)) != NO_ERROR)
		CUIPutS("ǿ�н�������ʧ�ܣ�\n");
}

/*����GUI������Ӧ�ó���*/
void startgui(char *args)
{
	THREAD_ID ptid;
	
	if (KGetKptThed(SRV_GUI_PORT, &ptid) == NO_ERROR)
	{
		CUIPutS("ͼ�ν�����������\n");
		return;
	}
	ptid.ProcID = SRV_CUI_PORT;
	ptid.ThedID = INVALID;
	SendExitProcReq(ptid);	/*�ر�CUI*/
	KCreateProcess(0, "gui.bin", NULL, &ptid);
	KSleep(5);	/*��ʱ,��ֹ���̼�������ϵ������*/
	KCreateProcess(0, "desktop.bin", NULL, &ptid);
	KSleep(5);
	KCreateProcess(0, "cui.bin", NULL, &ptid);	/*����ͼ��ģʽCUI*/
	KExitProcess(NO_ERROR);
}

/*����*/
void sound(char *args)
{
	if (SPKSound(Atoi10(args)) != NO_ERROR)
	{
		CUIPutS("�޷����ӵ�������������\n");
		return;
	}
	KSleep(100);
	SPKNosound();
}

/*����*/
void help(char *args)
{
	CUIPutS(
		"cls:����\n"
		"color rrggbb rrggbb:����ǰ���ͱ���ɫ\n"
		"exit:�˳�\n"
		"part:�����б�\n"
		"del path:ɾ���ļ����Ŀ¼\n"
		"copy SrcPath DestPath:�����ļ�\n"
		"ren path name:������\n"
		"dir DirPath:Ŀ¼�б�\n"
		"cd DirPath:��ʾ���л���ǰĿ¼\n"
		"md DirPath:����Ŀ¼\n"
		"show FilePath:��ʾ�ļ�����\n"
		"time:��ʾ��ǰʱ��\n"
		"ps:�����б�\n"
		"kill ProcID:ǿ�н�������\n"
		"startgui:����ͼ�ν���\n"
		"sound freq:��һ��Ƶ�ʷ���һ��\n"
		"help:����\n"
		"�����ִ���ļ�·�������иó���\n");
}

/*����ƥ��*/
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

void ProcStr(char *str, char **exec, char **args)
{
	(*args) = (*exec) = NULL;
	while (*str == ' ' || *str == '\t')	/*�������ǰ������Ŀո�*/
		str++;
	if (*str == '\0')
		return;
	if (*str == '\"')	/*˫�����ڵĲ�����������*/
	{
		*exec = ++str;
		while (*str != '\0' && *str != '\"')	/*����ƥ���˫����*/
			str++;
	}
	else	/*��ͨ�����ÿո�ָ�*/
	{
		*exec = str;
		while (*str != '\0' && *str != ' ' && *str != '\t')	/*�����ո�*/
			str++;
	}
	if (*str == '\0')
		return;
	*str++ = '\0';
	while (*str == ' ' || *str == '\t')	/*�������ǰ������Ŀո�*/
		str++;
	*args = str;
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
		{"part", partlist},
		{"del", delfile},
		{"copy", copy},
		{"ren", ren},
		{"dir", dir},
		{"cd", cd},
		{"md", md},
		{"show", show},
		{"time", showtim},
		{"ps", proclist},
		{"kill", killproc},
		{"startgui", startgui},
		{"sound", sound},
		{"help", help}
	};
	long i;
	char *exec, *args;
	THREAD_ID ptid;
	char buf[40];
	for (i = 0; i < sizeof(CMD) / 8; i++)
		if ((args = cmdcmp(CMD[i].str, str)) != NULL)
		{
			CMD[i].cmd(args);	/*�ڲ�����*/
			CUIPutS(PROMPT);
			return;
		}
	ProcStr(str, &exec, &args);
	if (KCreateProcess(0, exec, args, &ptid) != NO_ERROR)
		Sprintf(buf, "��Ч��������ִ���ļ�!\n%s", PROMPT);
	else
		Sprintf(buf, "����ID: %d\n%s", ptid.ProcID, PROMPT);
	CUIPutS(buf);
}

/*����������Ӧ*/
void KeyProc(char c)
{
	switch (c)
	{
	case '\0':
		return;
	case '\b':
		if (cmdp != cmd)
		{
			cmdp--;	/*ɾ���ַ�*/
			CUIPutC('\b');
		}
		break;
	case '\r':
		CUIPutC('\n');
		*cmdp = '\0';
		CmdProc(cmd);	/*ִ������*/
		cmdp = cmd;
		*cmdp = '\0';
		break;
	default:
		if (cmdp - cmd < CMD_LEN - 1)
		{
			if (c == '\t')
				c = ' ';
			*cmdp++ = c;
			CUIPutC(c);
		}
		break;
	}
}

int main()
{
	THREAD_ID ptid;
	long res;

	if ((res = CUISetRecv()) != NO_ERROR)
		return res;
	CUIPutS(
		"��ӭ����\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����help����������!\n");
	CUIPutS(PROMPT);
	cmdp = cmd;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
			break;
		if (data[MSG_ATTR_ID] == MSG_ATTR_CUIKEY)	/*������Ϣ*/
			KeyProc(data[1]);
		else if (data[MSG_ATTR_ID] == MSG_ATTR_EXTPROCREQ)
			break;
	}
	return NO_ERROR;
}

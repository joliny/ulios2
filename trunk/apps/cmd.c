/*	cmd.c for ulios application
	���ߣ�����
	���ܣ�������ʾ������
	����޸����ڣ�2010-06-14
*/

#include "../driver/basesrv.h"
#include "../fs/fsapi.h"

#define CMD_LEN		256
#define PROMPT		"����:"

THREAD_ID TimePtid, FsPtid, KbdPtid, CuiPtid;
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
	CUIClrScr(CuiPtid);
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
			CUIPutS(CuiPtid, "����: ǰ��ɫ ����ɫ\n");
			return;
		}
	}
	CUISetCol(CuiPtid, Atoi16(args), Atoi16(p));
}

/*�˳�*/
void exitcmd(char *args)
{
	SendExitReq(FsPtid);
	KExitProcess(NO_ERROR);
}

/*ɾ���ļ�*/
void delfile(char *args)
{
	if (FSremove(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "�޷�ɾ����\n");
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
			CUIPutS(CuiPtid, "����: Դ�ļ�·�� Ŀ��·��\n");
			return;
		}
	}
	if ((in = FSopen(FsPtid, args, FS_OPEN_READ)) < 0)
	{
		CUIPutS(CuiPtid, "Դ�ļ������ڣ�\n");
		return;
	}
	if ((out = FScreat(FsPtid, bufp)) < 0)
	{
		FSclose(FsPtid, in);
		CUIPutS(CuiPtid, "�޷�����Ŀ���ļ���\n");
		return;
	}
	while ((siz = FSread(FsPtid, in, buf, sizeof(buf))) > 0)
		FSwrite(FsPtid, out, buf, siz);
	FSclose(FsPtid, out);
	FSclose(FsPtid, in);
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
			CUIPutS(CuiPtid, "����: Ŀ��·�� ������\n");
			return;
		}
	}
	if (FSrename(FsPtid, args, p) != NO_ERROR)
		CUIPutS(CuiPtid, "����������\n");
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
	while (FSEnumPart(FsPtid, &pid) == NO_ERROR)
	{
		THREAD_ID ptid;
		char buf[4096];

		FSGetPart(FsPtid, pid, &pi.info);
		Sprintf(buf, "/%u\t����:%uMB\tʣ��:%uMB\t��ʽ:%s\t���:%s\n", pid, (DWORD)(pi.info.size / 0x100000), (DWORD)(pi.info.remain / 0x100000), pi.fstype, pi.info.label);
		CUIPutS(CuiPtid, buf);
		ptid = KbdPtid;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR && ((DWORD*)buf)[0] == MSG_ATTR_KBD && buf[4] == 27)
		{
			CUIPutS(CuiPtid, "�û�ȡ����\n");
			break;
		}
		pid++;
	}
}

/*��ʾĿ¼�б�*/
void dir(char *args)
{
	FILE_INFO fi;
	long dhl;

	if ((dhl = FSOpenDir(FsPtid, args)) < 0)
	{
		CUIPutS(CuiPtid, "Ŀ¼�����ڣ�\n");
		return;
	}
	while (FSReadDir(FsPtid, dhl, &fi) == NO_ERROR)
	{
		THREAD_ID ptid;
		TM tm;
		char buf[4096];

		TMLocalTime(TimePtid, fi.ModifyTime, &tm);
		Sprintf(buf, "%d-%d-%d\t%d:%d:%d   \t%s\t%d\t%c%c%c%c%c%c\t%s\n", tm.yer, tm.mon, tm.day, tm.hor, tm.min, tm.sec, (fi.attr & FILE_ATTR_DIREC) ? "Ŀ¼" : "�ļ�", (DWORD)fi.size, (fi.attr & FILE_ATTR_RDONLY) ? 'R' : ' ', (fi.attr & FILE_ATTR_HIDDEN) ? 'H' : ' ', (fi.attr & FILE_ATTR_SYSTEM) ? 'S' : ' ', (fi.attr & FILE_ATTR_LABEL) ? 'L' : ' ', (fi.attr & FILE_ATTR_ARCH) ? 'A' : ' ', (fi.attr & FILE_ATTR_EXEC) ? 'X' : ' ', fi.name);
		CUIPutS(CuiPtid, buf);
		ptid = KbdPtid;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR && ((DWORD*)buf)[0] == MSG_ATTR_KBD && buf[4] == 27)
		{
			CUIPutS(CuiPtid, "�û�ȡ����\n");
			break;
		}
	}
	FSclose(FsPtid, dhl);
}

/*�л�Ŀ¼*/
void cd(char *args)
{
	if (FSChDir(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "Ŀ¼�����ڣ�\n");
}

/*����Ŀ¼*/
void md(char *args)
{
	if (FSMkDir(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "�޷�����Ŀ¼��\n");
}

/*��ʾʱ��*/
void showtim(char *args)
{
	static const char *WeekName[7] = {"��", "һ", "��", "��", "��", "��", "��"};
	TM tm;
	char buf[40];
	TMCurTime(TimePtid, &tm);
	Sprintf(buf, "����ʱ��:%d��%d��%d�� ����%s %dʱ%d��%d��\n", tm.yer, tm.mon, tm.day, WeekName[tm.wday], tm.hor, tm.min, tm.sec);
	CUIPutS(CuiPtid, buf);
}

/*��ʾ�����б�*/
void proclist(char *args)
{
	FILE_INFO fi;
	DWORD pid;

	pid = 0;
	while (FSProcInfo(FsPtid, &pid, &fi) == NO_ERROR)
	{
		THREAD_ID ptid;
		char buf[4096];

		Sprintf(buf, "PID:%d\t%s\n", pid, fi.name);
		CUIPutS(CuiPtid, buf);
		ptid = KbdPtid;
		if (KRecvProcMsg(&ptid, (DWORD*)buf, 0) == NO_ERROR && ((DWORD*)buf)[0] == MSG_ATTR_KBD && buf[4] == 27)
		{
			CUIPutS(CuiPtid, "�û�ȡ����\n");
			break;
		}
		pid++;
	}
}

/*ɱ������*/
void killproc(char *args)
{
	if (KKillProcess(Atoi10(args)) != NO_ERROR)
		CUIPutS(CuiPtid, "ǿ�н�������ʧ�ܣ�\n");
}

/*����*/
void sound(char *args)
{
	THREAD_ID SpkPtid;

	if (KGetKptThed(SRV_SPK_PORT, &SpkPtid) != NO_ERROR)
	{
		CUIPutS(CuiPtid, "����������δ������\n");
		return;
	}
	SPKSound(SpkPtid, Atoi10(args));
	KSleep(100);
	SPKNosound(SpkPtid);
}

/*����*/
void help(char *args)
{
	CUIPutS(CuiPtid,
		"cls:����\n"
		"color rrggbb rrggbb:����ǰ���ͱ���ɫ\n"
		"exit:�˳�\n"
		"part:�����б�\n"
		"del path:ɾ���ļ����Ŀ¼\n"
		"copy SrcPath DestPath:�����ļ�\n"
		"ren path name:������\n"
		"dir DirPath:Ŀ¼�б�\n"
		"cd DirPath:�л�Ŀ¼\n"
		"md DirPath:����Ŀ¼\n"
		"time:��ʾ��ǰʱ��\n"
		"ps:�����б�\n"
		"kill ProcID:ǿ�н�������\n"
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
		{"time", showtim},
		{"ps", proclist},
		{"kill", killproc},
		{"sound", sound},
		{"help", help}
	};
	long i;
	char *args;
	THREAD_ID ptid;
	char buf[40];
	for (i = 0; i < sizeof(CMD) / 8; i++)
		if ((args = cmdcmp(CMD[i].str, str)) != NULL)
		{
			CMD[i].cmd(args);	/*�ڲ�����*/
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
		Sprintf(buf, "��Ч��������ִ���ļ�!\n%s", PROMPT);
	else
		Sprintf(buf, "����ID: %d\n%s", ptid.ProcID, PROMPT);
	CUIPutS(CuiPtid, buf);
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
			CUIPutC(CuiPtid, '\b');
		}
		break;
	case '\r':
		CUIPutC(CuiPtid, '\n');
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
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &KbdPtid)) != NO_ERROR)
		return res;
	if ((res = KMSetRecv(KbdPtid)) != NO_ERROR)
		return res;
	CUIPutS(CuiPtid,
		"��ӭ����\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����������������������������������������\n"
		"����help����������!\n");
	CUIPutS(CuiPtid, PROMPT);
	cmdp = cmd;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
			break;
		if (ptid.ProcID == KbdPtid.ProcID && data[0] == MSG_ATTR_KBD)	/*������Ϣ*/
			KeyProc(data[1]);
	}
	return NO_ERROR;
}

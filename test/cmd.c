/*	cmd.c for ulios application
	���ߣ�����
	���ܣ�������ʾ������
	����޸����ڣ�2010-06-14
*/

#include "../driver/basesrv.h"
#include "../fs/fsapi.h"

#define CMD_LEN		256
#define PROMPT		"����:"

THREAD_ID TimePtid, FsPtid, CuiPtid;
char cmd[CMD_LEN], *cmdp;	/*���������*/

/*˫��ת��Ϊ����*/
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

/*16�����ַ���ת��Ϊ�޷�������*/
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
	KExitProcess(NO_ERROR);
}

/*ɾ���ļ�*/
void delfile(char *args)
{
	if (FSremove(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "ɾ���ļ�����\n");
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
	if ((in = FSopen(FsPtid, args, 0)) < 0)
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

/*��ʾĿ¼�б�*/
void dir(char *args)
{
	FILE_INFO fi;
	long dir;
	if ((dir = FSOpenDir(FsPtid, args)) < 0)
	{
		CUIPutS(CuiPtid, "Ŀ¼�����ڣ�\n");
		return;
	}
	while (FSReadDir(FsPtid, dir, &fi) == NO_ERROR)
	{
		TM tm;
		char buf[4096];

		TMLocalTime(TimePtid, fi.ModifyTime, &tm);
		Sprintf(buf, "%d-%d-%d\t%d:%d:%d   \t%s\t%d\t%s\n", tm.yer, tm.mon, tm.day, tm.hor, tm.min, tm.sec, (fi.attr & FILE_ATTR_DIREC) ? "Ŀ¼" : "�ļ�", (DWORD)fi.size, fi.name);
		CUIPutS(CuiPtid, buf);
	}
	FSclose(FsPtid, dir);
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

/*ɾ����Ŀ¼*/
void rd(char *args)
{
	if (FSremove(FsPtid, args) != NO_ERROR)
		CUIPutS(CuiPtid, "�޷�ɾ��Ŀ¼��\n");
}

/*��ʾʱ��*/
void showtim(char *args)
{
	TM tm;
	char buf[40];
	TMCurTime(TimePtid, &tm);
	Sprintf(buf, "����ʱ��:%d��%d��%d�� %dʱ%d��%d��\n", tm.yer, tm.mon, tm.day, tm.hor, tm.min, tm.sec);
	CUIPutS(CuiPtid, buf);
}

/*����*/
void help(char *args)
{
	CUIPutS(CuiPtid, 
		"cls:����\n"
		"color:������ʾ��ɫ\n"
		"exit:�˳�\n"
		"del:ɾ���ļ�\n"
		"copy:�����ļ�\n"
		"ren:������\n"
		"dir:Ŀ¼�б�\n"
		"cd:�л�Ŀ¼\n"
		"md:����Ŀ¼\n"
		"rd:ɾ����Ŀ¼\n"
		"time:��ʾ��ǰʱ��\n"
		"help:����\n");
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
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &ptid)) != NO_ERROR)
		return res;
	if ((res = KMSetRecv(ptid)) != NO_ERROR)
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
		if (data[0] == MSG_ATTR_KBD)	/*������Ϣ*/
			KeyProc(data[1]);
	}
	return NO_ERROR;
}

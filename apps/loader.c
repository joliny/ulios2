/*	loader.c for ulios application
	���ߣ�����
	���ܣ��û�Ӧ�ó��������
	����޸����ڣ�2010-05-19
*/

#include "../fs/fsapi.h"

#define LOADLIST_SIZ	10240

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

int main()
{
	char LoadList[LOADLIST_SIZ], *path;
	DWORD siz;
	THREAD_ID ptid;
	void *addr;
	long res;

	if ((res = KMapPhyAddr(&addr, 0x90280, 0x7C)) != NO_ERROR)	/*ȡ��ϵͳĿ¼*/
		return res;
	if ((res = FSChDir((const char*)addr)) != NO_ERROR)	/*�л���ϵͳĿ¼*/
		return res;
	KFreeAddr(addr);
	if ((res = FSopen("bootlist", FS_OPEN_READ)) < 0)	/*��ȡ�����ļ�*/
		return res;
	siz = FSread(res, LoadList, LOADLIST_SIZ - 1);
	FSclose(res);
	path = LoadList;
	LoadList[siz] = '\0';
	for (;;)
	{
		char *pathp = path, *exec, *args;

		KSleep(5);	/*��ʱ,��ֹ���̼�������ϵ������*/
		while (*pathp != '\n' && *pathp != '\0')
			pathp++;
		if (*pathp)
			*pathp++ = '\0';
		switch (*path++)
		{
		case 'D':	/*driver*/
		case 'd':
			ProcStr(path, &exec, &args);
			KCreateProcess(EXEC_ATTR_DRIVER, exec, args, &ptid);
			break;
		case 'A':	/*apps*/
		case 'a':
			ProcStr(path, &exec, &args);
			KCreateProcess(0, exec, args, &ptid);
			break;
		}
		if (*(path = pathp) == '\0')	/*û���ļ���*/
			break;
	}
	return NO_ERROR;
}

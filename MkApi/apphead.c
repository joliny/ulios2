/*	apphead.c for ulios program
	���ߣ�����
	���ܣ�Ӧ�ó������ڴ���,��ʽ�����������ó����C����main����
	����޸����ڣ�2010-05-25
*/

#include "ulimkapi.h"

extern int main(int argc, char *argv[]);

void start(char *args)
{
	int argc;
	char *argv[0x100];

	for (argc = 1;;)
	{
		while (*args == ' ' || *args == '\t')	/*�������ǰ������Ŀո�*/
			args++;
		if (*args == '\0')
			break;
		if (*args == '\"')	/*˫�����ڵĲ�����������*/
		{
			argv[argc++] = ++args;
			while (*args != '\0' && *args != '\"')	/*����ƥ���˫����*/
				args++;
		}
		else	/*��ͨ�����ÿո�ָ�*/
		{
			argv[argc++] = args;
			while (*args != '\0' && *args != ' ' && *args != '\t')	/*�����ո�*/
				args++;
		}
		if (*args == '\0')
			break;
		*args++ = '\0';
	}
	KExitProcess(main(argc, argv));
}

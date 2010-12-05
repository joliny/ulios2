/*	loader.c for ulios application
	���ߣ�����
	���ܣ��û�Ӧ�ó��������
	����޸����ڣ�2010-05-19
*/

#include "../fs/fsapi.h"

#define LOADLIST_SIZ	10240

int main()
{
	char LoadList[LOADLIST_SIZ], *path;
	DWORD siz;
	THREAD_ID ptid;
	void *addr;
	long res;

	if ((res = KGetKptThed(SRV_FS_PORT, &ptid)) != NO_ERROR)	/*ȡ���ļ�ϵͳ�߳�ID*/
		return res;
	if ((res = KMapPhyAddr(&addr, 0x90280, 0x7C)) != NO_ERROR)	/*ȡ��ϵͳĿ¼*/
		return res;
	if ((res = FSChDir(ptid, (const char*)addr)) != NO_ERROR)	/*�л���ϵͳĿ¼*/
		return res;
	KFreeAddr(addr);
	if ((res = FSopen(ptid, "loadlist.txt", FS_OPEN_READ)) < 0)	/*��ȡ�����ļ�*/
		return res;
	siz = FSread(ptid, res, LoadList, LOADLIST_SIZ - 1);
	FSclose(ptid, res);
	path = LoadList;
	LoadList[siz] = '\0';
	for (;;)
	{
		char *pathp = path;

		while (*pathp != '\n' && *pathp != '\0')
			pathp++;
		if (*pathp)
			*pathp++ = '\0';
		switch (*path++)
		{
		case 'D':	/*driver*/
		case 'd':
			KCreateProcess(EXEC_ATTR_DRIVER, path, NULL, &ptid);
			break;
		case 'A':	/*apps*/
		case 'a':
			KCreateProcess(0, path, NULL, &ptid);
			break;
		}
		if (*(path = pathp) == '\0')	/*û���ļ���*/
			break;
	}
	return NO_ERROR;
}

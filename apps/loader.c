/*	loader.c for ulios application
	���ߣ�����
	���ܣ��û�Ӧ�ó��������
	����޸����ڣ�2010-05-19
*/

#include "../fs/fsapi.h"

int main()
{
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
	KCreateProcess(0, "cmd.bin", NULL, &ptid);
	return NO_ERROR;
}

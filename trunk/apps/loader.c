/*	loader.c for ulios application
	作者：孙亮
	功能：用户应用程序加载器
	最后修改日期：2010-05-19
*/

#include "../fs/fsapi.h"

int main()
{
	THREAD_ID ptid;
	void *addr;
	long res;

	if ((res = KGetKptThed(SRV_FS_PORT, &ptid)) != NO_ERROR)	/*取得文件系统线程ID*/
		return res;
	if ((res = KMapPhyAddr(&addr, 0x90280, 0x7C)) != NO_ERROR)	/*取得系统目录*/
		return res;
	if ((res = FSChDir(ptid, (const char*)addr)) != NO_ERROR)	/*切换到系统目录*/
		return res;
	KFreeAddr(addr);
	KCreateProcess(0, "cmd.bin", NULL, &ptid);
	return NO_ERROR;
}

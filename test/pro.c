/*	pro.c
	作者：孙亮
	功能：可执行文件测试程序
	最后修改日期：2010-05-14
*/

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"

int main()
{
	THREAD_ID VesaPtid;
	long res;

	if ((res = KGetKpToThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	KSleep(1000);
	VSDrawStr(VesaPtid, 100, 100, (const BYTE*)"老婆老婆我爱你，就像老鼠爱大米", 0xFFFFFF);
	return NO_ERROR;
}

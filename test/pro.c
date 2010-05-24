/*	pro.c
	作者：孙亮
	功能：可执行文件测试程序
	最后修改日期：2010-05-14
*/

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"

int main(char *args)
{
	THREAD_ID VesaPtid;
	long res;

	KSleep(500);
	if ((res = KGetKptThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	VSDrawStr(VesaPtid, 200, 180, "hello:", 0xFFFF00);
	VSDrawStr(VesaPtid, 200, 200, args, 0xFFFF00);
	return NO_ERROR;
}

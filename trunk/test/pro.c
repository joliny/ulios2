/*	pro.c
	作者：孙亮
	功能：可执行文件测试程序
	最后修改日期：2010-05-14
*/

#include "../driver/basesrv.h"

int main(int argc, char *argv[])
{
	THREAD_ID ptid;
	long res;

	KSleep(300);
	if ((res = KGetKptThed(SRV_CUI_PORT, &ptid)) != NO_ERROR)
		return res;
	CUISetCur(ptid, 10, 24);
	CUIPutS(ptid, "This is a args test pro!");
	for (res = 0; res < 10; res++)
		CUIPutS(ptid, "老婆，CUI搞定啦 哈哈\n");
	return NO_ERROR;
}

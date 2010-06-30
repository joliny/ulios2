/*	pro.c
	作者：孙亮
	功能：可执行文件测试程序
	最后修改日期：2010-05-14
*/

#include "../driver/basesrv.h"

#define WIDTH	800
#define HEIGHT	600
#define PN		50

int main()
{
	float x[PN], y[PN], z[PN];
	long sx[PN], sy[PN];
	THREAD_ID ptid;
	long i, res;

	if ((res = KGetKptThed(SRV_TIME_PORT, &ptid)) != NO_ERROR)
		return res;
	for (i = 0; i < PN; i++)
	{
		x[i] = (float)((TMGetRand(ptid) & 0x1FF) - 256);
		y[i] = (float)((TMGetRand(ptid) & 0x1FF) - 256);
		z[i] = (float)((TMGetRand(ptid) & 0x1FF) - 256);
	}
	KSleep(100);
	if ((res = GDIinit()) != NO_ERROR)
		return res;
	GDIFillRect(0, 0, WIDTH, HEIGHT, 0x4080FF);
	GDIDrawStr(0, 0, "welcome to ulios 3Dline Demo!", 0xABCDEF);
	for (res = 0; res < 1000; res++)
	{
		for (i = 0; i < PN - 1; i++)
			GDIDrawLine(sx[i], sy[i], sx[i + 1], sy[i + 1], 0x4080FF);
		for (i = 0; i < PN; i++)
		{
			float temp = x[i];
			x[i] = x[i] * 0.99955f + y[i] * 0.0299955f;
			y[i] = y[i] * 0.99955f - temp * 0.0299955f;
			sx[i] = (long)(1000.0f * y[i] / (1400.0f - x[i])) + 400;
			sy[i] = 300 - (long)(1000.0f * z[i] / (1400.0f - x[i]));
		}
		for (i = 0; i < PN - 1; i++)
			GDIDrawLine(sx[i], sy[i], sx[i + 1], sy[i + 1], 0xFFFFFF);
		KSleep(5);
	}
	GDIrelease();
	return NO_ERROR;
}

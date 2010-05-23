/*	pro.c
	作者：孙亮
	功能：可执行文件测试程序
	最后修改日期：2010-05-14
*/

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"

#define WIDTH 800
#define HEIGHT 600
#define PN 100

int main()
{
	THREAD_ID VesaPtid;
	long res;
	float x[PN], y[PN], z[PN];
	short sx[PN], sy[PN];

	KSleep(500);
	if ((res = KGetKpToThed(SRV_TIME_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	for (res = 0; res < PN; res++)
	{
		x[res] = (float)((TMGetRand(VesaPtid) & 0x1FF) - 256);
		y[res] = (float)((TMGetRand(VesaPtid) & 0x1FF) - 256);
		z[res] = (float)((TMGetRand(VesaPtid) & 0x1FF) - 256);
	}
	if ((res = KGetKpToThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	for (;;)
	{
		for (res = 0; res < PN - 1; res++)
			VSDrawLine(VesaPtid, sx[res], sy[res], sx[res + 1], sy[res + 1], 0xFF00);
		for (res = 0; res < PN; res++)
		{
			float temp = x[res];
			x[res] = x[res] * 0.99955f + y[res] * 0.0299955f;
			y[res] = y[res] * 0.99955f - temp * 0.0299955f;
			sx[res] = (int)(1000.0f * y[res] / (1400.0f - x[res])) + 400;
			sy[res] = 300 - (int)(1000.0f * z[res] / (1400.0f - x[res]));
		}
		for (res = 0; res < PN - 1; res++)
			VSDrawLine(VesaPtid, sx[res], sy[res], sx[res + 1], sy[res + 1], 0xFFFFFF);
		KSleep(5);
	}
	return NO_ERROR;
}

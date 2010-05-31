/*	test.c
	作者：孙亮
	功能：ulios系统内核测试程序
	最后修改日期：2009-05-28
*/

#include "../driver/basesrv.h"
#include "../fs/fsapi.h"

char *itoa(char *buf, DWORD n)
{
	char *p, *q;

	p = q = buf;
	do
	{
		*p++ = n % 10 + '0';
		n /= 10;
	} while (n);
	buf = p;	/*确定字符串尾部*/
	*p-- = 0;
	while (p > q)	/*翻转字符串*/
	{
		n = *q;
		*q++ = *p;
		*p-- = n;
	}
	return buf;
}

int main()
{
	THREAD_ID FsPtid, VesaPtid;
	long res, i, j;
	DWORD bmp[64 * 64];

	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)
		return res;
	FSChDir(FsPtid, "/0/ulios");
	if ((res = FSopen(FsPtid, "uli2k.bmp", 0)) < 0)	/*打开BMP文件*/
		return res;
	FSseek(FsPtid, res, 54, FS_SEEK_SET);
	for (j = 63; j >= 0; j--)
	{
		if (FSread(FsPtid, res, bmp + 64 * j, 64 * 3) <= 0)	/*开始读取BMP文件*/
			return -1;
		for (i = 63; i >= 0; i--)
		{
			DWORD s = 64 * j * 4 + i * 3, d = 64 * j * 4 + i * 4;
			((BYTE*)bmp)[d + 3] = 0;
			((BYTE*)bmp)[d + 2] = ((BYTE*)bmp)[s + 2];
			((BYTE*)bmp)[d + 1] = ((BYTE*)bmp)[s + 1];
			((BYTE*)bmp)[d] = ((BYTE*)bmp)[s];
		}
	}
	FSclose(FsPtid, res);
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	if ((res = KMSetRecv(VesaPtid)) != NO_ERROR)
		return res;
	if ((res = KGetKptThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
			break;
		if (data[0] == MSG_ATTR_KBD)
		{
			WORD mode[VESA_MAX_MODE];
			char str[16];
			DWORD ModeCou, i;

			ModeCou = VSGetMode(VesaPtid, mode);
			itoa(str, ModeCou);
			VSDrawStr(VesaPtid, 120, 0, str, 0xFF0000);
			for (i = 0; i < ModeCou; i++)
			{
				itoa(str, mode[i]);
				VSDrawStr(VesaPtid, 0, i * 12, str, 0xFF0000);
			}
			KCreateProcess(0, "pro.bin", "hello laopo heihei ", (THREAD_ID*)&ModeCou);

/*			if (data[1] & KBD_STATE_LSHIFT)
				KPrintf("LSHIFT\t", 0);
			if (data[1] & KBD_STATE_RSHIFT)
				KPrintf("RSHIFT\t", 0);
			if (data[1] & KBD_STATE_LCTRL)
				KPrintf("LCTRL\t", 0);
			if (data[1] & KBD_STATE_RCTRL)
				KPrintf("RCTRL\t", 0);
			if (data[1] & KBD_STATE_LALT)
				KPrintf("LALT\t", 0);
			if (data[1] & KBD_STATE_RALT)
				KPrintf("RALT\t", 0);
			if (data[1] & KBD_STATE_PAUSE)
				KPrintf("PAUSE\t", 0);
			if (data[1] & KBD_STATE_PRTSC)
				KPrintf("PRTSC\t", 0);
			if (data[1] & KBD_STATE_SCRLOCK)
				KPrintf("SCRLOCK\t", 0);
			if (data[1] & KBD_STATE_NUMLOCK)
				KPrintf("NUMLOCK\t", 0);
			if (data[1] & KBD_STATE_CAPSLOCK)
				KPrintf("CAPSLOCK\t", 0);
			if (data[1] & KBD_STATE_INSERT)
				KPrintf("INSERT\t", 0);
			KPrintf("SCAN:%d\t", (data[1] >> 8) & 0xFF);
			KPrintf("CHAR:%c\n", data[1] & 0xFF);
*/		}
		if (data[0] == MSG_ATTR_MUS)
		{
			DWORD col;
/*			if (data[1] & MUS_STATE_LBUTTON)
				KPrintf("LBTN\t", 0);
			if (data[1] & MUS_STATE_RBUTTON)
				KPrintf("RBTN\t", 0);
			if (data[1] & MUS_STATE_MBUTTON)
				KPrintf("MBTN\t", 0);
			KPrintf("X:%d\t", data[2]);
			KPrintf("Y:%d\t", data[3]);
			KPrintf("Z:%d\n", data[4]);
*/			if (data[1] & MUS_STATE_LBUTTON)
				col = 0xFF;
			else if (data[1] & MUS_STATE_RBUTTON)
				col = 0xFF00;
			else if (data[1] & MUS_STATE_MBUTTON)
				col = 0xFF0000;
			else
				col = 0xFFFFFF;
			VSPutImage(VesaPtid, data[2], data[3], bmp, 64, 64);
		}
	}
	return NO_ERROR;
}

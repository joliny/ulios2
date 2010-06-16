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

void DrawWindow(long x, long y, long w, long h)
{
	GDIFillRect(x, y, w - 1, 1, 0xD4D0C8);	/*top*/
	GDIFillRect(x, y + 1, 1, h - 2, 0xD4D0C8);	/*left*/
	GDIFillRect(x + w - 1, y, 1, h, 0x404040);	/*right*/
	GDIFillRect(x, y + h - 1, w - 1, 1, 0x404040);	/*bottom*/

	GDIFillRect(x + 1, y + 1, w - 3, 1, 0xFFFFFF);	/*top*/
	GDIFillRect(x + 1, y + 2, 1, h - 4, 0xFFFFFF);	/*left*/
	GDIFillRect(x + w - 2, y + 1, 1, h - 2, 0x808080);	/*right*/
	GDIFillRect(x + 1, y + h - 2, w - 3, 1, 0x808080);	/*bottom*/

	GDIFillRect(x + 2, y + 2, w - 4, h - 4, 0xD4D0C8);	/*center*/
	GDIFillRect(x + 3, y + 3, w - 6, 16, 0x5270A7);	/*title*/
}

int main()
{
	THREAD_ID ptid;
	long res, i, j;
	DWORD *bmp;
	DWORD tempimg[80 * 60];
	long tempx, tempy;
	
	if ((res = KGetKptThed(SRV_FS_PORT, &ptid)) != NO_ERROR)
		return res;
	KMapUserAddr((void**)&bmp, 800 * 600 * sizeof(DWORD));
	FSChDir(ptid, "/0/ulios");
	if ((res = FSopen(ptid, "desktop.bmp", 0)) < 0)	/*打开BMP文件*/
		return res;
	FSseek(ptid, res, 54, FS_SEEK_SET);
	for (j = 599; j >= 0; j--)
	{
		if (FSread(ptid, res, bmp + 800 * j, 800 * 3) <= 0)	/*开始读取BMP文件*/
			return -1;
		for (i = 799; i >= 0; i--)
		{
			DWORD s = 800 * j * 4 + i * 3, d = 800 * j * 4 + i * 4;
			((BYTE*)bmp)[d + 3] = 0;
			((BYTE*)bmp)[d + 2] = ((BYTE*)bmp)[s + 2];
			((BYTE*)bmp)[d + 1] = ((BYTE*)bmp)[s + 1];
			((BYTE*)bmp)[d] = ((BYTE*)bmp)[s];
		}
	}
	FSclose(ptid, res);

	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &ptid)) != NO_ERROR)
		return res;
	if ((res = KMSetRecv(ptid)) != NO_ERROR)
		return res;
	if ((res = GDIinit()) != NO_ERROR)
		return res;
	GDIPutImage(0, 0, bmp, 800, 600);
	tempx = 0;
	tempy = 0;
	GDIGetImage(tempx, tempy, tempimg, 80, 60);
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
			break;
		if (data[0] == MSG_ATTR_KBD)
		{
			WORD mode[VESA_MAX_MODE];
			char str[16];
			DWORD ModeCou, CurMode, i;

			VSGetMode(GDIVesaPtid, mode, &ModeCou, &CurMode);
			itoa(str, ModeCou);
			GDIDrawStr(120, 0, str, 0xFF0000);
			itoa(str, CurMode);
			GDIDrawStr(120, 12, str, 0xFF0000);
			for (i = 0; i < ModeCou; i++)
			{
				itoa(str, mode[i]);
				GDIDrawStr(0, i * 12, str, 0xFF0000);
			}
			KCreateProcess(0, "pro.bin", "hello ulios \"lao po\" lao po i AI u", (THREAD_ID*)&ModeCou);

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
			//GDIDrawStr(data[2], data[3], "Press Key", col);
			GDIPutImage(tempx, tempy, tempimg, 80, 60);
			tempx = data[2];
			tempy = data[3];
			GDIGetImage(tempx, tempy, tempimg, 80, 60);
			DrawWindow(data[2], data[3], 80, 60);
		}
	}
	GDIrelease();
	return NO_ERROR;
}

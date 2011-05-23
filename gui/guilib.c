/*	guilib.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面功能库
	最后修改日期：2011-05-22
*/

#include "gui.h"
#include "../fs/fsapi.h"

long LoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height)
{
	BYTE BmpHead[32];
	THREAD_ID FsPtid;
	long i, j, bmpw, bmph, file, res;
	
	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)	/*取得键盘鼠标服务线程*/
		return res;
	if ((file = FSopen(FsPtid, path, FS_OPEN_READ)) < 0)	/*读取BMP文件*/
		return file;
	if (FSread(FsPtid, file, &BmpHead[2], 30) < 30 || *((WORD*)&BmpHead[2]) != 0x4D42 || *((WORD*)&BmpHead[30]) != 24)	/*保证32位对齐访问*/
	{
		FSclose(FsPtid, file);
		return -1;
	}
	bmpw = *((long*)&BmpHead[20]);
	bmph = *((long*)&BmpHead[24]);
	if (bmpw * bmph > len)
	{
		FSclose(FsPtid, file);
		return -1;
	}
	FSseek(FsPtid, file, 54, FS_SEEK_SET);
	for (j = bmph - 1; j >= 0; j--)
	{
		FSread(FsPtid, file, buf + bmpw * j, bmpw * 3);
		for (i = bmpw - 1; i >= 0; i--)
		{
			DWORD s = bmpw * j * 4 + i * 3, d = bmpw * j * 4 + i * 4;
			((BYTE*)buf)[d + 3] = 0xFF;
			((BYTE*)buf)[d + 2] = ((BYTE*)buf)[s + 2];
			((BYTE*)buf)[d + 1] = ((BYTE*)buf)[s + 1];
			((BYTE*)buf)[d + 0] = ((BYTE*)buf)[s + 0];
		}
	}
	FSclose(FsPtid, file);
	if (width)
		*width = bmpw;
	if (height)
		*height = bmph;
	return NO_ERROR;
}

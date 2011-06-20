/*	guilib.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面功能库
	最后修改日期：2011-05-22
*/

#include "gui.h"
#include "../lib/gdi.h"
#include "../fs/fsapi.h"

typedef struct _RGB24
{
	BYTE blue, green, red;
}__attribute__((packed)) RGB24;	/*每像素24位模式专用结构*/

static inline DWORD DW2RGB15(DWORD c)
{
	return ((c >> 3) & 0x001F) | ((c >> 6) & 0x03E0) | ((c >> 9) & 0x7C00);
}

static inline DWORD DW2RGB16(DWORD c)
{
	return ((c >> 3) & 0x001F) | ((c >> 5) & 0x07E0) | ((c >> 8) & 0xF800);
}

static inline void Dw2Rgb15(WORD *dest, const DWORD *src, DWORD n)
{
	while (n--)
		*dest++ = DW2RGB15(*src++);
}

static inline void Dw2Rgb16(WORD *dest, const DWORD *src, DWORD n)
{
	while (n--)
		*dest++ = DW2RGB16(*src++);
}

static inline void Dw2Rgb24(RGB24 *dest, const DWORD *src, DWORD n)
{
	while (n--)
		*dest++ = *(RGB24*)(src++);
}

/*GUI矩形块贴图*/
void GuiPutImage(long x, long y, DWORD *img, long memw, long w, long h)
{
	switch (GDIPixBits)
	{
	case 15:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb15(tmpvm, img, w);
		}
		break;
	case 16:
		{
			WORD *tmpvm;
			for (tmpvm = (WORD*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb16(tmpvm, img, w);
		}
		break;
	case 24:
		{
			RGB24 *tmpvm;
			for (tmpvm = (RGB24*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				Dw2Rgb24(tmpvm, img, w);
		}
		break;
	case 32:
		{
			DWORD *tmpvm;
			for (tmpvm = (DWORD*)GDIvm + x + GDIwidth * y; h > 0; tmpvm += GDIwidth, img += memw, h--)
				memcpy32(tmpvm, img, w);
		}
		break;
	}
}

/*加载BMP图像文件*/
long LoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height)
{
	BYTE BmpHead[32];
	THREAD_ID FsPtid;
	long bmpw, bmph, file, res;

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
	len = ((DWORD)bmpw * 3 + 3) & 0xFFFFFFFC;
	for (res = bmph, buf += bmpw * (res - 1); res > 0; res--, buf -= bmpw)
	{
		BYTE *src, *dst;

		if (FSread(FsPtid, file, buf, len) < len)
		{
			FSclose(FsPtid, file);
			return -1;
		}
		src = (BYTE*)buf + bmpw * 3;
		dst = (BYTE*)buf + bmpw * 4;
		while (src > (BYTE*)buf)
		{
			*(--dst) = 0xFF;
			*(--dst) = *(--src);
			*(--dst) = *(--src);
			*(--dst) = *(--src);
		}
	}
	FSclose(FsPtid, file);
	if (width)
		*width = bmpw;
	if (height)
		*height = bmph;
	return NO_ERROR;
}

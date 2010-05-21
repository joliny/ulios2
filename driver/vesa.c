/*	vesa.c for ulios driver
	作者：孙亮
	功能：VESA 2.0显卡驱动服务程序
	最后修改日期：2010-05-19
*/

#include "basesrv.h"
#include "../fs/fsapi.h"

#define FONT_SIZE	199344
#define HZ_SIZE		196272
#define abs(v) ((v) >= 0 ? (v) : -(v))

/*画点*/
static inline long PutPixel(DWORD *vm, DWORD width, DWORD height, DWORD x, DWORD y, DWORD c)
{
	if (x >= width || y >= height)
		return VESA_ERR_LOCATION;	/*位置越界*/
	vm[x + y * width] = c;
	return NO_ERROR;
}

/*取点*/
static inline long GetPixel(DWORD *vm, DWORD width, DWORD height, DWORD x, DWORD y, DWORD *c)
{
	if (x >= width || y >= height)
		return VESA_ERR_LOCATION;	/*位置越界*/
	*c = vm[x + y * width];
	return NO_ERROR;
}

/*贴图*/
static inline long PutImage(DWORD *vm, long width, long height, long x, long y, DWORD *img, long w, long h)
{
	long tx, ty, tw, th;	/*实际位置尺寸*/

	if (x >= width || y >= height)
		return VESA_ERR_LOCATION;	/*位置越界*/
	if (w <= 0 || h <= 0)
		return VESA_ERR_SIZE;	/*非法尺寸*/
	tx = (x > 0 ? x : 0);	/*位置越界处理*/
	ty = (y > 0 ? y : 0);
	if ((tw = (x + w > width ? width : x + w) - tx) <= 0)	/*尺寸越界处理*/
		return VESA_ERR_SIZE;
	if ((th = (y + h > height ? height : y + h) - ty) <= 0)
		return VESA_ERR_SIZE;
	x = tx - x;
	img += x;
	vm += tx;
	for (h = th - 1; h >= 0; h--)
		memcpy32(vm + (ty + h) * width, img + (ty + h - y) * w, tw);
	return NO_ERROR;
}

/*截图*/
static inline long GetImage(DWORD *vm, long width, long height, long x, long y, DWORD *img, long w, long h)
{
	long tx, ty, tw, th;	/*实际位置尺寸*/

	if (x >= width || y >= height)
		return VESA_ERR_LOCATION;	/*位置越界*/
	if (w <= 0 || h <= 0)
		return VESA_ERR_SIZE;	/*非法尺寸*/
	tx = (x > 0 ? x : 0);	/*位置越界处理*/
	ty = (y > 0 ? y : 0);
	if ((tw = (x + w > width ? width : x + w) - tx) <= 0)	/*尺寸越界处理*/
		return VESA_ERR_SIZE;
	if ((th = (y + h > height ? height : y + h) - ty) <= 0)
		return VESA_ERR_SIZE;
	x = tx - x;
	img += x;
	vm += tx;
	for (h = th - 1; h >= 0; h--)
		memcpy32(img + (ty + h - y) * w, vm + (ty + h) * width, tw);
	return NO_ERROR;
}

/*填充矩形*/
static inline long FillRect(DWORD *vm, long width, long height, long x, long y, long w, long h, DWORD c)
{
	long tx, ty, tw, th;	/*实际位置尺寸*/
	
	if (x >= width || y >= height)
		return VESA_ERR_LOCATION;	/*位置越界*/
	if (w <= 0 || h <= 0)
		return VESA_ERR_SIZE;	/*非法尺寸*/
	tx = (x > 0 ? x : 0);	/*位置越界处理*/
	ty = (y > 0 ? y : 0);
	if ((tw = (x + w > width ? width : x + w) - tx) <= 0)	/*尺寸越界处理*/
		return VESA_ERR_SIZE;
	if ((th = (y + h > height ? height : y + h) - ty) <= 0)
		return VESA_ERR_SIZE;
	vm += tx;
	for (h = th - 1; h >= 0; h--)
		memset32(vm + (ty + h) * width, c, tw);
	return NO_ERROR;
}

/*Bresenham改进算法画线*/
static inline long DrawLine(DWORD *vm, DWORD width, DWORD height, long x1, long y1, long x2, long y2, DWORD c)
{
	long dx, dy, dx2, dy2;
	long d, xinc, yinc, dxy, half;

	dx = abs(x2 - x1);
	dx2 = dx << 1;
	dy = abs(y2 - y1);
	dy2 = dy << 1;
	xinc = (x2 > x1) ? 1 : (x2 == x1 ? 0 : -1);
	yinc = (y2 > y1) ? 1 : (y2 == y1 ? 0 : -1);
	PutPixel(vm, width, height, x1, y1, c);
	PutPixel(vm, width, height, x2, y2, c);
	if (dx >= dy)
	{
		d = dy2 - dx;
		dxy = dy2 - dx2;
		half = (dx + 1) >> 1;
		while (half--)
		{
			if (d <= 0)
				d += dy2;
			else
			{
				d += dxy;
				y1 += yinc;
				y2 -= yinc;
			}
			PutPixel(vm, width, height, x1 += xinc, y1, c);
			PutPixel(vm, width, height, x2 -= xinc, y2, c);
		}
		if (x1 + xinc != x2)	/*需多写一个中间点*/
		{
			if (d >= 0)
				y1 += yinc;
			PutPixel(vm, width, height, x1 + xinc, y1, c);
		}
	}
	else
	{
		d = dx2 - dy;
		dxy = dx2 - dy2;
		half = (dy + 1) >> 1;
		while (half--)
		{
			if (d <= 0)
				d += dx2;
			else
			{
				d += dxy;
				x1 += xinc;
				x2 -= xinc;
			}
			PutPixel(vm, width, height, x1, y1 += yinc, c);
			PutPixel(vm, width, height, x2, y2 -= yinc, c);
		}
		if (y1 + yinc != y2)	/*需多写一个中间点*/
		{
			if (d >= 0)
				x1 += xinc;
			PutPixel(vm, width, height, x1, y1 + yinc, c);
		}
	}
	return NO_ERROR;
}

/*Bresenham算法画圆*/
static inline long circle(DWORD *vm, DWORD width, DWORD height, long cx, long cy, long r, DWORD c)
{
	long x, y, d;

	if (!r)
		return VESA_ERR_SIZE;
	y = r;
	d = (3 - y) << 1;
	for (x = 0; x <= y; x++)
	{
		PutPixel(vm, width, height, cx + x, cy + y, c);
		PutPixel(vm, width, height, cx + x, cy - y, c);
		PutPixel(vm, width, height, cx - x, cy + y, c);
		PutPixel(vm, width, height, cx - x, cy - y, c);
		PutPixel(vm, width, height, cx + y, cy + x, c);
		PutPixel(vm, width, height, cx + y, cy - x, c);
		PutPixel(vm, width, height, cx - y, cy + x, c);
		PutPixel(vm, width, height, cx - y, cy - x, c);
		if (d < 0)
			d += (x << 2) + 6;
		else
			d += ((x - y--) << 2) + 10;
	}
	return NO_ERROR;
}

/*显示汉字*/
static inline long DrawHz(DWORD *vm, DWORD width, DWORD height, long x, long y, DWORD hz, DWORD c, const BYTE *font)
{
	long i, j;
	WORD *p;

	if (x <= -12 || x >= width || y <= -12 || y >= height)
		return VESA_ERR_LOCATION;
	if ((p = (WORD*)(font + ((((hz & 0xFF) - 161) * 94 + ((hz >> 8) & 0xFF) - 161) * 24))) >= (WORD*)(font + HZ_SIZE))
		return NO_ERROR;
	for (j = 11; j >= 0; j--)
	{
		if ((DWORD)(y + j) >= height)
			continue;
		for (i = 7; i >= 0; i--)
		{
			if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
				vm[(x - i + 7) + (y + j) * width] = c;
			if ((DWORD)(x - i + 15) < width && ((p[j] >> (i + 8)) & 1ul))
				vm[(x - i + 15) + (y + j) * width] = c;
		}
	}
	return NO_ERROR;
}

/*显示ANSI字符*/
static inline long DrawAnsi(DWORD *vm, DWORD width, DWORD height, long x, long y, DWORD ch, DWORD c, const BYTE *font)
{
	long i, j;
	const BYTE *p;

	if (x <= -8 || x >= width || y <= -12 || y >= height)
		return VESA_ERR_LOCATION;
	p = font + (ch & 0xFF) * 12;
	for (j = 11; j >= 0; j--)
	{
		if ((DWORD)(y + j) >= height)
			continue;
		for (i = 7;i >= 0; i--)
		{
			if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
				vm[(x - i + 7) + (y + j) * width] = c;
		}
	}
	return NO_ERROR;
}

/*输出字符串*/
static inline long DrawStr(DWORD *vm, DWORD width, DWORD height, long x, long y, const BYTE *str, DWORD c, const BYTE *font)
{
	DWORD hzf;	/*汉字内码首字符标志*/

	for (hzf = 0; *str && x < width; str++)
	{
		if (*str > 160)
		{
			if (hzf)	/*显示汉字*/
			{
				DrawHz(vm, width, height, x, y, ((*str) << 8) | hzf, c, font);
				x += 12;
				hzf = 0;
			}
			else
				hzf = *str;
		}
		else
		{
			if (hzf)	/*有未显示的ASCII*/
			{
				DrawAnsi(vm, width, height, x, y, hzf, c, font + HZ_SIZE);
				x += 8;
				hzf = 0;
			}
			DrawAnsi(vm, width, height, x, y, *str, c, font + HZ_SIZE);	/*显示当前ASCII*/
			x += 8;
		}
	}
	return NO_ERROR;
}

int main()
{
	DWORD VmPhy;	/*显存物理地址,大小*/
	DWORD width, height;	/*屏幕大小*/
	void *addr;		/*显存映射地址*/
	THREAD_ID ptid;
	long res;	/*返回结果*/
	BYTE font[FONT_SIZE];	/*字体*/

	if ((res = KRegKnlPort(SRV_VESA_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	if ((res = KGetKpToThed(SRV_FS_PORT, &ptid)) != NO_ERROR)	/*取得文件系统线程ID*/
		return res;
	if ((res = FSopen(ptid, (const BYTE*)"/0/ulios/font.bin", 0)) < 0)	/*打开字体文件*/
		return res;
	if (FSread(ptid, res, font, FONT_SIZE) <= 0)	/*开始读取字体文件*/
		return -1;
	FSclose(ptid, res);
	if ((res = KMapPhyAddr(&addr, 0x90300, 0x100)) != NO_ERROR)	/*取得显卡信息*/
		return res;
	width = *((WORD*)(addr + 18));
	height = *((WORD*)(addr + 20));
	VmPhy = *((DWORD*)(addr + 40));
//	VmSiz = *((WORD*)(addr + 48)) * 0x400;
	KFreeAddr(addr);
	if ((res = KMapPhyAddr(&addr, VmPhy, 0x300000)) != NO_ERROR)	/*映射显存*/
		return res;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (data[0] == MSG_ATTR_USER)
		{
			switch (data[3])
			{
			case VESA_API_PUTPIXEL:
				PutPixel(addr, width, height, data[1], data[2], data[4]);
				break;
			case VESA_API_GETPIXEL:
				data[0] = MSG_ATTR_USER;
				data[1] = GetPixel(addr, width, height, data[1], data[2], &data[2]);
				KSendMsg(ptid, data, 0);
				break;
			case VESA_API_FILLRECT:
				FillRect(addr, width, height, data[1], data[2], data[4], data[5], data[6]);
				break;
			case VESA_API_DRAWLINE:
				DrawLine(addr, width, height, data[1], data[2], data[4], data[5], data[6]);
				break;				
			case VESA_API_CIRCLE:
				circle(addr, width, height, data[1], data[2], data[4], data[5]);
				break;
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_MAP)
		{
			switch (data[3])
			{
			case VESA_API_PUTIMAGE:
				data[0] = PutImage(addr, width, height, data[4], data[5], (DWORD*)data[2], data[6], data[7]);
				KUnmapProcAddr((void*)data[2], data);
				break;
			case VESA_API_GETIMAGE:
				data[0] = GetImage(addr, width, height, data[4], data[5], (DWORD*)data[2], data[6], data[7]);
				KUnmapProcAddr((void*)data[2], data);
				break;
			case VESA_API_DRAWSTR:
				data[0] = DrawStr(addr, width, height, data[4], data[5], (const BYTE*)data[2], data[6], font);
				KUnmapProcAddr((void*)data[2], data);
				break;
			}
		}
	}
	KFreeAddr(addr);
	KUnregKnlPort(SRV_VESA_PORT);
	return NO_ERROR;
}
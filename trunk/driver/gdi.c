/*	gdi.c for ulios driver
	作者：孙亮
	功能：图形设备接口库
	最后修改日期：2010-06-10
*/

#include "basesrv.h"

void *vm;
const BYTE *font;
DWORD GDIwidth, GDIheight, GDIPixBits;
DWORD GDICharWidth, GDICharHeight;
THREAD_ID GDIVesaPtid;

/*初始化GDI库*/
long GDIinit()
{
	long res;

	if ((res = KGetKptThed(SRV_VESA_PORT, &GDIVesaPtid)) != NO_ERROR)
		return res;
	if (vm == NULL && (res = VSGetVmem(GDIVesaPtid, &vm, &GDIwidth, &GDIheight, &GDIPixBits)) != NO_ERROR)
		return res;
	if (font == NULL && (res = VSGetFont(GDIVesaPtid, &font, &GDICharWidth, &GDICharHeight)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*撤销GDI库*/
void GDIrelease()
{
	DWORD data[MSG_DATA_LEN - 2];

	if (vm)
	{
		KUnmapProcAddr(vm, data);
		vm = NULL;
	}
	if (font)
	{
		KUnmapProcAddr((void*)font, data);
		font = NULL;
	}
}

typedef struct _RGB24
{
	BYTE blue, green, red;
}RGB24;	/*每像素24位模式专用结构*/

#define HZ_SIZE		196272

static inline DWORD DW2RGB15(DWORD c)
{
	return ((c >> 3) & 0x001F) | ((c >> 6) & 0x03E0) | ((c >> 9) & 0x7C00);
}

static inline DWORD RGB152DW(DWORD c)
{
	return ((c << 3) & 0x0000F8) | ((c << 6) & 0x00F800) | ((c << 9) & 0xF80000);
}

static inline DWORD DW2RGB16(DWORD c)
{
	return ((c >> 3) & 0x001F) | ((c >> 5) & 0x07E0) | ((c >> 8) & 0xF800);
}

static inline DWORD RGB162DW(DWORD c)
{
	return ((c << 3) & 0x0000F8) | ((c << 5) & 0x00FC00) | ((c << 8) & 0xF80000);
}

/*画点*/
long GDIPutPixel(DWORD x, DWORD y, DWORD c)
{
	if (x >= GDIwidth || y >= GDIheight)
		return VESA_ERR_LOCATION;	/*位置越界*/
	switch (GDIPixBits)
	{
	case 15:
		((WORD*)vm)[x + y * GDIwidth] = DW2RGB15(c);
		break;
	case 16:
		((WORD*)vm)[x + y * GDIwidth] = DW2RGB16(c);
		break;
	case 24:
		((RGB24*)vm)[x + y * GDIwidth] = *(RGB24*)&c;
		break;
	case 32:
		((DWORD*)vm)[x + y * GDIwidth] = c;
		break;
	}
	return NO_ERROR;
}

/*取点*/
long GDIGetPixel(DWORD x, DWORD y, DWORD *c)
{
	if (x >= GDIwidth || y >= GDIheight)
		return VESA_ERR_LOCATION;	/*位置越界*/
	switch (GDIPixBits)
	{
	case 15:
		*c = RGB152DW(((WORD*)vm)[x + y * GDIwidth]);
		break;
	case 16:
		*c = RGB162DW(((WORD*)vm)[x + y * GDIwidth]);
		break;
	case 24:
		*(RGB24*)c = ((RGB24*)vm)[x + y * GDIwidth];
		break;
	case 32:
		*c = ((DWORD*)vm)[x + y * GDIwidth];
		break;
	}
	return NO_ERROR;
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

/*贴图*/
long GDIPutImage(long x, long y, DWORD *img, long w, long h)
{
	void *tmpvm;
	long tx, ty, tw, th;	/*实际位置尺寸*/

	if (x >= GDIwidth || y >= GDIheight)
		return VESA_ERR_LOCATION;	/*位置越界*/
	if (w <= 0 || h <= 0)
		return VESA_ERR_SIZE;	/*非法尺寸*/
	tx = (x > 0 ? x : 0);	/*位置越界处理*/
	ty = (y > 0 ? y : 0);
	if ((tw = (x + w > GDIwidth ? GDIwidth : x + w) - tx) <= 0)	/*尺寸越界处理*/
		return VESA_ERR_SIZE;
	if ((th = (y + h > GDIheight ? GDIheight : y + h) - ty) <= 0)
		return VESA_ERR_SIZE;
	x = tx - x;
	img += x;
	switch (GDIPixBits)
	{
	case 15:
		tmpvm = vm + tx * 2;
		for (h = th - 1; h >= 0; h--)
			Dw2Rgb15((WORD*)tmpvm + (ty + h) * GDIwidth, img + (ty + h - y) * w, tw);
		break;
	case 16:
		tmpvm = vm + tx * 2;
		for (h = th - 1; h >= 0; h--)
			Dw2Rgb16((WORD*)tmpvm + (ty + h) * GDIwidth, img + (ty + h - y) * w, tw);
		break;
	case 24:
		tmpvm = vm + tx * 3;
		for (h = th - 1; h >= 0; h--)
			Dw2Rgb24((RGB24*)tmpvm + (ty + h) * GDIwidth, img + (ty + h - y) * w, tw);
		break;
	case 32:
		tmpvm = vm + tx * 4;
		for (h = th - 1; h >= 0; h--)
			memcpy32((DWORD*)tmpvm + (ty + h) * GDIwidth, img + (ty + h - y) * w, tw);
		break;
	}
	return NO_ERROR;
}

static inline void Rgb152Dw(DWORD *dest, const WORD *src, DWORD n)
{
	while (n--)
		*dest++ = RGB152DW(*src++);
}

static inline void Rgb162Dw(DWORD *dest, const WORD *src, DWORD n)
{
	while (n--)
		*dest++ = RGB162DW(*src++);
}

static inline void Rgb242Dw(DWORD *dest, const RGB24 *src, DWORD n)
{
	while (n--)
		*dest++ = *(DWORD*)(src++);
}

/*截图*/
long GDIGetImage(long x, long y, DWORD *img, long w, long h)
{
	void *tmpvm;
	long tx, ty, tw, th;	/*实际位置尺寸*/

	if (x >= GDIwidth || y >= GDIheight)
		return VESA_ERR_LOCATION;	/*位置越界*/
	if (w <= 0 || h <= 0)
		return VESA_ERR_SIZE;	/*非法尺寸*/
	tx = (x > 0 ? x : 0);	/*位置越界处理*/
	ty = (y > 0 ? y : 0);
	if ((tw = (x + w > GDIwidth ? GDIwidth : x + w) - tx) <= 0)	/*尺寸越界处理*/
		return VESA_ERR_SIZE;
	if ((th = (y + h > GDIheight ? GDIheight : y + h) - ty) <= 0)
		return VESA_ERR_SIZE;
	x = tx - x;
	img += x;
	switch (GDIPixBits)
	{
	case 15:
		tmpvm = vm + tx * 2;
		for (h = th - 1; h >= 0; h--)
			Rgb152Dw(img + (ty + h - y) * w, (WORD*)tmpvm + (ty + h) * GDIwidth, tw);
		break;
	case 16:
		tmpvm = vm + tx * 2;
		for (h = th - 1; h >= 0; h--)
			Rgb162Dw(img + (ty + h - y) * w, (WORD*)tmpvm + (ty + h) * GDIwidth, tw);
		break;
	case 24:
		tmpvm = vm + tx * 3;
		for (h = th - 1; h >= 0; h--)
			Rgb242Dw(img + (ty + h - y) * w, (RGB24*)tmpvm + (ty + h) * GDIwidth, tw);
		break;
	case 32:
		tmpvm = vm + tx * 4;
		for (h = th - 1; h >= 0; h--)
			memcpy32(img + (ty + h - y) * w, (DWORD*)tmpvm + (ty + h) * GDIwidth, tw);
		break;
	}
	return NO_ERROR;
}

static inline void SetRgb15(WORD *dest, DWORD d, DWORD n)
{
	while (n--)
		*dest++ = DW2RGB15(d);
}

static inline void SetRgb16(WORD *dest, DWORD d, DWORD n)
{
	while (n--)
		*dest++ = DW2RGB16(d);
}

static inline void SetRgb24(RGB24 *dest, DWORD d, DWORD n)
{
	while (n--)
		*dest++ = *(RGB24*)&d;
}

/*填充矩形*/
long GDIFillRect(long x, long y, long w, long h, DWORD c)
{
	void *tmpvm;
	long tx, ty, tw, th;	/*实际位置尺寸*/

	if (x >= GDIwidth || y >= GDIheight)
		return VESA_ERR_LOCATION;	/*位置越界*/
	if (w <= 0 || h <= 0)
		return VESA_ERR_SIZE;	/*非法尺寸*/
	tx = (x > 0 ? x : 0);	/*位置越界处理*/
	ty = (y > 0 ? y : 0);
	if ((tw = (x + w > GDIwidth ? GDIwidth : x + w) - tx) <= 0)	/*尺寸越界处理*/
		return VESA_ERR_SIZE;
	if ((th = (y + h > GDIheight ? GDIheight : y + h) - ty) <= 0)
		return VESA_ERR_SIZE;
	switch (GDIPixBits)
	{
	case 15:
		tmpvm = vm + tx * 2;
		for (h = th - 1; h >= 0; h--)
			SetRgb15((WORD*)tmpvm + (ty + h) * GDIwidth, c, tw);
		break;
	case 16:
		tmpvm = vm + tx * 2;
		for (h = th - 1; h >= 0; h--)
			SetRgb16((WORD*)tmpvm + (ty + h) * GDIwidth, c, tw);
		break;
	case 24:
		tmpvm = vm + tx * 3;
		for (h = th - 1; h >= 0; h--)
			SetRgb24((RGB24*)tmpvm + (ty + h) * GDIwidth, c, tw);
		break;
	case 32:
		tmpvm = vm + tx * 4;
		for (h = th - 1; h >= 0; h--)
			memset32((DWORD*)tmpvm + (ty + h) * GDIwidth, c, tw);
		break;
	}
	return NO_ERROR;
}

/*向上滚屏*/
long GDIMoveUp(DWORD pix)
{
	if (pix >= GDIheight)
		return VESA_ERR_LOCATION;
	memcpy32(vm, vm + ((GDIPixBits + 7) / 8) * GDIwidth * pix, ((GDIPixBits + 7) / 8) * GDIwidth * (GDIheight - pix) / sizeof(DWORD));
	return NO_ERROR;
}

#define abs(v) ((v) >= 0 ? (v) : -(v))

/*无越界判断画点*/
static inline void NCPutPixel(DWORD x, DWORD y, DWORD c)
{
	switch (GDIPixBits)
	{
	case 15:
		((WORD*)vm)[x + y * GDIwidth] = DW2RGB15(c);
		break;
	case 16:
		((WORD*)vm)[x + y * GDIwidth] = DW2RGB16(c);
		break;
	case 24:
		((RGB24*)vm)[x + y * GDIwidth] = *(RGB24*)&c;
		break;
	case 32:
		((DWORD*)vm)[x + y * GDIwidth] = c;
		break;
	}
}

#define CS_LEFT		1
#define CS_RIGHT	2
#define CS_TOP		4
#define CS_BOTTOM	8

/*Cohen-Sutherland裁剪算法编码*/
static inline DWORD CsEncode(long x, long y)
{
	DWORD mask;

	mask = 0;
	if (x < 0)
		mask |= CS_LEFT;
	else if (x >= GDIwidth)
		mask |= CS_RIGHT;
	if (y < 0)
		mask |= CS_TOP;
	else if (y >= GDIheight)
		mask |= CS_BOTTOM;
	return mask;
}

/*Cohen-Sutherland算法裁剪Bresenham改进算法画线*/
long GDIDrawLine(long x1, long y1, long x2, long y2, DWORD c)
{
	DWORD mask, mask1, mask2;
	long dx, dy, dx2, dy2;
	long e, xinc, yinc, half;

	/*裁剪*/
	mask1 = CsEncode(x1, y1);
	mask2 = CsEncode(x2, y2);
	if (mask1 || mask2)
	{
		float ydivx, xdivy;

		xdivy = ydivx = 0.f;
		if (x1 != x2)
			ydivx = (float)(y2 - y1) / (float)(x2 - x1);
		if (y1 != y2)
			xdivy = (float)(x2 - x1) / (float)(y2 - y1);
		do
		{
			if (mask1 & mask2)	/*在裁剪区一侧,完全裁剪掉*/
				return NO_ERROR;

			dy = dx = 0;
			mask = mask1 ? mask1 : mask2;
			if (mask & CS_LEFT)
			{
				dx = 0;
				dy = y1 - x1 * ydivx;
			}
			else if (mask & CS_RIGHT)
			{
				dx = GDIwidth - 1;
				dy = y1 - (x1 + 1 - GDIwidth) * ydivx;
			}
			if (mask & CS_TOP)
			{
				dy = 0;
				dx = x1 - y1 * xdivy;
			}
			else if (mask & CS_BOTTOM)
			{
				dy = GDIheight - 1;
				dx = x1 - (y1 + 1 - GDIheight) * xdivy;
			}
			if (mask == mask1)
			{
				x1 = dx;
				y1 = dy;
				mask1 = CsEncode(dx, dy);
			}
			else
			{
				x2 = dx;
				y2 = dy;
				mask2 = CsEncode(dx, dy);
			}
		}
		while (mask1 || mask2);
	}

	/*画线*/
	dx = abs(x2 - x1);
	dx2 = dx << 1;
	xinc = (x2 > x1) ? 1 : (x2 < x1 ? -1 : 0);
	dy = abs(y2 - y1);
	dy2 = dy << 1;
	yinc = (y2 > y1) ? 1 : (y2 < y1 ? -1 : 0);
	if (dx >= dy)
	{
		e = dy2 - dx;
		half = (dx + 1) >> 1;
		while (half--)
		{
			NCPutPixel(x1, y1, c);
			NCPutPixel(x2, y2, c);
			if (e > 0)
			{
				e -= dx2;
				y1 += yinc;
				y2 -= yinc;
			}
			e += dy2;
			x1 += xinc;
			x2 -= xinc;
		}
		if (x1 == x2)	/*需多写一个中间点*/
			NCPutPixel(x1, y1, c);
	}
	else
	{
		e = dx2 - dy;
		half = (dy + 1) >> 1;
		while (half--)
		{
			NCPutPixel(x1, y1, c);
			NCPutPixel(x2, y2, c);
			if (e > 0)
			{
				e -= dy2;
				x1 += xinc;
				x2 -= xinc;
			}
			e += dx2;
			y1 += yinc;
			y2 -= yinc;
		}
		if (y1 == y2)	/*需多写一个中间点*/
			NCPutPixel(x1, y1, c);
	}
	return NO_ERROR;
}

/*Bresenham算法画圆*/
long GDIcircle(long cx, long cy, long r, DWORD c)
{
	long x, y, d;

	if (!r)
		return VESA_ERR_SIZE;
	y = r;
	d = (3 - y) << 1;
	for (x = 0; x <= y; x++)
	{
		GDIPutPixel(cx + x, cy + y, c);
		GDIPutPixel(cx + x, cy - y, c);
		GDIPutPixel(cx - x, cy + y, c);
		GDIPutPixel(cx - x, cy - y, c);
		GDIPutPixel(cx + y, cy + x, c);
		GDIPutPixel(cx + y, cy - x, c);
		GDIPutPixel(cx - y, cy + x, c);
		GDIPutPixel(cx - y, cy - x, c);
		if (d < 0)
			d += (x << 2) + 6;
		else
			d += ((x - y--) << 2) + 10;
	}
	return NO_ERROR;
}

/*显示汉字*/
long GDIDrawHz(long x, long y, DWORD hz, DWORD c)
{
	long i, j;
	WORD *p;

	if (x <= -12 || x >= GDIwidth || y <= -12 || y >= GDIheight)
		return VESA_ERR_LOCATION;
	if ((p = (WORD*)(font + ((((hz & 0xFF) - 161) * 94 + ((hz >> 8) & 0xFF) - 161) * 24))) >= (WORD*)(font + HZ_SIZE))
		return NO_ERROR;
	switch (GDIPixBits)
	{
	case 15:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < GDIwidth && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 7) + (y + j) * GDIwidth] = DW2RGB15(c);
				if ((DWORD)(x - i + 15) < GDIwidth && ((p[j] >> (i + 8)) & 1ul))
					((WORD*)vm)[(x - i + 15) + (y + j) * GDIwidth] = DW2RGB15(c);
			}
		}
		break;
	case 16:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < GDIwidth && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 7) + (y + j) * GDIwidth] = DW2RGB16(c);
				if ((DWORD)(x - i + 15) < GDIwidth && ((p[j] >> (i + 8)) & 1ul))
					((WORD*)vm)[(x - i + 15) + (y + j) * GDIwidth] = DW2RGB16(c);
			}
		}
		break;
	case 24:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < GDIwidth && ((p[j] >> i) & 1ul))
					((RGB24*)vm)[(x - i + 7) + (y + j) * GDIwidth] = *(RGB24*)&c;
				if ((DWORD)(x - i + 15) < GDIwidth && ((p[j] >> (i + 8)) & 1ul))
					((RGB24*)vm)[(x - i + 15) + (y + j) * GDIwidth] = *(RGB24*)&c;
			}
		}
		break;
	case 32:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < GDIwidth && ((p[j] >> i) & 1ul))
					((DWORD*)vm)[(x - i + 7) + (y + j) * GDIwidth] = c;
				if ((DWORD)(x - i + 15) < GDIwidth && ((p[j] >> (i + 8)) & 1ul))
					((DWORD*)vm)[(x - i + 15) + (y + j) * GDIwidth] = c;
			}
		}
		break;
	}
	return NO_ERROR;
}

/*显示ASCII字符*/
long GDIDrawAscii(long x, long y, DWORD ch, DWORD c)
{
	long i, j;
	const BYTE *p;

	if (x <= -6 || x >= GDIwidth || y <= -12 || y >= GDIheight)
		return VESA_ERR_LOCATION;
	p = font + HZ_SIZE + (ch & 0xFF) * 12;
	switch (GDIPixBits)
	{
	case 15:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 5; i >= 0; i--)
			{
				if ((DWORD)(x - i + 5) < GDIwidth && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 5) + (y + j) * GDIwidth] = DW2RGB15(c);
			}
		}
		break;
	case 16:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 5; i >= 0; i--)
			{
				if ((DWORD)(x - i + 5) < GDIwidth && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 5) + (y + j) * GDIwidth] = DW2RGB16(c);
			}
		}
		break;
	case 24:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 5; i >= 0; i--)
			{
				if ((DWORD)(x - i + 5) < GDIwidth && ((p[j] >> i) & 1ul))
					((RGB24*)vm)[(x - i + 5) + (y + j) * GDIwidth] = *(RGB24*)&c;
			}
		}
		break;
	case 32:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= GDIheight)
				continue;
			for (i = 5; i >= 0; i--)
			{
				if ((DWORD)(x - i + 5) < GDIwidth && ((p[j] >> i) & 1ul))
					((DWORD*)vm)[(x - i + 5) + (y + j) * GDIwidth] = c;
			}
		}
		break;
	}
	return NO_ERROR;
}

/*输出字符串*/
long GDIDrawStr(long x, long y, const char *str, DWORD c)
{
	DWORD hzf;	/*汉字内码首字符标志*/

	for (hzf = 0; *str && x < GDIwidth; str++)
	{
		if ((BYTE)(*str) > 160)
		{
			if (hzf)	/*显示汉字*/
			{
				GDIDrawHz(x, y, ((BYTE)(*str) << 8) | hzf, c);
				x += 12;
				hzf = 0;
			}
			else
				hzf = (BYTE)(*str);
		}
		else
		{
			if (hzf)	/*有未显示的ASCII*/
			{
				GDIDrawAscii(x, y, hzf, c);
				x += 6;
				hzf = 0;
			}
			GDIDrawAscii(x, y, (BYTE)(*str), c);	/*显示当前ASCII*/
			x += 6;
		}
	}
	return NO_ERROR;
}

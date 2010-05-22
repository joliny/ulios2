/*	vesa.c for ulios driver
	作者：孙亮
	功能：VESA 2.0显卡驱动服务程序
	最后修改日期：2010-05-19
*/

#include "basesrv.h"
#include "../fs/fsapi.h"

typedef struct _RGB24
{
	BYTE blue, green, red;
}RGB24;	/*每像素24位模式专用结构*/

#define FONT_SIZE	199344
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
static inline long PutPixel(void *vm, DWORD PixBits, DWORD width, DWORD height, DWORD x, DWORD y, DWORD c)
{
	if (x >= width || y >= height)
		return VESA_ERR_LOCATION;	/*位置越界*/
	switch (PixBits)
	{
	case 15:
		((WORD*)vm)[x + y * width] = DW2RGB15(c);
		break;
	case 16:
		((WORD*)vm)[x + y * width] = DW2RGB16(c);
		break;
	case 24:
		((RGB24*)vm)[x + y * width] = *(RGB24*)&c;
		break;
	case 32:
		((DWORD*)vm)[x + y * width] = c;
		break;
	}
	return NO_ERROR;
}

/*取点*/
static inline long GetPixel(void *vm, DWORD PixBits, DWORD width, DWORD height, DWORD x, DWORD y, DWORD *c)
{
	if (x >= width || y >= height)
		return VESA_ERR_LOCATION;	/*位置越界*/
	switch (PixBits)
	{
	case 15:
		*c = RGB152DW(((WORD*)vm)[x + y * width]);
		break;
	case 16:
		*c = RGB162DW(((WORD*)vm)[x + y * width]);
		break;
	case 24:
		*(RGB24*)c = ((RGB24*)vm)[x + y * width];
		break;
	case 32:
		*c = ((DWORD*)vm)[x + y * width];
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
static inline long PutImage(void *vm, DWORD PixBits, long width, long height, long x, long y, DWORD *img, long w, long h)
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
	switch (PixBits)
	{
	case 15:
		vm += tx * 2;
		for (h = th - 1; h >= 0; h--)
			Dw2Rgb15((WORD*)vm + (ty + h) * width, img + (ty + h - y) * w, tw);
		break;
	case 16:
		vm += tx * 2;
		for (h = th - 1; h >= 0; h--)
			Dw2Rgb16((WORD*)vm + (ty + h) * width, img + (ty + h - y) * w, tw);
		break;
	case 24:
		vm += tx * 3;
		for (h = th - 1; h >= 0; h--)
			Dw2Rgb24((RGB24*)vm + (ty + h) * width, img + (ty + h - y) * w, tw);
		break;
	case 32:
		vm += tx * 4;
		for (h = th - 1; h >= 0; h--)
			memcpy32((DWORD*)vm + (ty + h) * width, img + (ty + h - y) * w, tw);
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
static inline long GetImage(void *vm, DWORD PixBits, long width, long height, long x, long y, DWORD *img, long w, long h)
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
	switch (PixBits)
	{
	case 15:
		vm += tx * 2;
		for (h = th - 1; h >= 0; h--)
			Rgb152Dw(img + (ty + h - y) * w, (WORD*)vm + (ty + h) * width, tw);
		break;
	case 16:
		vm += tx * 2;
		for (h = th - 1; h >= 0; h--)
			Rgb162Dw(img + (ty + h - y) * w, (WORD*)vm + (ty + h) * width, tw);
		break;
	case 24:
		vm += tx * 3;
		for (h = th - 1; h >= 0; h--)
			Rgb242Dw(img + (ty + h - y) * w, (RGB24*)vm + (ty + h) * width, tw);
		break;
	case 32:
		vm += tx * 4;
		for (h = th - 1; h >= 0; h--)
			memcpy32(img + (ty + h - y) * w, (DWORD*)vm + (ty + h) * width, tw);
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
static inline long FillRect(void *vm, DWORD PixBits, long width, long height, long x, long y, long w, long h, DWORD c)
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
	switch (PixBits)
	{
	case 15:
		vm += tx * 2;
		for (h = th - 1; h >= 0; h--)
			SetRgb15((WORD*)vm + (ty + h) * width, c, tw);
		break;
	case 16:
		vm += tx * 2;
		for (h = th - 1; h >= 0; h--)
			SetRgb16((WORD*)vm + (ty + h) * width, c, tw);
		break;
	case 24:
		vm += tx * 3;
		for (h = th - 1; h >= 0; h--)
			SetRgb24((RGB24*)vm + (ty + h) * width, c, tw);
		break;
	case 32:
		vm += tx * 4;
		for (h = th - 1; h >= 0; h--)
			memset32((DWORD*)vm + (ty + h) * width, c, tw);
		break;
	}
	return NO_ERROR;
}

#define abs(v) ((v) >= 0 ? (v) : -(v))

/*Bresenham改进算法画线*/
static inline long DrawLine(void *vm, DWORD PixBits, DWORD width, DWORD height, long x1, long y1, long x2, long y2, DWORD c)
{
	long dx, dy, dx2, dy2;
	long d, xinc, yinc, dxy, half;

	dx = abs(x2 - x1);
	dx2 = dx << 1;
	dy = abs(y2 - y1);
	dy2 = dy << 1;
	xinc = (x2 > x1) ? 1 : (x2 == x1 ? 0 : -1);
	yinc = (y2 > y1) ? 1 : (y2 == y1 ? 0 : -1);
	PutPixel(vm, PixBits, width, height, x1, y1, c);
	PutPixel(vm, PixBits, width, height, x2, y2, c);
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
			PutPixel(vm, PixBits, width, height, x1 += xinc, y1, c);
			PutPixel(vm, PixBits, width, height, x2 -= xinc, y2, c);
		}
		if (x1 + xinc != x2)	/*需多写一个中间点*/
		{
			if (d >= 0)
				y1 += yinc;
			PutPixel(vm, PixBits, width, height, x1 + xinc, y1, c);
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
			PutPixel(vm, PixBits, width, height, x1, y1 += yinc, c);
			PutPixel(vm, PixBits, width, height, x2, y2 -= yinc, c);
		}
		if (y1 + yinc != y2)	/*需多写一个中间点*/
		{
			if (d >= 0)
				x1 += xinc;
			PutPixel(vm, PixBits, width, height, x1, y1 + yinc, c);
		}
	}
	return NO_ERROR;
}

/*Bresenham算法画圆*/
static inline long circle(void *vm, DWORD PixBits, DWORD width, DWORD height, long cx, long cy, long r, DWORD c)
{
	long x, y, d;

	if (!r)
		return VESA_ERR_SIZE;
	y = r;
	d = (3 - y) << 1;
	for (x = 0; x <= y; x++)
	{
		PutPixel(vm, PixBits, width, height, cx + x, cy + y, c);
		PutPixel(vm, PixBits, width, height, cx + x, cy - y, c);
		PutPixel(vm, PixBits, width, height, cx - x, cy + y, c);
		PutPixel(vm, PixBits, width, height, cx - x, cy - y, c);
		PutPixel(vm, PixBits, width, height, cx + y, cy + x, c);
		PutPixel(vm, PixBits, width, height, cx + y, cy - x, c);
		PutPixel(vm, PixBits, width, height, cx - y, cy + x, c);
		PutPixel(vm, PixBits, width, height, cx - y, cy - x, c);
		if (d < 0)
			d += (x << 2) + 6;
		else
			d += ((x - y--) << 2) + 10;
	}
	return NO_ERROR;
}

/*显示汉字*/
static inline long DrawHz(void *vm, DWORD PixBits, DWORD width, DWORD height, long x, long y, DWORD hz, DWORD c, const BYTE *font)
{
	long i, j;
	WORD *p;

	if (x <= -12 || x >= width || y <= -12 || y >= height)
		return VESA_ERR_LOCATION;
	if ((p = (WORD*)(font + ((((hz & 0xFF) - 161) * 94 + ((hz >> 8) & 0xFF) - 161) * 24))) >= (WORD*)(font + HZ_SIZE))
		return NO_ERROR;
	switch (PixBits)
	{
	case 15:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 7) + (y + j) * width] = DW2RGB15(c);
				if ((DWORD)(x - i + 15) < width && ((p[j] >> (i + 8)) & 1ul))
					((WORD*)vm)[(x - i + 15) + (y + j) * width] = DW2RGB15(c);
			}
		}
		break;
	case 16:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 7) + (y + j) * width] = DW2RGB16(c);
				if ((DWORD)(x - i + 15) < width && ((p[j] >> (i + 8)) & 1ul))
					((WORD*)vm)[(x - i + 15) + (y + j) * width] = DW2RGB16(c);
			}
		}
		break;
	case 24:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((RGB24*)vm)[(x - i + 7) + (y + j) * width] = *(RGB24*)&c;
				if ((DWORD)(x - i + 15) < width && ((p[j] >> (i + 8)) & 1ul))
					((RGB24*)vm)[(x - i + 15) + (y + j) * width] = *(RGB24*)&c;
			}
		}
		break;
	case 32:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((DWORD*)vm)[(x - i + 7) + (y + j) * width] = c;
				if ((DWORD)(x - i + 15) < width && ((p[j] >> (i + 8)) & 1ul))
					((DWORD*)vm)[(x - i + 15) + (y + j) * width] = c;
			}
		}
		break;
	}
	return NO_ERROR;
}

/*显示ANSI字符*/
static inline long DrawAnsi(void *vm, DWORD PixBits, DWORD width, DWORD height, long x, long y, DWORD ch, DWORD c, const BYTE *font)
{
	long i, j;
	const BYTE *p;

	if (x <= -8 || x >= width || y <= -12 || y >= height)
		return VESA_ERR_LOCATION;
	p = font + (ch & 0xFF) * 12;
	switch (PixBits)
	{
	case 15:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 7) + (y + j) * width] = DW2RGB15(c);
			}
		}
		break;
	case 16:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((WORD*)vm)[(x - i + 7) + (y + j) * width] = DW2RGB16(c);
			}
		}
		break;
	case 24:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((RGB24*)vm)[(x - i + 7) + (y + j) * width] = *(RGB24*)&c;
			}
		}
		break;
	case 32:
		for (j = 11; j >= 0; j--)
		{
			if ((DWORD)(y + j) >= height)
				continue;
			for (i = 7; i >= 0; i--)
			{
				if ((DWORD)(x - i + 7) < width && ((p[j] >> i) & 1ul))
					((DWORD*)vm)[(x - i + 7) + (y + j) * width] = c;
			}
		}
		break;
	}
	return NO_ERROR;
}

/*输出字符串*/
static inline long DrawStr(void *vm, DWORD PixBits, DWORD width, DWORD height, long x, long y, const BYTE *str, DWORD c, const BYTE *font)
{
	DWORD hzf;	/*汉字内码首字符标志*/

	for (hzf = 0; *str && x < width; str++)
	{
		if (*str > 160)
		{
			if (hzf)	/*显示汉字*/
			{
				DrawHz(vm, PixBits, width, height, x, y, ((*str) << 8) | hzf, c, font);
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
				DrawAnsi(vm, PixBits, width, height, x, y, hzf, c, font + HZ_SIZE);
				x += 8;
				hzf = 0;
			}
			DrawAnsi(vm, PixBits, width, height, x, y, *str, c, font + HZ_SIZE);	/*显示当前ASCII*/
			x += 8;
		}
	}
	return NO_ERROR;
}

#define FAR2LINE(addr)	((WORD)(addr) + (((addr) & 0xFFFF0000) >> 12))

int main()
{
	DWORD CurMode;
	DWORD width, height;	/*屏幕大小*/
	DWORD PixBits;	/*每像素位数*/
	DWORD VmPhy;	/*显存物理地址*/
	WORD *Modep;
	DWORD ModeCou;	/*模式数量*/
	void *vm;		/*显存映射地址*/
	THREAD_ID ptid;
	long res;	/*返回结果*/
	WORD ModeList[VESA_MAX_MODE];	/*显示模式列表*/
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
	if ((res = KMapPhyAddr(&vm, 0x90000, 0x70000)) != NO_ERROR)	/*取得显卡信息*/
		return res;
	CurMode = *((DWORD*)(vm + 0xFC));
	width = *((WORD*)(vm + 0x312));
	height = *((WORD*)(vm + 0x314));
	PixBits = *((BYTE*)(vm + 0x319));
	VmPhy = *((DWORD*)(vm + 0x328));
	Modep = (WORD*)((DWORD)vm + FAR2LINE(*(DWORD*)(vm + 0x10E)) - 0x90000);
	for (ModeCou = 0; *Modep != 0xFFFF; ModeCou++, Modep++)
		ModeList[ModeCou] = *Modep;
	KFreeAddr(vm);
	if ((res = KMapPhyAddr(&vm, VmPhy, width * height * (PixBits + 7) / 8)) != NO_ERROR)	/*映射显存*/
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
			case VESA_API_CURMODE:
				data[0] = MSG_ATTR_USER;
				data[1] = CurMode;
				KSendMsg(ptid, data, 0);
				break;
			case VESA_API_PUTPIXEL:
				PutPixel(vm, PixBits, width, height, data[1], data[2], data[4]);
				break;
			case VESA_API_GETPIXEL:
				data[0] = MSG_ATTR_USER;
				data[1] = GetPixel(vm, PixBits, width, height, data[1], data[2], &data[2]);
				KSendMsg(ptid, data, 0);
				break;
			case VESA_API_FILLRECT:
				FillRect(vm, PixBits, width, height, data[1], data[2], data[4], data[5], data[6]);
				break;
			case VESA_API_DRAWLINE:
				DrawLine(vm, PixBits, width, height, data[1], data[2], data[4], data[5], data[6]);
				break;				
			case VESA_API_CIRCLE:
				circle(vm, PixBits, width, height, data[1], data[2], data[4], data[5]);
				break;
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_MAP)
		{
			switch (data[3])
			{
			case VESA_API_GETMODE:
				if ((data[0] & 1) && data[1] >= (((ModeCou + 1) >> 1) << 2))
				{
					memcpy32((void*)data[2], ModeList, (ModeCou + 1) >> 1);
					data[1] = ModeCou;
					data[0] = NO_ERROR;
				}
				else
					data[0] = VESA_ERR_ARGS;
				KUnmapProcAddr((void*)data[2], data);
				break;
			case VESA_API_PUTIMAGE:
				data[0] = PutImage(vm, PixBits, width, height, data[4], data[5], (DWORD*)data[2], data[6], data[7]);
				KUnmapProcAddr((void*)data[2], data);
				break;
			case VESA_API_GETIMAGE:
				if (data[0] & 1)
					data[0] = GetImage(vm, PixBits, width, height, data[4], data[5], (DWORD*)data[2], data[6], data[7]);
				else
					data[0] = VESA_ERR_ARGS;
				KUnmapProcAddr((void*)data[2], data);
				break;
			case VESA_API_DRAWSTR:
				data[0] = DrawStr(vm, PixBits, width, height, data[4], data[5], (const BYTE*)data[2], data[6], font);
				KUnmapProcAddr((void*)data[2], data);
				break;
			}
		}
	}
	KFreeAddr(vm);
	KUnregKnlPort(SRV_VESA_PORT);
	return NO_ERROR;
}

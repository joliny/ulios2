/*	gclient.c for ulios graphical user interface
���ߣ�����
���ܣ�GUI�ͻ��˹��ܿ�ʵ��
����޸����ڣ�2011-08-15
*/

#include "gclient.h"
#include "gcres.h"
#include "malloc.h"
#include "../fs/fsapi.h"

/**********��ͼ����ʵ��**********/

const BYTE *GCfont;
DWORD GCwidth, GCheight;
DWORD GCCharWidth, GCCharHeight;

/*��ʼ��GC��*/
long GCinit()
{
	long res;

	if ((res = GUIGetGinfo(&GCwidth, &GCheight)) != NO_ERROR)
		return res;
	if (GCfont == NULL && (res = FNTGetFont(&GCfont, &GCCharWidth, &GCCharHeight)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*����GC��*/
void GCrelease()
{
	DWORD data[MSG_DATA_LEN];

	if (GCfont)
	{
		KUnmapProcAddr((void*)GCfont, data);
		GCfont = NULL;
	}
}

/*���û�ͼ����Ĳ����ͻ���,��һ�������޸���ͼ��ʱ�����uda->vbuf*/
long GCSetArea(UDI_AREA *uda, DWORD width, DWORD height, const UDI_AREA *par, long x, long y)
{
	if (par && par != uda)	/*���и�����*/
	{
		if (x < 0 || x + (long)width > (long)par->width || y < 0 || y + (long)height > (long)par->height)
			return GC_ERR_LOCATION;
		uda->vbuf = par->vbuf + x + y * par->root->width;
		uda->root = par->root;
	}
	else	/*�������뻺��*/
	{
		DWORD *p;
		if ((uda->vbuf = (DWORD*)realloc(uda->vbuf, width * height * sizeof(DWORD))) == NULL)
			return GC_ERR_OUT_OF_MEM;
		for (p = uda->vbuf + width * height - 1; p >= uda->vbuf; p -= 512)	/*���GUI��������,��Ҫ,��ϵͳ����������2Kҳ��,�ʵ�ַ�ݼ�ֵΪ2KB*/
			*p = 0;
		*uda->vbuf = 0;
		uda->root = uda;
	}
	uda->width = width;
	uda->height = height;
	return NO_ERROR;
}

/*���ջ�ͼ���򻺴�*/
void GCFreeArea(UDI_AREA *uda)
{
	if (uda->root == uda && uda->vbuf)
	{
		free(uda->vbuf);
		uda->vbuf = NULL;
	}
}

/*����*/
long GCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c)
{
	if (x >= uda->width || y >= uda->height)
		return GC_ERR_LOCATION;	/*λ��Խ��*/
	uda->vbuf[x + y * uda->root->width] = c;
	return NO_ERROR;
}

/*ȡ��*/
long GCGetPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD *c)
{
	if (x >= uda->width || y >= uda->height)
		return GC_ERR_LOCATION;	/*λ��Խ��*/
	*c = uda->vbuf[x + y * uda->root->width];
	return NO_ERROR;
}

/*��ͼ*/
long GCPutImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h)
{
	long memw;
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*λ�����Դ���*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*�Ƿ��ߴ�*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, img += memw, h--)
		memcpy32(tmpvm, img, w);
	return NO_ERROR;
}

/*ȥ����ɫ����ͼ������*/
static inline void BCDw2Rgb32(DWORD *dest, const DWORD *src, DWORD n, DWORD bc)
{
	while (n--)
	{
		if (*src != bc)
			*dest = *src;
		src++;
		dest++;
	}
}

/*ȥ����ɫ��ͼ*/
long GCPutBCImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h, DWORD bc)
{
	long memw;
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*λ�����Դ���*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*�Ƿ��ߴ�*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, img += memw, h--)
		BCDw2Rgb32(tmpvm, img, w, bc);
	return NO_ERROR;
}

/*��ͼ*/
long GCGetImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h)
{
	long memw;
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*λ�����Դ���*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*�Ƿ��ߴ�*/
	memw = w;
	if (x < 0)
	{
		img -= x;
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		img -= y * memw;
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, img += memw, h--)
		memcpy32(img, tmpvm, w);
	return NO_ERROR;
}

/*������*/
long GCFillRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c)
{
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*λ�����Դ���*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*�Ƿ��ߴ�*/
	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (w > (long)uda->width - x)
		w = (long)uda->width - x;
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (h > (long)uda->height - y)
		h = (long)uda->height - y;

	vbufw = uda->root->width;
	for (tmpvm = uda->vbuf + x + y * vbufw; h > 0; tmpvm += vbufw, h--)
		memset32(tmpvm, c, w);
	return NO_ERROR;
}

#define abs(v) ((v) >= 0 ? (v) : -(v))

/*��Խ���жϻ���*/
static inline void NCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c)
{
	uda->vbuf[x + y * uda->root->width] = c;
}

#define CS_LEFT		1
#define CS_RIGHT	2
#define CS_TOP		4
#define CS_BOTTOM	8

/*Cohen-Sutherland�ü��㷨����*/
static inline DWORD CsEncode(UDI_AREA *uda, long x, long y)
{
	DWORD mask;

	mask = 0;
	if (x < 0)
		mask |= CS_LEFT;
	else if (x >= (long)uda->width)
		mask |= CS_RIGHT;
	if (y < 0)
		mask |= CS_TOP;
	else if (y >= (long)uda->height)
		mask |= CS_BOTTOM;
	return mask;
}

#define F2L_SCALE	0x10000

/*Cohen-Sutherland�㷨�ü�Bresenham�Ľ��㷨����*/
long GCDrawLine(UDI_AREA *uda, long x1, long y1, long x2, long y2, DWORD c)
{
	DWORD mask, mask1, mask2;
	long dx, dy, dx2, dy2;
	long e, xinc, yinc, half;

	/*�ü�*/
	mask1 = CsEncode(uda, x1, y1);
	mask2 = CsEncode(uda, x2, y2);
	if (mask1 || mask2)
	{
		long ydivx, xdivy;

		xdivy = ydivx = 0;
		if (x1 != x2)
			ydivx = ((y2 - y1) * F2L_SCALE) / (x2 - x1);
		if (y1 != y2)
			xdivy = ((x2 - x1) * F2L_SCALE) / (y2 - y1);
		do
		{
			if (mask1 & mask2)	/*�ڲü���һ��,��ȫ�ü���*/
				return NO_ERROR;

			dy = dx = 0;
			mask = mask1 ? mask1 : mask2;
			if (mask & CS_LEFT)
			{
				dx = 0;
				dy = y1 - (x1 * ydivx / F2L_SCALE);
			}
			else if (mask & CS_RIGHT)
			{
				dx = (long)uda->width - 1;
				dy = y1 - ((x1 + 1 - (long)uda->width) * ydivx / F2L_SCALE);
			}
			if (mask & CS_TOP)
			{
				dy = 0;
				dx = x1 - (y1 * xdivy / F2L_SCALE);
			}
			else if (mask & CS_BOTTOM)
			{
				dy = (long)uda->height - 1;
				dx = x1 - ((y1 + 1 - (long)uda->height) * xdivy / F2L_SCALE);
			}
			if (mask == mask1)
			{
				x1 = dx;
				y1 = dy;
				mask1 = CsEncode(uda, dx, dy);
			}
			else
			{
				x2 = dx;
				y2 = dy;
				mask2 = CsEncode(uda, dx, dy);
			}
		}
		while (mask1 || mask2);
	}

	/*����*/
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
			NCPutPixel(uda, x1, y1, c);
			NCPutPixel(uda, x2, y2, c);
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
		if (x1 == x2)	/*���дһ���м��*/
			NCPutPixel(uda, x1, y1, c);
	}
	else
	{
		e = dx2 - dy;
		half = (dy + 1) >> 1;
		while (half--)
		{
			NCPutPixel(uda, x1, y1, c);
			NCPutPixel(uda, x2, y2, c);
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
		if (y1 == y2)	/*���дһ���м��*/
			NCPutPixel(uda, x1, y1, c);
	}
	return NO_ERROR;
}

/*Bresenham�㷨��Բ*/
long GCcircle(UDI_AREA *uda, long cx, long cy, long r, DWORD c)
{
	long x, y, d;

	if (!r)
		return GC_ERR_AREASIZE;
	y = r;
	d = (3 - y) << 1;
	for (x = 0; x <= y; x++)
	{
		GCPutPixel(uda, cx + x, cy + y, c);
		GCPutPixel(uda, cx + x, cy - y, c);
		GCPutPixel(uda, cx - x, cy + y, c);
		GCPutPixel(uda, cx - x, cy - y, c);
		GCPutPixel(uda, cx + y, cy + x, c);
		GCPutPixel(uda, cx + y, cy - x, c);
		GCPutPixel(uda, cx - y, cy + x, c);
		GCPutPixel(uda, cx - y, cy - x, c);
		if (d < 0)
			d += (x << 2) + 6;
		else
			d += ((x - y--) << 2) + 10;
	}
	return NO_ERROR;
}

#define HZ_COUNT	8178

/*���ƺ���*/
long GCDrawHz(UDI_AREA *uda, long x, long y, DWORD hz, DWORD c)
{
	DWORD *tmpvm, vbufw;
	long i, j, HzWidth;
	const WORD *p;

	HzWidth = GCCharWidth * 2;
	if (x <= -HzWidth || x >= (long)uda->width || y <= -(long)GCCharHeight || y >= (long)uda->height)
		return GC_ERR_LOCATION;
	hz = ((hz & 0xFF) - 161) * 94 + ((hz >> 8) & 0xFF) - 161;
	if (hz >= HZ_COUNT)
		return NO_ERROR;
	p = (WORD*)(GCfont + hz * GCCharHeight * 2);
	vbufw = uda->root->width;
	for (j = GCCharHeight - 1; j >= 0; j--, p++, y++)
	{
		if ((DWORD)y >= uda->height)
			continue;
		tmpvm = uda->vbuf + x + y * vbufw;
		for (i = HzWidth - 1; i >= 0; i--, x++, tmpvm++)
			if ((DWORD)x < uda->width && ((*p >> i) & 1u))
				*tmpvm = c;
		x -= HzWidth;
	}
	return NO_ERROR;
}

/*����ASCII�ַ�*/
long GCDrawAscii(UDI_AREA *uda, long x, long y, DWORD ch, DWORD c)
{
	DWORD *tmpvm, vbufw;
	long i, j;
	const BYTE *p;

	if (x <= -(long)GCCharWidth || x >= (long)uda->width || y <= -(long)GCCharHeight || y >= (long)uda->height)
		return GC_ERR_LOCATION;
	p = GCfont + HZ_COUNT * GCCharHeight * 2 + (ch & 0xFF) * GCCharHeight;
	vbufw = uda->root->width;
	for (j = GCCharHeight - 1; j >= 0; j--, p++, y++)
	{
		if ((DWORD)y >= uda->height)
			continue;
		tmpvm = uda->vbuf + x + y * vbufw;
		for (i = GCCharWidth - 1; i >= 0; i--, x++, tmpvm++)
			if ((DWORD)x < uda->width && ((*p >> i) & 1u))
				*tmpvm = c;
		x -= GCCharWidth;
	}
	return NO_ERROR;
}

/*�����ַ���*/
long GCDrawStr(UDI_AREA *uda, long x, long y, const char *str, DWORD c)
{
	DWORD hzf;	/*�����������ַ���־*/

	for (hzf = 0; *str && x < (long)uda->width; str++)
	{
		if ((BYTE)(*str) > 160)
		{
			if (hzf)	/*��ʾ����*/
			{
				GCDrawHz(uda, x, y, ((BYTE)(*str) << 8) | hzf, c);
				x += GCCharWidth * 2;
				hzf = 0;
			}
			else
				hzf = (BYTE)(*str);
		}
		else
		{
			if (hzf)	/*��δ��ʾ��ASCII*/
			{
				GCDrawAscii(uda, x, y, hzf, c);
				x += GCCharWidth;
				hzf = 0;
			}
			GCDrawAscii(uda, x, y, (BYTE)(*str), c);	/*��ʾ��ǰASCII*/
			x += GCCharWidth;
		}
	}
	return NO_ERROR;
}

/*����BMPͼ���ļ�*/
long GCLoadBmp(char *path, DWORD *buf, DWORD len, DWORD *width, DWORD *height)
{
	BYTE BmpHead[32];
	DWORD bmpw, bmph;
	long file;

	if ((file = FSopen(path, FS_OPEN_READ)) < 0)	/*��ȡBMP�ļ�*/
		return file;
	if (FSread(file, &BmpHead[2], 30) < 30 || *((WORD*)&BmpHead[2]) != 0x4D42 || *((WORD*)&BmpHead[30]) != 24)	/*��֤32λ�������*/
	{
		FSclose(file);
		return -1;
	}
	bmpw = *((DWORD*)&BmpHead[20]);
	bmph = *((DWORD*)&BmpHead[24]);
	if (width)
		*width = bmpw;
	if (height)
		*height = bmph;
	if (bmpw * bmph > len)
	{
		FSclose(file);
		return -1;
	}
	FSseek(file, 54, FS_SEEK_SET);
	len = (bmpw * 3 + 3) & 0xFFFFFFFC;
	for (buf += bmpw * (bmph - 1); bmph > 0; bmph--, buf -= bmpw)
	{
		BYTE *src, *dst;

		if (FSread(file, buf, len) < len)
		{
			FSclose(file);
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
	FSclose(file);
	return NO_ERROR;
}

/**********�ؼ���ͼ��ɫ�ʶ���**********/

#define COL_WND_FLAT		0xCCCCCC	// ����ƽ����ɫ
#define COL_WND_BORDER		0x7B858E	// ���ڱ߿�ɫ
#define COL_CAP_GRADDARK	0x589FCE	// ���⽥��ɫ����
#define COL_CAP_GRADLIGHT	0xE1EEF6	// ���⽥��ɫ����
#define COL_CAP_NOFCDARK	0xBFBFBF	// �޽����⽥��ɫ����
#define COL_CAP_NOFCLIGHT	0xFBFBFB	// �޽����⽥��ɫ����

#define COL_BTN_BORDER		0x848a8a	// ��ť�߿�ɫ
#define COL_BTN_GRADDARK	0xa8abaf	// ��ť����ɫ����
#define COL_BTN_GRADLIGHT	0xefeff6	// ��ť����ɫ����
#define COL_BTN_TEXT		0x000000	// ��ť����
#define COL_BTN_CLICK_GRADDARK	0xa8abaf	// ��ť���½���ɫ����
#define COL_BTN_CLICK_GRADLIGHT	0xefeff6	// ��ť���½���ɫ����
#define COL_BTN_CLICK_TEXT		0x000000	// ��ť��������
#define COL_BTN_HOVER_GRADDARK	0x818488	// ��ť��꾭������ɫ����
#define COL_BTN_HOVER_GRADLIGHT	0xf9f9fc	// ��ť��꾭������ɫ����
#define COL_BTN_HOVER_TEXT		0x000000	// ��ť��꾭������
#define COL_BTN_DISABLED_GRADDARK	0xa8abaf	// ��ťʧЧ����ɫ����
#define COL_BTN_DISABLED_GRADLIGHT	0xefeff6	// ��ťʧЧ����ɫ����
#define COL_BTN_DISABLED_TEXT		0x6e6a6a	// ��ťʧЧ����ɫ����

#define COL_TEXT_DARK		0x001C30	// �ı���ɫ
#define COL_TEXT_LIGHT		0xFFE3CF	// �ı���ɫ

/**********�ؼ���ͼ��**********/

/*���ƽ���ɫ��*/
static void FillGradRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c1, DWORD c2)
{
	long i;
	long BaseR, BaseG, BaseB, StepR, StepG, StepB;

	BaseR = (c1 & 0xFF0000) >> 16;
	BaseG = (c1 & 0xFF00) >> 8;
	BaseB = (c1 & 0xFF);
	StepR = ((c2 & 0xFF0000) >> 16) - BaseR;
	StepG = ((c2 & 0xFF00) >> 8) - BaseG;
	StepB = ((c2 & 0xFF)) - BaseB;
	for (i = 0; i < h; i++)
		GCFillRect(uda, x, y + i, w, 1, (((BaseR + StepR * i / h) & 0xFF) << 16) | (((BaseG + StepG * i / h) & 0xFF) << 8) | ((BaseB + StepB * i / h) & 0xFF));
}

/*��ť����*/
static void DrawButton(UDI_AREA *uda, long x, long y, long w, long h, DWORD c1, DWORD c2, DWORD bc)
{
	FillGradRect(uda, x + 1, y + 1, w - 2, h - 2, c1, c2);
	/*����Բ�Ǿ���*/
	GCFillRect(uda, x + 2, y, w - 4, 1, bc);	/*�ϱ߿�*/
	GCFillRect(uda, x + 2, y + h - 1, w - 4, 1, bc);	/*�±߿�*/
	GCFillRect(uda, x, y + 2, 1, h - 4, bc);	/*��߿�*/
	GCFillRect(uda, x + w - 1, y + 2, 1, h - 4, bc);	/*�ұ߿�*/
	GCPutPixel(uda, x + 1, y + 1, bc);	/*���Ͻ�*/
	GCPutPixel(uda, x + w - 2, y + 1, bc);	/*���Ͻ�*/
	GCPutPixel(uda, x + 1, y + h - 2, bc);	/*���½�*/
	GCPutPixel(uda, x + w - 2, y + h - 2, bc);	/*���½�*/
}

/**********�ؼ�����**********/

/*��ʼ��CTRL_GOBJ�ṹ*/
long GCGobjInit(CTRL_GOBJ *gobj, const CTRL_ARGS *args, MSGPROC MsgProc, DRAWPROC DrawProc, DWORD pid, CTRL_GOBJ *ParGobj)
{
	long res;

	gobj->uda.vbuf = NULL;
	if ((res = GCSetArea(&gobj->uda, args->width, args->height, &ParGobj->uda, args->x, args->y)) != NO_ERROR)	/*�����ͼ����*/
		return res;
	memcpy32(&gobj->x, &args->x, 4);	// ������Ҫ�Ĳ���
	if (gobj->MsgProc == NULL)
		gobj->MsgProc = MsgProc;
	gobj->DrawProc = DrawProc;
	if ((res = GUIcreate(pid, (DWORD)gobj, args->x, args->y, args->width, args->height, ParGobj == NULL ? gobj->uda.vbuf : NULL)) != NO_ERROR)
	{
		GCFreeArea(&gobj->uda);
		return res;
	}
	if (ParGobj)
	{
		CTRL_GOBJ *CurGobj;
		CurGobj = ParGobj->chl;
		if (CurGobj)
		{
			while (CurGobj->nxt)	/*Ϊ�˼򻯻��ƴ������,���ú�巨,�����ֵܽڵ�*/
				CurGobj = CurGobj->nxt;
			CurGobj->nxt = gobj;
		}
		else
			ParGobj->chl = gobj;
		gobj->pre = CurGobj;
	}
	else
		gobj->pre = NULL;
	gobj->nxt = NULL;
	gobj->par = ParGobj;
	gobj->chl = NULL;
	return NO_ERROR;
}

/*�ݹ��ͷŴ�����*/
static void FreeGobjList(CTRL_GOBJ *gobj)
{
	CTRL_GOBJ *CurGobj;

	CurGobj = gobj->chl;
	while (CurGobj)
	{
		CTRL_GOBJ *TmpGobj;
		TmpGobj = CurGobj->nxt;
		FreeGobjList(CurGobj);
		CurGobj = TmpGobj;
	}
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];
		ptid.ProcID = INVALID;
		data[MSG_API_ID] = MSG_ATTR_GUI | GM_DESTROY;
		data[GUIMSG_GOBJ_ID] = (DWORD)gobj;
		data[MSG_RES_ID] = NO_ERROR;
		gobj->MsgProc(ptid, data);	/*���ô����������Ϣ������*/
	}
	GCFreeArea(&gobj->uda);
	free(gobj);
}

/*ɾ��������*/
void GCGobjDelete(CTRL_GOBJ *gobj)
{
	CTRL_GOBJ *ParGobj;

	ParGobj = gobj->par;
	if (ParGobj)	// ����
	{
		if (ParGobj->chl == gobj)
			ParGobj->chl = gobj->nxt;
		if (gobj->pre)
			gobj->pre->nxt = gobj->nxt;
		if (gobj->nxt)
			gobj->nxt->pre = gobj->pre;
	}
	FreeGobjList(gobj);
}

/*�ݹ���ƴ�����*/
static void DrawGobjList(CTRL_GOBJ *gobj)
{
	gobj->DrawProc(gobj);
	gobj = gobj->chl;
	while (gobj)
	{
		DrawGobjList(gobj);
		gobj = gobj->nxt;
	}
}

/*���ƴ�����*/
void GCGobjDraw(CTRL_GOBJ *gobj)
{
	DrawGobjList(gobj);
	GUIpaint(gobj->gid, 0, 0, gobj->uda.width, gobj->uda.height);	/*��������ύ*/
}

/*GUI�ͻ�����Ϣ����*/
long GCDispatchMsg(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_GOBJ *gobj;
	long res;

	if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) != MSG_ATTR_GUI)	/*��ȡ�Ϸ���GUI��Ϣ*/
		return GC_ERR_INVALID_GUIMSG;
	if (data[MSG_RES_ID] != NO_ERROR)	/*��������GUI��Ϣ*/
		return data[MSG_RES_ID];
	gobj = (CTRL_GOBJ*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_ATTR_ID] & MSG_API_MASK)
	{
	case GM_CREATE:	/*��ȡ�´����GUI����˶���ID*/
		gobj->gid = data[5];
		break;
	}
	res = gobj->MsgProc(ptid, data);	/*���ô������Ϣ������*/
	switch (data[MSG_ATTR_ID] & MSG_API_MASK)
	{
	case GM_CREATE:	/*�����´�����Gobj���ڵ�*/
		gobj->DrawProc(gobj);
		GUIpaint(gobj->gid, 0, 0, gobj->uda.width, gobj->uda.height);
		break;
	case GM_DESTROY:	/*���ٴ���*/
		GCGobjDelete(gobj);
		break;
	}
	return res;
}

/*�ƶ��򵥴���*/
void GCGobjMove(CTRL_GOBJ *gobj, long x, long y)
{
	if (GCSetArea(&gobj->uda, gobj->uda.width, gobj->uda.height, &gobj->par->uda, x, y) != NO_ERROR)	/*���ô�������*/
		return;
	gobj->x = x;
	gobj->y = y;
	gobj->DrawProc(gobj);
	GUImove(gobj->gid, gobj->x, gobj->y);
}

/*���ü򵥴���λ�ô�С*/
void GCGobjSetSize(CTRL_GOBJ *gobj, long x, long y, DWORD width, DWORD height)
{
	if (GCSetArea(&gobj->uda, width, height, &gobj->par->uda, x, y) != NO_ERROR)	/*���ô�������*/
		return;
	gobj->x = x;
	gobj->y = y;
	gobj->DrawProc(gobj);	/*�ػ洰��*/
	GUIsize(gobj->gid, gobj->uda.root == &gobj->uda ? gobj->uda.vbuf : NULL, gobj->x, gobj->y, gobj->uda.width, gobj->uda.height);	/*���贰���С*/
}

/**********����**********/

/*��������*/
long GCDskCreate(CTRL_DSK **dsk, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj)
{
	CTRL_DSK *NewDsk;
	long res;

	if ((NewDsk = (CTRL_DSK*)malloc(sizeof(CTRL_DSK))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewDsk->obj, args, GCDskDefMsgProc, (DRAWPROC)GCDskDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewDsk);
		return res;
	}
	NewDsk->obj.type = GC_CTRL_TYPE_DESKTOP;
	if (dsk)
		*dsk = NewDsk;
	return NO_ERROR;
}

/*������Ϣ����*/
long GCDskDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	return NO_ERROR;
}

/*������ƴ���*/
void GCDskDefDrawProc(CTRL_DSK *dsk)
{
}

/**********����**********/

#define WND_CAP_HEIGHT		20	/*���ڱ������߶�*/
#define WND_BORDER_WIDTH	1	/*���ڱ߿���*/
#define WND_CAPBTN_SIZE		16	/*��������ť��С*/
#define WND_SIZEBTN_SIZE	14	/*���Ű�ť��С*/
#define WND_MIN_WIDTH		128	/*������С�ߴ�*/
#define WND_MIN_HEIGHT		WND_CAP_HEIGHT

/*��������*/
long GCWndCreate(CTRL_WND **wnd, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *caption)
{
	CTRL_WND *NewWnd;
	long res;

	if ((NewWnd = (CTRL_WND*)malloc(sizeof(CTRL_WND))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewWnd->obj, args, GCWndDefMsgProc, (DRAWPROC)GCWndDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewWnd);
		return res;
	}
	NewWnd->obj.type = GC_CTRL_TYPE_WINDOW;
	NewWnd->obj.style |= WND_STATE_TOP;
	NewWnd->size = NewWnd->min = NewWnd->max = NewWnd->close = NULL;
	if (NewWnd->obj.style & WND_STYLE_CAPTION)	/*�б�����*/
	{
		if (NewWnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width - WND_BORDER_WIDTH * 2, NewWnd->obj.uda.height - WND_CAP_HEIGHT - WND_BORDER_WIDTH, &NewWnd->obj.uda, WND_BORDER_WIDTH, WND_CAP_HEIGHT);	/*�����ͻ���*/
		else	/*�ޱ߿�*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width, NewWnd->obj.uda.height - WND_CAP_HEIGHT, &NewWnd->obj.uda, 0, WND_CAP_HEIGHT);	/*�����ͻ���*/
	}
	else	/*�ޱ�����*/
	{
		if (NewWnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width - WND_BORDER_WIDTH * 2, NewWnd->obj.uda.height - WND_BORDER_WIDTH * 2, &NewWnd->obj.uda, WND_BORDER_WIDTH, WND_BORDER_WIDTH);	/*�����ͻ���*/
		else	/*�ޱ߿�*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width, NewWnd->obj.uda.height, &NewWnd->obj.uda, 0, 0);	/*�����ͻ���*/
	}
	if (caption)
	{
		strncpy(NewWnd->caption, caption, WND_CAP_LEN - 1);
		NewWnd->caption[WND_CAP_LEN - 1] = 0;
	}
	else
		NewWnd->caption[0] = 0;
	NewWnd->MinWidth = WND_MIN_WIDTH;
	NewWnd->MinHeight = WND_MIN_HEIGHT;
	NewWnd->MaxWidth = GCwidth;
	NewWnd->MaxHeight = GCheight;
	NewWnd->x0 = NewWnd->obj.x;
	NewWnd->y0 = NewWnd->obj.y;
	NewWnd->width0 = NewWnd->obj.uda.width;
	NewWnd->height0 = NewWnd->obj.uda.height;
	if (wnd)
		*wnd = NewWnd;
	return NO_ERROR;
}

/*���ڹرհ�ť������*/
static void WndCloseBtnProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];

	ptid.ProcID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_CLOSE;
	data[GUIMSG_GOBJ_ID] = (DWORD)btn->obj.par;
	data[MSG_RES_ID] = NO_ERROR;
	btn->obj.par->MsgProc(ptid, data);
}

/*��������������л�*/
static void WndMaxOrNormal(CTRL_WND *wnd)
{
	if (wnd->obj.uda.width == wnd->MaxWidth && wnd->obj.uda.height == wnd->MaxHeight)
		GCWndSetSize(wnd, wnd->x0, wnd->y0, wnd->width0, wnd->height0);	/*������*/
	else
		GCWndSetSize(wnd, 0, 0, wnd->MaxWidth, wnd->MaxHeight);	/*���*/
}

/*������С���������л�*/
static void WndMinOrNormal(CTRL_WND *wnd)
{
	if (wnd->obj.uda.width == wnd->MinWidth && wnd->obj.uda.height == wnd->MinHeight)
		GCWndSetSize(wnd, wnd->x0, wnd->y0, wnd->width0, wnd->height0);	/*������*/
	else
		GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, wnd->MinWidth, wnd->MinHeight);	/*��С��*/
}

/*������󻯰�ť������*/
static void WndMaxBtnProc(CTRL_BTN *btn)
{
	WndMaxOrNormal((CTRL_WND*)btn->obj.par);
}

/*������С����ť������*/
static void WndMinBtnProc(CTRL_BTN *btn)
{
	WndMinOrNormal((CTRL_WND*)btn->obj.par);
}

/*�������Ű�ť��Ϣ����*/
static long WndSizeBtnProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	if ((data[MSG_API_ID] & MSG_API_MASK) == GM_LBUTTONDOWN)
		GUIdrag(((CTRL_GOBJ*)data[GUIMSG_GOBJ_ID])->par->gid, GM_DRAGMOD_SIZE);
	return GCBtnDefMsgProc(ptid, data);
}

/*������Ϣ����*/
long GCWndDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			CTRL_ARGS args;
			args.height = args.width = WND_CAPBTN_SIZE;
			args.y = (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
			args.style = 0;
			args.MsgProc = NULL;
			if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*�йرհ�ť*/
			{
				args.x = (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->close, &args, wnd->obj.gid, &wnd->obj, "X", &WndCloseImg, WndCloseBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MAXBTN)	/*����󻯰�ť*/
			{
				args.x = WND_CAP_HEIGHT + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->max, &args, wnd->obj.gid, &wnd->obj, "[]", &WndMaxImg, WndMaxBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MINBTN)	/*����С����ť*/
			{
				args.x = WND_CAP_HEIGHT * 2 + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->min, &args, wnd->obj.gid, &wnd->obj, "_ ", &WndMinImg, WndMinBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*�����Ű�ť*/
			{
				args.height = args.width = WND_SIZEBTN_SIZE;
				args.x = wnd->obj.uda.width - WND_SIZEBTN_SIZE;
				args.y = wnd->obj.uda.height - WND_SIZEBTN_SIZE;
				args.MsgProc = WndSizeBtnProc;
				GCBtnCreate(&wnd->size, &args, wnd->obj.gid, &wnd->obj, ".:", &WndSizeImg, NULL);
			}
		}
		break;
	case GM_MOVE:
		wnd->obj.x = data[1];
		wnd->obj.y = data[2];
		break;
	case GM_SIZE:
		if ((wnd->obj.uda.width != wnd->MinWidth || wnd->obj.uda.height != wnd->MinHeight) && (wnd->obj.uda.width != wnd->MaxWidth || wnd->obj.uda.height != wnd->MaxHeight))	/*��¼����������ʱ�Ĵ�С*/
		{
			wnd->x0 = wnd->obj.x;
			wnd->y0 = wnd->obj.y;
			wnd->width0 = wnd->obj.uda.width;
			wnd->height0 = wnd->obj.uda.height;
		}
		break;
	case GM_SETTOP:
		if (data[1])
			wnd->obj.style |= WND_STATE_TOP;
		else
			wnd->obj.style &= (~WND_STATE_TOP);
		if (wnd->obj.style & WND_STYLE_CAPTION)	/*�б�����*/
		{
			if (wnd->obj.style & WND_STATE_TOP)
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*���Ʊ�����*/
			else
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*�����޽�������*/
			GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
			if (wnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
				GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*�ϱ߿�*/
			if (wnd->close)	/*�йرհ�ť*/
				GCBtnDefDrawProc(wnd->close);
			if (wnd->max)	/*����󻯰�ť*/
				GCBtnDefDrawProc(wnd->max);
			if (wnd->min)	/*����С����ť*/
				GCBtnDefDrawProc(wnd->min);
			GUIpaint(wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
		}
		break;
	case GM_DRAG:
		switch (data[1])
		{
		case GM_DRAGMOD_MOVE:
			GUImove(wnd->obj.gid, data[2], data[3]);
			break;
		case GM_DRAGMOD_SIZE:
			GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, data[2], data[3]);
			break;
		}
		break;
	case GM_LBUTTONDOWN:
		if (wnd->obj.style & WND_STYLE_CAPTION && (data[5] >> 16) < WND_CAP_HEIGHT)	/*���������϶�����*/
			GUIdrag(wnd->obj.gid, GM_DRAGMOD_MOVE);
		if (!(wnd->obj.style & WND_STATE_TOP))
			GUISetTop(wnd->obj.gid);
		break;
	case GM_LBUTTONDBCLK:
		if ((wnd->obj.style & (WND_STYLE_CAPTION | WND_STYLE_MAXBTN)) == (WND_STYLE_CAPTION | WND_STYLE_MAXBTN) && (data[5] >> 16) < WND_CAP_HEIGHT)	/*˫���������Ŵ���*/
			WndMaxOrNormal(wnd);
		break;
	case GM_CLOSE:
		GUIdestroy(wnd->obj.gid);
		break;
	}
	return NO_ERROR;
}

/*���ڻ��ƴ���*/
void GCWndDefDrawProc(CTRL_WND *wnd)
{
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*�б�����*/
	{
		if (wnd->obj.style & WND_STATE_TOP)
			FillGradRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*���Ʊ�����*/
		else
			FillGradRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*�����޽�������*/
		GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
	}
	if (wnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
	{
		GCFillRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_BORDER_WIDTH, COL_WND_BORDER);	/*�ϱ߿�*/
		GCFillRect(&wnd->obj.uda, 0, wnd->obj.uda.height - WND_BORDER_WIDTH, wnd->obj.uda.width, WND_BORDER_WIDTH, COL_WND_BORDER);	/*�±߿�*/
		GCFillRect(&wnd->obj.uda, 0, WND_BORDER_WIDTH, WND_BORDER_WIDTH, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, COL_WND_BORDER);	/*��߿�*/
		GCFillRect(&wnd->obj.uda, wnd->obj.uda.width - WND_BORDER_WIDTH, WND_BORDER_WIDTH, WND_BORDER_WIDTH, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, COL_WND_BORDER);	/*�ұ߿�*/
	}
	GCFillRect(&wnd->client, 0, 0, wnd->client.width, wnd->client.height, COL_WND_FLAT);
}

/*���ô��ڱ����ı�*/
void GCWndSetCaption(CTRL_WND *wnd, const char *caption)
{
	if (caption)
	{
		strncpy(wnd->caption, caption, WND_CAP_LEN - 1);
		wnd->caption[WND_CAP_LEN - 1] = 0;
	}
	else
		wnd->caption[0] = 0;
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*�б�����*/
	{
		if (wnd->obj.style & WND_STATE_TOP)
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*���Ʊ�����*/
		else
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*�����޽�������*/
		GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
		if (wnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
			GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*�ϱ߿�*/
		if (wnd->close)	/*�йرհ�ť*/
			GCBtnDefDrawProc(wnd->close);
		if (wnd->max)	/*����󻯰�ť*/
			GCBtnDefDrawProc(wnd->max);
		if (wnd->min)	/*����С����ť*/
			GCBtnDefDrawProc(wnd->min);
		GUIpaint(wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
	}
}

/*���ڼ����С���ߴ�*/
static void WndChkMinMax(CTRL_WND *wnd)
{
	if (wnd->MinWidth < WND_MIN_WIDTH)
		wnd->MinWidth = WND_MIN_WIDTH;
	if (wnd->MaxWidth > GCwidth)
		wnd->MaxWidth = GCwidth;
	if (wnd->MaxWidth < wnd->MinWidth)
		wnd->MaxWidth = wnd->MinWidth;
	if (wnd->MinHeight < WND_MIN_HEIGHT)
		wnd->MinHeight = WND_MIN_HEIGHT;
	if (wnd->MaxHeight > GCheight)
		wnd->MaxHeight = GCheight;
	if (wnd->MaxHeight < wnd->MinHeight)
		wnd->MaxHeight = wnd->MinHeight;
}

/*���ô���λ�ô�С*/
void GCWndSetSize(CTRL_WND *wnd, long x, long y, DWORD width, DWORD height)
{
	WndChkMinMax(wnd);
	if ((long)width < 0)
		width = 0;
	if ((long)height < 0)
		height = 0;
	if (width < wnd->MinWidth)	/*�������*/
		width = wnd->MinWidth;
	else if (width > wnd->MaxWidth)
		width = wnd->MaxWidth;
	if (height < wnd->MinHeight)	/*�����߶�*/
		height = wnd->MinHeight;
	else if (height > wnd->MaxHeight)
		height = wnd->MaxHeight;
	if (GCSetArea(&wnd->obj.uda, width, height, &wnd->obj.par->uda, x, y) != NO_ERROR)
		return;
	wnd->obj.x = x;
	wnd->obj.y = y;
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*�б�����*/
	{
		if (wnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
			GCSetArea(&wnd->client, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, wnd->obj.uda.height - WND_CAP_HEIGHT - WND_BORDER_WIDTH, &wnd->obj.uda, WND_BORDER_WIDTH, WND_CAP_HEIGHT);	/*�����ͻ���*/
		else	/*�ޱ߿�*/
			GCSetArea(&wnd->client, wnd->obj.uda.width, wnd->obj.uda.height - WND_CAP_HEIGHT, &wnd->obj.uda, 0, WND_CAP_HEIGHT);
	}
	else	/*�ޱ�����*/
	{
		if (wnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
			GCSetArea(&wnd->client, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, &wnd->obj.uda, WND_BORDER_WIDTH, WND_BORDER_WIDTH);	/*�����ͻ���*/
		else	/*�ޱ߿�*/
			GCSetArea(&wnd->client, wnd->obj.uda.width, wnd->obj.uda.height, &wnd->obj.uda, 0, 0);
	}
	GCWndDefDrawProc(wnd);
	if (wnd->close)	/*�йرհ�ť*/
	{
		GCSetArea(&wnd->close->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2);
		GCBtnDefDrawProc(wnd->close);
	}
	if (wnd->max)	/*����󻯰�ť*/
	{
		GCSetArea(&wnd->max->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, WND_CAP_HEIGHT + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2);
		GCBtnDefDrawProc(wnd->max);
	}
	if (wnd->min)	/*����С����ť*/
	{
		GCSetArea(&wnd->min->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, WND_CAP_HEIGHT * 2 + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2, (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2);
		GCBtnDefDrawProc(wnd->min);
	}
	if (wnd->size)	/*�����Ű�ť*/
	{
		wnd->size->obj.x = wnd->obj.uda.width - WND_SIZEBTN_SIZE;
		wnd->size->obj.y = wnd->obj.uda.height - WND_SIZEBTN_SIZE;
		GCSetArea(&wnd->size->obj.uda, WND_SIZEBTN_SIZE, WND_SIZEBTN_SIZE, &wnd->obj.uda, wnd->size->obj.x, wnd->size->obj.y);
		GCBtnDefDrawProc(wnd->size);
	}
	GUIsize(wnd->obj.gid, wnd->obj.uda.root == &wnd->obj.uda ? wnd->obj.uda.vbuf : NULL, wnd->obj.x, wnd->obj.y, wnd->obj.uda.width, wnd->obj.uda.height);	/*���贰�ڴ�С*/
	if (wnd->size)	/*�����Ű�ť*/
		GUImove(wnd->size->obj.gid, wnd->size->obj.x, wnd->size->obj.y);
}

/*ȡ�ô��ڿͻ���λ��*/
void GCWndGetClientLoca(CTRL_WND *wnd, long *x, long *y)
{
	DWORD PixCou;

	PixCou = wnd->client.vbuf - wnd->client.root->vbuf;
	*x = PixCou % wnd->client.root->width;
	*y = PixCou / wnd->client.root->width;
}

/**********��ť**********/

/*������ť*/
long GCBtnCreate(CTRL_BTN **btn, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, UDI_IMAGE *img, void (*PressProc)(CTRL_BTN *btn))
{
	CTRL_BTN *NewBtn;
	long res;

	if ((NewBtn = (CTRL_BTN*)malloc(sizeof(CTRL_BTN))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewBtn->obj, args, GCBtnDefMsgProc, (DRAWPROC)GCBtnDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewBtn);
		return res;
	}
	NewBtn->obj.type = GC_CTRL_TYPE_BUTTON;
	if (text)
	{
		strncpy(NewBtn->text, text, BTN_TXT_LEN - 1);
		NewBtn->text[BTN_TXT_LEN - 1] = 0;
	}
	else
		NewBtn->text[0] = 0;
	NewBtn->img = img;
	NewBtn->isPressDown = FALSE;
	NewBtn->PressProc = PressProc;
	if (btn)
		*btn = NewBtn;
	return NO_ERROR;
}

/*��ť��Ϣ����*/
long GCBtnDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_BTN *btn = (CTRL_BTN*)data[GUIMSG_GOBJ_ID];
	UDI_IMAGE *img;

	img = btn->img;
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_LBUTTONUP:	/*���̧��״̬*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED) && btn->isPressDown)
		{
			btn->isPressDown = FALSE;
			if (btn->PressProc)
				btn->PressProc(btn);	/*ִ�а�ť����*/
		}	/*�������ư�ť*/
	case GM_MOUSEENTER:	/*������״̬*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
		{
			DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_HOVER_GRADLIGHT, COL_BTN_HOVER_GRADDARK, COL_BTN_BORDER);	/*���ư�ť*/
			if (img)
				GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
			else
				GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_HOVER_TEXT);
			GUIpaint(btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		}
		break;
	case GM_MOUSELEAVE:	/*����뿪״̬*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
		{
			btn->isPressDown = FALSE;
			GCBtnDefDrawProc(btn);
			GUIpaint(btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		}
		break;
	case GM_LBUTTONDOWN:	/*��갴��״̬*/
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
		{
			btn->isPressDown = TRUE;
			DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_CLICK_GRADDARK, COL_BTN_CLICK_GRADLIGHT, COL_BTN_BORDER);	/*���ư�ť*/
			if (img)
				GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
			else
				GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_CLICK_TEXT);
			GUIpaint(btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		}
		break;
	}
	return NO_ERROR;
}

/*��ť���ƴ���*/
void GCBtnDefDrawProc(CTRL_BTN *btn)
{
	UDI_IMAGE *img;

	img = btn->img;
	if (btn->obj.style & BTN_STYLE_DISABLED)
	{
		DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_DISABLED_GRADLIGHT, COL_BTN_DISABLED_GRADDARK, COL_BTN_BORDER);	/*���ư�ť*/
		if (img)
			GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
		else
			GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_DISABLED_TEXT);
	}
	else
	{
		DrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_GRADLIGHT, COL_BTN_GRADDARK, COL_BTN_BORDER);	/*���ư�ť*/
		if (img)
			GCPutBCImage(&btn->obj.uda, (btn->obj.uda.width - (long)img->width) / 2, (btn->obj.uda.height - (long)img->height) / 2, img->buf, img->width, img->height, 0xFFFFFFFF);
		else
			GCDrawStr(&btn->obj.uda, (btn->obj.uda.width - (long)strlen(btn->text) * (long)GCCharWidth) / 2, (btn->obj.uda.height - (long)GCCharHeight) / 2, btn->text, COL_BTN_TEXT);
	}
}

/*���ð�ť�ı�*/
void GCBtnSetText(CTRL_BTN *btn, const char *text)
{
	if (text)
	{
		strncpy(btn->text, text, BTN_TXT_LEN - 1);
		btn->text[BTN_TXT_LEN - 1] = 0;
	}
	else
		btn->text[0] = 0;
	GCGobjDraw(&btn->obj);
}

/*���ð�ťΪ��������ʽ*/
void GCBtnSetDisable(CTRL_BTN *btn, BOOL isDisable)
{
	if (isDisable)
	{
		if (btn->obj.style & BTN_STYLE_DISABLED)
			return;
		btn->obj.style |= BTN_STYLE_DISABLED;
	}
	else
	{
		if (!(btn->obj.style & BTN_STYLE_DISABLED))
			return;
		btn->obj.style &= (~BTN_STYLE_DISABLED);
	}
	GCGobjDraw(&btn->obj);
}

/**********��̬�ı���**********/

/*������̬�ı���*/
long GCTxtCreate(CTRL_TXT **txt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text)
{
	CTRL_TXT *NewTxt;
	long res;

	if ((NewTxt = (CTRL_TXT*)malloc(sizeof(CTRL_TXT))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewTxt->obj, args, GCTxtDefMsgProc, (DRAWPROC)GCTxtDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewTxt);
		return res;
	}
	NewTxt->obj.type = GC_CTRL_TYPE_TEXT;
	if (text)
	{
		strncpy(NewTxt->text, text, TXT_TXT_LEN - 1);
		NewTxt->text[TXT_TXT_LEN - 1] = 0;
	}
	else
		NewTxt->text[0] = 0;
	if (txt)
		*txt = NewTxt;
	return NO_ERROR;
}

/*��̬�ı�����Ϣ����*/
long GCTxtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	return NO_ERROR;
}

/*��̬�ı�����ƴ���*/
void GCTxtDefDrawProc(CTRL_TXT *txt)
{
	GCFillRect(&txt->obj.uda, 0, ((long)txt->obj.uda.height - (long)GCCharHeight) / 2, txt->obj.uda.width, GCCharHeight, COL_WND_FLAT);	/*��ɫ*/
	GCDrawStr(&txt->obj.uda, 0, ((long)txt->obj.uda.height - (long)GCCharHeight) / 2, txt->text, COL_TEXT_DARK);
}

/*�����ı����ı�*/
void GCTxtSetText(CTRL_TXT *txt, const char *text)
{
	if (text)
	{
		strncpy(txt->text, text, TXT_TXT_LEN - 1);
		txt->text[TXT_TXT_LEN - 1] = 0;
	}
	else
		txt->text[0] = 0;
	GCGobjDraw(&txt->obj);
}

/**********���б༭��**********/

/*�������б༭��*/
long GCSedtCreate(CTRL_SEDT **edt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, void (*EnterProc)(CTRL_SEDT *edt))
{
	CTRL_SEDT *NewEdt;
	long res;

	if ((NewEdt = (CTRL_SEDT*)malloc(sizeof(CTRL_SEDT))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewEdt->obj, args, GCSedtDefMsgProc, (DRAWPROC)GCSedtDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewEdt);
		return res;
	}
	NewEdt->obj.type = GC_CTRL_TYPE_SLEDIT;
	if (text)
	{
		strncpy(NewEdt->text, text, SEDT_TXT_LEN - 1);
		NewEdt->text[SEDT_TXT_LEN - 1] = 0;
	}
	else
		NewEdt->text[0] = 0;
	NewEdt->CurC = NewEdt->FstC = NewEdt->text;
	NewEdt->EnterProc = EnterProc;
	if (edt)
		*edt = NewEdt;
	return NO_ERROR;
}

/*���б༭���ı�������*/
static void SedtDrawText(CTRL_SEDT *edt)
{
	GCFillRect(&edt->obj.uda, 1, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, edt->obj.uda.width - 2, GCCharHeight, COL_BTN_GRADLIGHT);	/*��ɫ*/
	GCDrawStr(&edt->obj.uda, 1, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, edt->FstC, COL_TEXT_DARK);
	if (edt->obj.style & SEDT_STATE_FOCUS)	/*��ý���ʱ����ʾ���*/
		GCFillRect(&edt->obj.uda, 1 + (edt->CurC - edt->FstC) * GCCharWidth, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, 2, GCCharHeight, COL_TEXT_DARK);	/*���*/
	GUIpaint(edt->obj.gid, 1, ((long)edt->obj.uda.height - (long)GCCharHeight) / 2, edt->obj.uda.width - 2, GCCharHeight);
}

/*���б༭����Ϣ����*/
long GCSedtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_SEDT *edt = (CTRL_SEDT*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_SETFOCUS:
		if (data[1])
			edt->obj.style |= SEDT_STATE_FOCUS;
		else
			edt->obj.style &= (~SEDT_STATE_FOCUS);
		SedtDrawText(edt);
		break;
	case GM_LBUTTONDOWN:	/*��갴��*/
		if (edt->obj.style & SEDT_STYLE_RDONLY)
			break;
		if (!(edt->obj.style & SEDT_STATE_FOCUS))
			GUISetFocus(edt->obj.gid);
		{
			DWORD len;
			len = ((data[5] & 0xFFFF) - 1 + GCCharWidth / 2) / GCCharWidth;
			for (edt->CurC = edt->FstC; *edt->CurC; edt->CurC++, len--)
				if (len == 0)
					break;
		}
		SedtDrawText(edt);
		break;
	case GM_KEY:	/*����*/
		if (edt->obj.style & SEDT_STYLE_RDONLY)
			break;
		if ((data[1] & (KBD_STATE_LCTRL | KBD_STATE_RCTRL)) && (data[1] & 0xFF) == ' ')	/*����Ctrl+�ո�,�������뷨*/
		{
			if (edt->obj.style & SEDT_STATE_IME)
			{
				edt->obj.style &= (~SEDT_STATE_IME);
				IMECloseBar();
			}
			else
			{
				edt->obj.style |= SEDT_STATE_IME;
				IMEOpenBar(edt->obj.x + edt->obj.par->x, edt->obj.y + edt->obj.par->y - 20);
			}
			break;
		}
		else if (edt->obj.style & SEDT_STATE_IME)	/*���뷨�ѿ���*/
		{
			THREAD_ID ptid;
			ptid.ProcID = SRV_IME_PORT;
			ptid.ThedID = INVALID;
			data[MSG_API_ID] = MSG_ATTR_IME | IME_API_PUTKEY;
			if (KSendMsg(&ptid, data, 0) != NO_ERROR)	/*�����ʹ�����뷨*/
				edt->obj.style &= (~SEDT_STATE_IME);
			data[MSG_API_ID] = MSG_ATTR_GUI | GM_KEY;
			break;
		}
		switch (data[1] & 0xFF)
		{
		case '\b':	/*�˸��*/
			if (edt->CurC > edt->text)	/*���ǰ�����ַ�*/
			{
				strcpy(edt->CurC - 1, edt->CurC);	/*�ƶ��ַ���*/
				edt->CurC--;	/*�ƶ����*/
				if (edt->FstC > edt->text)
					edt->FstC--;	/*�ƶ����ַ�*/
			}
			break;
		case '\r':	/*�س���*/
			if (edt->EnterProc)
				edt->EnterProc(edt);	/*ִ�лس�����*/
			break;
		case '\0':	/*�޿��Է��Ű���*/
			switch ((data[1] >> 8) & 0xFF)
			{
			case 0x4B:	/*���ͷ*/
				if (edt->CurC > edt->text)
				{
					edt->CurC--;	/*�ƶ����*/
					if (edt->FstC > edt->CurC)
						edt->FstC = edt->CurC;	/*�ƶ����ַ�*/
				}
				break;
			case 0x4D:	/*�Ҽ�ͷ*/
				if (*edt->CurC != '\0')
				{
					DWORD len;
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*������ڿ���ʾ�ַ���*/
					edt->CurC++;	/*�ƶ����*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*�ƶ����ַ�*/
				}
				break;
			case 0x47:	/*HOME��*/
				edt->CurC = edt->FstC = edt->text;
				break;
			case 0x4F:	/*END��*/
				{
					DWORD len;
					len = strlen(edt->text);
					edt->CurC = edt->text + len;	/*�ƶ����*/
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*������ڿ���ʾ�ַ���*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*�ƶ����ַ�*/
				}
				break;
			case 0x53:	/*DELETE��*/
				if (*edt->CurC != '\0')
					strcpy(edt->CurC, edt->CurC + 1);	/*�ƶ��ַ���*/
				if (strlen(edt->FstC) < (edt->obj.uda.width - 4) / GCCharWidth && edt->FstC > edt->text)
					edt->FstC--;	/*�ƶ����ַ�*/
				break;
			}
			break;
		default:	/*�����ַ����뻺��*/
			{
				DWORD len;
				len = strlen(edt->text);
				if (len < SEDT_TXT_LEN - 1)	/*����û��*/
				{
					char *strp;
					strp = edt->text + len + 1;
					do
					*strp = *(strp - 1);
					while (--strp > edt->CurC);	/*��������ַ�*/
					*edt->CurC = (char)(BYTE)data[1];
					edt->CurC++;	/*�ƶ����*/
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*������ڿ���ʾ�ַ���*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*�ƶ����ַ�*/
				}
			}
		}
		SedtDrawText(edt);
		break;
	case GM_IMEPUTKEY:
		{
			DWORD len;
			len = strlen(edt->text);
			if (len < SEDT_TXT_LEN - 2)	/*����û��*/
			{
				char *strp;
				strp = edt->text + len + 2;
				do
				*strp = *(strp - 2);
				while (--strp > edt->CurC);	/*��������ַ�*/
				*edt->CurC = (char)(BYTE)data[1];
				*(edt->CurC + 1) = (char)(BYTE)(data[1] >> 8);
				edt->CurC += 2;	/*�ƶ����*/
				len = (edt->obj.uda.width - 4) / GCCharWidth;	/*������ڿ���ʾ�ַ���*/
				if (edt->FstC < edt->CurC - len)
					edt->FstC = edt->CurC - len;	/*�ƶ����ַ�*/
				SedtDrawText(edt);
			}
		}
		break;
	}
	return NO_ERROR;
}

/*���б༭����ƴ���*/
void GCSedtDefDrawProc(CTRL_SEDT *edt)
{
	GCFillRect(&edt->obj.uda, 0, 0, edt->obj.uda.width, 1, COL_BTN_BORDER);	/*�ϱ߿�*/
	GCFillRect(&edt->obj.uda, 0, edt->obj.uda.height - 1, edt->obj.uda.width, 1, COL_BTN_BORDER);	/*�±߿�*/
	GCFillRect(&edt->obj.uda, 0, 1, 1, edt->obj.uda.height - 2, COL_BTN_BORDER);	/*��߿�*/
	GCFillRect(&edt->obj.uda, edt->obj.uda.width - 1, 1, 1, edt->obj.uda.height - 2, COL_BTN_BORDER);	/*�ұ߿�*/
	GCFillRect(&edt->obj.uda, 1, 1, edt->obj.uda.width - 2, edt->obj.uda.height - 2, (edt->obj.style & SEDT_STYLE_RDONLY) ? COL_BTN_GRADDARK : COL_BTN_GRADLIGHT);	/*��ɫ*/
	GCDrawStr(&edt->obj.uda, 1, (edt->obj.uda.height - (long)GCCharHeight) / 2, edt->FstC, COL_TEXT_DARK);	/*�ı�*/
	if (edt->obj.style & SEDT_STATE_FOCUS)	/*��ý���ʱ����ʾ���*/
		GCFillRect(&edt->obj.uda, 1 + (edt->CurC - edt->FstC) * GCCharWidth, (edt->obj.uda.height - (long)GCCharHeight) / 2, 2, GCCharHeight, COL_TEXT_DARK);	/*���*/
}

/*���õ��б༭���ı�*/
void GCSedtSetText(CTRL_SEDT *edt, const char *text)
{
	DWORD len;

	if (text)
	{
		strncpy(edt->text, text, SEDT_TXT_LEN - 1);
		edt->text[SEDT_TXT_LEN - 1] = 0;
	}
	else
		edt->text[0] = 0;
	len = strlen(edt->text);
	edt->CurC = edt->text + len;	/*�ƶ����*/
	len = (edt->obj.uda.width - 4) / GCCharWidth;	/*������ڿ���ʾ�ַ���*/
	if (edt->FstC < edt->CurC - len)
		edt->FstC = edt->CurC - len;	/*�ƶ����ַ�*/
	GCGobjDraw(&edt->obj);
}

/*׷�ӵ��б༭���ı�*/
void GCSedtAddText(CTRL_SEDT *edt, const char *text)
{
	DWORD len;

	if (text == NULL)
		return;
	len = strlen(edt->text);
	strncpy(edt->text + len, text, SEDT_TXT_LEN - 1 - len);	/*׷���ַ���*/
	edt->text[SEDT_TXT_LEN - 1] = 0;
	len += strlen(edt->text + len);
	edt->CurC = edt->text + len;	/*�ƶ����*/
	len = (edt->obj.uda.width - 4) / GCCharWidth;	/*������ڿ���ʾ�ַ���*/
	if (edt->FstC < edt->CurC - len)
		edt->FstC = edt->CurC - len;	/*�ƶ����ַ�*/
	GCGobjDraw(&edt->obj);
}

/**********���б༭��**********/

/**********������**********/

#define SCRL_BTN_SIZE		16	/*��������ť��С*/

/*����������*/
long GCScrlCreate(CTRL_SCRL **scl, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, long min, long max, long pos, long page, void (*ChangeProc)(CTRL_SCRL *scl))
{
	CTRL_SCRL *NewScl;
	long res;

	if (min >= max || page <= 0 || page > max - min || pos < min || pos > max - page)
		return GC_ERR_WRONG_ARGS;
	if ((NewScl = (CTRL_SCRL*)malloc(sizeof(CTRL_SCRL))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewScl->obj, args, GCScrlDefMsgProc, (DRAWPROC)GCScrlDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewScl);
		return res;
	}
	NewScl->obj.type = GC_CTRL_TYPE_SCROLL;
	NewScl->min = min;
	NewScl->max = max;
	NewScl->pos = pos;
	NewScl->page = page;
	NewScl->ChangeProc = ChangeProc;
	if (scl)
		*scl = NewScl;
	return NO_ERROR;
}

/*��������С��ť������*/
static void ScrlSubBtnProc(CTRL_BTN *sub)
{
	CTRL_SCRL *scl;
	CTRL_BTN *drag;

	if (sub == NULL)
		return;
	scl = (CTRL_SCRL*)sub->obj.par;
	drag = scl->drag;
	if (drag == NULL)
		return;
	if (scl->obj.style & SCRL_STYLE_VER)	/*��ֱ��*/
	{
		if (drag->obj.y <= (long)sub->obj.uda.height)
			return;
		drag->obj.y -= drag->obj.uda.height;
		if (drag->obj.y < (long)sub->obj.uda.height)
			drag->obj.y = (long)sub->obj.uda.height;
		scl->pos = (drag->obj.y - (long)sub->obj.uda.height) * (scl->max - scl->min) / (scl->add->obj.y - (long)sub->obj.uda.height) + scl->min;
	}
	else
	{
		if (drag->obj.x <= (long)sub->obj.uda.width)
			return;
		drag->obj.x -= drag->obj.uda.width;
		if (drag->obj.x < (long)sub->obj.uda.width)
			drag->obj.x = (long)sub->obj.uda.width;
		scl->pos = (drag->obj.x - (long)sub->obj.uda.width) * (scl->max - scl->min) / (scl->add->obj.x - (long)sub->obj.uda.width) + scl->min;
	}
	GCFillRect(&drag->obj.uda, 0, 0, drag->obj.uda.width, drag->obj.uda.height, COL_CAP_GRADLIGHT);	/*�ָ���ɫ*/
	GCSetArea(&drag->obj.uda, drag->obj.uda.width, drag->obj.uda.height, &scl->obj.uda, drag->obj.x, drag->obj.y);
	GCBtnDefDrawProc(drag);
	GUImove(drag->obj.gid, drag->obj.x, drag->obj.y);
	if (scl->ChangeProc)
		scl->ChangeProc(scl);
}

/*����������ť������*/
static void ScrlAddBtnProc(CTRL_BTN *add)
{
	CTRL_SCRL *scl;
	CTRL_BTN *drag;

	if (add == NULL)
		return;
	scl = (CTRL_SCRL*)add->obj.par;
	drag = scl->drag;
	if (drag == NULL)
		return;
	if (scl->obj.style & SCRL_STYLE_VER)	/*��ֱ��*/
	{
		if (drag->obj.y >= add->obj.y - (long)drag->obj.uda.height)
			return;
		drag->obj.y += drag->obj.uda.height;
		if (drag->obj.y > add->obj.y - (long)drag->obj.uda.height)
			drag->obj.y = add->obj.y - (long)drag->obj.uda.height;
		scl->pos = (drag->obj.y - (long)scl->sub->obj.uda.height) * (scl->max - scl->min) / (add->obj.y - (long)scl->sub->obj.uda.height) + scl->min;
	}
	else
	{
		if (drag->obj.x >= add->obj.x - (long)drag->obj.uda.width)
			return;
		drag->obj.x += drag->obj.uda.width;
		if (drag->obj.x > add->obj.x - (long)drag->obj.uda.width)
			drag->obj.x = add->obj.x - (long)drag->obj.uda.width;
		scl->pos = (drag->obj.x - (long)scl->sub->obj.uda.width) * (scl->max - scl->min) / (add->obj.x - (long)scl->sub->obj.uda.width) + scl->min;
	}
	GCFillRect(&drag->obj.uda, 0, 0, drag->obj.uda.width, drag->obj.uda.height, COL_CAP_GRADLIGHT);	/*�ָ���ɫ*/
	GCSetArea(&drag->obj.uda, drag->obj.uda.width, drag->obj.uda.height, &scl->obj.uda, drag->obj.x, drag->obj.y);
	GCBtnDefDrawProc(drag);
	GUImove(drag->obj.gid, drag->obj.x, drag->obj.y);
	if (scl->ChangeProc)
		scl->ChangeProc(scl);
}

/*�������϶���ť��Ϣ����*/
static long ScrlDragBtnProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_BTN *btn = (CTRL_BTN*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_DRAG:
		switch (data[1])
		{
		case GM_DRAGMOD_MOVE:
			{
				CTRL_SCRL *scl = (CTRL_SCRL*)btn->obj.par;
				CTRL_BTN *sub = scl->sub;
				CTRL_BTN *add = scl->add;
				if (scl->obj.style & SCRL_STYLE_VER)	/*��ֱ��*/
				{
					if ((long)data[3] <= (long)sub->obj.uda.height)
					{
						if (btn->obj.y <= (long)sub->obj.uda.height)
							break;
						data[3] = sub->obj.uda.height;
					}
					else if ((long)data[3] >= add->obj.y - (long)btn->obj.uda.height)
					{
						if (btn->obj.y >= add->obj.y - (long)btn->obj.uda.height)
							break;
						data[3] = (DWORD)add->obj.y - btn->obj.uda.height;
					}
					btn->obj.y = data[3];
					scl->pos = (btn->obj.y - (long)sub->obj.uda.height) * (scl->max - scl->min) / (add->obj.y - (long)sub->obj.uda.height) + scl->min;
				}
				else	/*ˮƽ��*/
				{
					if ((long)data[2] <= (long)sub->obj.uda.width)
					{
						if (btn->obj.x <= (long)sub->obj.uda.width)
							break;
						data[2] = sub->obj.uda.width;
					}
					else if ((long)data[2] >= add->obj.x - (long)btn->obj.uda.width)
					{
						if (btn->obj.x >= add->obj.x - (long)btn->obj.uda.width)
							break;
						data[2] = (DWORD)add->obj.x - btn->obj.uda.width;
					}
					btn->obj.x = data[2];
					scl->pos = (btn->obj.x - (long)sub->obj.uda.width) * (scl->max - scl->min) / (add->obj.x - (long)sub->obj.uda.width) + scl->min;
				}
				GCFillRect(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_CAP_GRADLIGHT);	/*�ָ���ɫ*/
				GCSetArea(&btn->obj.uda, btn->obj.uda.width, btn->obj.uda.height, &scl->obj.uda, btn->obj.x, btn->obj.y);
				GCBtnDefDrawProc(btn);
				GUImove(btn->obj.gid, btn->obj.x, btn->obj.y);
				if (scl->ChangeProc)
					scl->ChangeProc(scl);
			}
			break;
		}
		break;
	case GM_LBUTTONDOWN:
		GUIdrag(btn->obj.gid, GM_DRAGMOD_MOVE);
		break;
	}
	return GCBtnDefMsgProc(ptid, data);
}

/*��������Ϣ����*/
long GCScrlDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_SCRL *scl = (CTRL_SCRL*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			CTRL_ARGS args;
			args.style = 0;
			args.MsgProc = NULL;
			if (scl->obj.style & SCRL_STYLE_VER)	/*��ֱ��*/
			{
				args.x = 0;
				args.width = scl->obj.uda.width;
				args.height = (scl->obj.uda.height < SCRL_BTN_SIZE * 3) ? scl->obj.uda.height / 3 : SCRL_BTN_SIZE;
				args.y = 0;
				GCBtnCreate(&scl->sub, &args, scl->obj.gid, &scl->obj, "��", &ScrlVSubImg, ScrlSubBtnProc);
				args.y = scl->obj.uda.height - args.height;
				GCBtnCreate(&scl->add, &args, scl->obj.gid, &scl->obj, "��", &ScrlVAddImg, ScrlAddBtnProc);
				args.y = (scl->pos - scl->min) * (scl->obj.uda.height - args.height * 2) / (scl->max - scl->min) + args.height;
				args.height = scl->page * (scl->obj.uda.height - args.height * 2) / (scl->max - scl->min);
				args.MsgProc = ScrlDragBtnProc;
				GCBtnCreate(&scl->drag, &args, scl->obj.gid, &scl->obj, "-", &ScrlVDragImg, NULL);
			}
			else	/*ˮƽ��*/
			{
				args.y = 0;
				args.height = scl->obj.uda.height;
				args.width = (scl->obj.uda.width < SCRL_BTN_SIZE * 3) ? scl->obj.uda.width / 3 : SCRL_BTN_SIZE;
				args.x = 0;
				GCBtnCreate(&scl->sub, &args, scl->obj.gid, &scl->obj, "<", &ScrlHSubImg, ScrlSubBtnProc);
				args.x = scl->obj.uda.width - args.width;
				GCBtnCreate(&scl->add, &args, scl->obj.gid, &scl->obj, ">", &ScrlHAddImg, ScrlAddBtnProc);
				args.x = (scl->pos - scl->min) * (scl->obj.uda.width - args.width * 2) / (scl->max - scl->min) + args.width;
				args.width = scl->page * (scl->obj.uda.width - args.width * 2) / (scl->max - scl->min);
				args.MsgProc = ScrlDragBtnProc;
				GCBtnCreate(&scl->drag, &args, scl->obj.gid, &scl->obj, "|", &ScrlHDragImg, NULL);
			}
		}
		break;
	case GM_LBUTTONDOWN:
		if (scl->obj.style & SCRL_STYLE_VER)	/*��ֱ��*/
		{
			if ((data[5] >> 16) < scl->drag->obj.y)	/*��С*/
				ScrlSubBtnProc(scl->sub);
			else	/*����*/
				ScrlAddBtnProc(scl->add);
		}
		else	/*ˮƽ��*/
		{
			if ((data[5] & 0xFFFF) < scl->drag->obj.x)	/*��С*/
				ScrlSubBtnProc(scl->sub);
			else	/*����*/
				ScrlAddBtnProc(scl->add);
		}
		break;
	}
	return NO_ERROR;
}

/*���������ƴ���*/
void GCScrlDefDrawProc(CTRL_SCRL *scl)
{
	GCFillRect(&scl->obj.uda, 0, 0, scl->obj.uda.width, scl->obj.uda.height, COL_CAP_GRADLIGHT);
}

/*���ù�����λ�ô�С*/
void GCScrlSetSize(CTRL_SCRL *scl, long x, long y, DWORD width, DWORD height)
{
	if ((long)width < SCRL_BTN_SIZE)	/*�������*/
		width = SCRL_BTN_SIZE;
	else if (width > GCwidth)
		width = GCwidth;
	if ((long)height < SCRL_BTN_SIZE)	/*�����߶�*/
		height = SCRL_BTN_SIZE;
	else if (height > GCheight)
		height = GCheight;
	if (GCSetArea(&scl->obj.uda, width, height, &scl->obj.par->uda, x, y) != NO_ERROR)
		return;
	scl->obj.x = x;
	scl->obj.y = y;
	GCScrlDefDrawProc(scl);
	if (scl->sub && scl->add && scl->drag)
	{
		if (scl->obj.style & SCRL_STYLE_VER)	/*��ֱ��*/
		{
			height = (scl->obj.uda.height < SCRL_BTN_SIZE * 3) ? scl->obj.uda.height / 3 : SCRL_BTN_SIZE;
			GCSetArea(&scl->sub->obj.uda, width, height, &scl->obj.uda, 0, 0);
			GCBtnDefDrawProc(scl->sub);
			scl->add->obj.y = scl->obj.uda.height - height;
			GCSetArea(&scl->add->obj.uda, width, height, &scl->obj.uda, 0, scl->add->obj.y);
			GCBtnDefDrawProc(scl->add);
			scl->drag->obj.y = (scl->pos - scl->min) * (scl->add->obj.y - height) / (scl->max - scl->min) + height;
			GCSetArea(&scl->drag->obj.uda, width, scl->page * (scl->add->obj.y - height) / (scl->max - scl->min), &scl->obj.uda, 0, scl->drag->obj.y);
			GCBtnDefDrawProc(scl->drag);
		}
		else	/*ˮƽ��*/
		{
			width = (scl->obj.uda.width < SCRL_BTN_SIZE * 3) ? scl->obj.uda.width / 3 : SCRL_BTN_SIZE;
			GCSetArea(&scl->sub->obj.uda, width, height, &scl->obj.uda, 0, 0);
			GCBtnDefDrawProc(scl->sub);
			scl->add->obj.x = scl->obj.uda.width - width;
			GCSetArea(&scl->add->obj.uda, width, height, &scl->obj.uda, scl->add->obj.x, 0);
			GCBtnDefDrawProc(scl->add);
			scl->drag->obj.x = (scl->pos - scl->min) * (scl->add->obj.x - width) / (scl->max - scl->min) + width;
			GCSetArea(&scl->drag->obj.uda, scl->page * (scl->add->obj.x - width) / (scl->max - scl->min), height, &scl->obj.uda, scl->drag->obj.x, 0);
			GCBtnDefDrawProc(scl->drag);
		}
	}
	GUIsize(scl->obj.gid, scl->obj.uda.root == &scl->obj.uda ? scl->obj.uda.vbuf : NULL, scl->obj.x, scl->obj.y, scl->obj.uda.width, scl->obj.uda.height);	/*�����������С*/
	if (scl->sub && scl->add && scl->drag)
	{
		GUIsize(scl->sub->obj.gid, NULL, scl->sub->obj.x, scl->sub->obj.y, scl->sub->obj.uda.width, scl->sub->obj.uda.height);
		GUIsize(scl->add->obj.gid, NULL, scl->add->obj.x, scl->add->obj.y, scl->add->obj.uda.width, scl->add->obj.uda.height);
		GUIsize(scl->drag->obj.gid, NULL, scl->drag->obj.x, scl->drag->obj.y, scl->drag->obj.uda.width, scl->drag->obj.uda.height);
	}
}

/*���ù���������*/
long GCScrlSetData(CTRL_SCRL *scl, long min, long max, long pos, long page)
{
	CTRL_BTN *drag;
	if (min >= max || page <= 0 || page > max - min || pos < min || pos > max - page)
		return GC_ERR_WRONG_ARGS;
	scl->min = min;
	scl->max = max;
	scl->pos = pos;
	scl->page = page;
	drag = scl->drag;
	if (drag)
	{
		GCFillRect(&drag->obj.uda, 0, 0, drag->obj.uda.width, drag->obj.uda.height, COL_CAP_GRADLIGHT);	/*�ָ���ɫ*/
		if (scl->obj.style & SCRL_STYLE_VER)	/*��ֱ��*/
		{
			DWORD height;
			height = scl->sub->obj.uda.height;
			drag->obj.y = (pos - min) * (scl->obj.uda.height - height * 2) / (max - min) + height;
			GCSetArea(&drag->obj.uda, drag->obj.uda.width, page * (scl->obj.uda.height - height * 2) / (max - min), &scl->obj.uda, 0, drag->obj.y);
			GCBtnDefDrawProc(drag);
		}
		else	/*ˮƽ��*/
		{
			DWORD width;
			width = scl->sub->obj.uda.width;
			drag->obj.x = (pos - min) * (scl->obj.uda.width - width * 2) / (max - min) + width;
			GCSetArea(&drag->obj.uda, page * (scl->obj.uda.width - width * 2) / (max - min), drag->obj.uda.height, &scl->obj.uda, drag->obj.x, 0);
			GCBtnDefDrawProc(drag);
		}
		GUIsize(drag->obj.gid, NULL, drag->obj.x, drag->obj.y, drag->obj.uda.width, drag->obj.uda.height);
	}
	return NO_ERROR;
}

/**********�б��**********/

#define LST_MIN_WIDTH		32	/*�б����С�ߴ�*/
#define LST_MIN_HEIGHT		32

long GCLstCreate(CTRL_LST **lst, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, void (*SelProc)(CTRL_LST *lst))
{
	CTRL_LST *NewLst;
	long res;

	if ((NewLst = (CTRL_LST*)malloc(sizeof(CTRL_LST))) == NULL)
		return GC_ERR_OUT_OF_MEM;
	if ((res = GCGobjInit(&NewLst->obj, args, GCLstDefMsgProc, (DRAWPROC)GCLstDefDrawProc, pid, ParGobj)) != NO_ERROR)
	{
		free(NewLst);
		return res;
	}
	NewLst->obj.type = GC_CTRL_TYPE_LIST;
	NewLst->TextX = NewLst->MaxWidth = NewLst->ItemCou = 0;
	NewLst->SelItem = NewLst->CurItem = NewLst->item = NULL;
	NewLst->vscl = NewLst->hscl = NULL;
	NewLst->SelProc = SelProc;
	GCSetArea(&NewLst->cont, NewLst->obj.uda.width - 2, NewLst->obj.uda.height - 2, &NewLst->obj.uda, 1, 1);	/*�������ݻ�ͼ��*/
	if (lst)
		*lst = NewLst;
	return NO_ERROR;
}

static void LstFreeItems(LIST_ITEM *item)
{
	while (item)
	{
		LIST_ITEM *TmpItem;
		TmpItem = item->nxt;
		free(item);
		item = TmpItem;
	}
}

static void LstVsclProc(CTRL_SCRL *scl)
{
	CTRL_LST *lst;

	lst = (CTRL_LST*)scl->obj.par;
	if (lst->CurPos == scl->pos)
		return;
	if (lst->CurPos < scl->pos)	/*�б�����ƶ�*/
	{
		do
			lst->CurItem = lst->CurItem->nxt;
		while (++(lst->CurPos) < scl->pos);
	}
	else	/*�б���ǰ�ƶ�*/
	{
		do
			lst->CurItem = lst->CurItem->pre;
		while (--(lst->CurPos) > scl->pos);
	}
	GCGobjDraw(&lst->obj);
}

static void LstHsclProc(CTRL_SCRL *scl)
{
	CTRL_LST *lst;

	lst = (CTRL_LST*)scl->obj.par;
	if (lst->TextX == scl->pos)
		return;
	lst->TextX = scl->pos;
	GCGobjDraw(&lst->obj);
}

static void LstMoveItem(CTRL_LST *lst, long move)
{
	CTRL_SCRL *scl;
	scl = lst->vscl;
	if (scl)	/*û�й�����,�����ƶ�*/
	{
		if (move > 0)
		{
			if (scl->pos >= scl->max - scl->page)	/*��������*/
				return;
			if ((move += scl->pos) > scl->max - scl->page)
				move = scl->max - scl->page;
		}
		else
		{
			if (scl->pos <= 0)	/*������С��*/
				return;
			if ((move += scl->pos) < 0)
				move = 0;
		}
		GCScrlSetData(scl, 0, scl->max, move, scl->page);
		LstVsclProc(scl);
	}
}

long GCLstDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_LST *lst = (CTRL_LST*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_LBUTTONDOWN:
		if ((data[5] && 0xFFFF) >= 1 && (data[5] && 0xFFFF) <= lst->cont.width && (data[5] >> 16) >= 1 && (data[5] >> 16) <= lst->cont.height)
		{
			DWORD y;
			LIST_ITEM *item;
			if (lst->SelItem)
			{
				lst->SelItem->attr &= (~LSTITM_ATTR_SELECTED);
				lst->SelItem = NULL;
			}
			for (y = 0, item = lst->CurItem; item; y++, item = item->nxt)
				if (y == ((data[5] >> 16) - 1) / GCCharHeight)
				{
					item->attr |= LSTITM_ATTR_SELECTED;
					break;
				}
			lst->SelItem = item;
			GCGobjDraw(&lst->obj);
			if (lst->SelProc)
				lst->SelProc(lst);
		}
		break;
	case GM_MOUSEWHEEL:
		LstMoveItem(lst, (long)data[4]);
		break;
	case GM_DESTROY:
		LstFreeItems(lst->item);
		lst->TextX = lst->MaxWidth = lst->ItemCou = 0;
		lst->SelItem = lst->CurItem = lst->item = NULL;
		break;
	}
	return NO_ERROR;
}

void GCLstDefDrawProc(CTRL_LST *lst)
{
	DWORD y, DrawFlag;
	LIST_ITEM *item;

	for (y = 0, DrawFlag = 0, item = lst->CurItem; y < lst->cont.height; y += GCCharHeight)
	{
		GCFillRect(&lst->cont, 0, y, lst->cont.width, GCCharHeight, (DrawFlag ^= 1) ? COL_BTN_GRADDARK : COL_BTN_GRADLIGHT);
		if (item)
		{
			GCDrawStr(&lst->cont, -lst->TextX, y, item->text, (item->attr & LSTITM_ATTR_SELECTED) ? COL_TEXT_LIGHT : COL_TEXT_DARK);
			item = item->nxt;
		}
	}
	GCFillRect(&lst->obj.uda, 0, 0, lst->obj.uda.width, 1, COL_BTN_BORDER);	/*�ϱ߿�*/
	GCFillRect(&lst->obj.uda, 0, lst->obj.uda.height - 1, lst->obj.uda.width, 1, COL_BTN_BORDER);	/*�±߿�*/
	GCFillRect(&lst->obj.uda, 0, 1, 1, lst->obj.uda.height - 2, COL_BTN_BORDER);	/*��߿�*/
	GCFillRect(&lst->obj.uda, lst->obj.uda.width - 1, 1, 1, lst->obj.uda.height - 2, COL_BTN_BORDER);	/*�ұ߿�*/
}

/*�����б��λ�ô�С*/
void GCLstSetSize(CTRL_LST *lst, long x, long y, DWORD width, DWORD height)
{
	DWORD VsclLen, HsclLen;
	CTRL_ARGS argsv, argsh;

	if ((long)width < LST_MIN_WIDTH)	/*�������*/
		width = LST_MIN_WIDTH;
	if ((long)height < LST_MIN_HEIGHT)	/*�����߶�*/
		height = LST_MIN_HEIGHT;
	if (GCSetArea(&lst->obj.uda, width, height, &lst->obj.par->uda, x, y) != NO_ERROR)
		return;
	lst->obj.x = x;
	lst->obj.y = y;
	GCSetArea(&lst->cont, lst->obj.uda.width - 2, lst->obj.uda.height - 2, &lst->obj.uda, 1, 1);	/*���ݻ�ͼ��*/
	VsclLen = lst->cont.height / GCCharHeight;
	if (lst->ItemCou > VsclLen)
	{
		while (lst->CurPos > lst->ItemCou - VsclLen)
		{
			lst->CurItem = lst->CurItem->pre;
			lst->CurPos--;
		}
		argsv.width = (lst->obj.uda.width < SCRL_BTN_SIZE * 2) ? lst->obj.uda.width / 2 : SCRL_BTN_SIZE;
		argsv.height = lst->obj.uda.height - 2;
		argsv.x = lst->obj.uda.width - argsv.width - 1;
		argsv.y = 1;
		lst->cont.width -= argsv.width;
	}
	else
	{
		lst->CurItem = lst->item;
		lst->CurPos = 0;
		if (lst->vscl)
		{
			GUIdestroy(lst->vscl->obj.gid);	/*�Զ��������������*/
			lst->vscl = NULL;
		}
		VsclLen = 0;
	}
	HsclLen = lst->cont.width;
	if (lst->MaxWidth > HsclLen)
	{
		if (lst->TextX > lst->MaxWidth - HsclLen)
			lst->TextX = lst->MaxWidth - HsclLen;
		argsh.width = HsclLen;
		argsh.height = (lst->obj.uda.height < SCRL_BTN_SIZE * 2) ? lst->obj.uda.height / 2 : SCRL_BTN_SIZE;
		argsh.x = 1;
		argsh.y = lst->obj.uda.height - argsh.height - 1;
		lst->cont.height -= argsh.height;
	}
	else
	{
		lst->TextX = 0;
		if (lst->hscl)
		{
			GUIdestroy(lst->hscl->obj.gid);	/*�Զ����ٺ��������*/
			lst->hscl = NULL;
		}
		HsclLen = 0;
	}
	GCLstDefDrawProc(lst);
	GUIsize(lst->obj.gid, lst->obj.uda.root == &lst->obj.uda ? lst->obj.uda.vbuf : NULL, lst->obj.x, lst->obj.y, lst->obj.uda.width, lst->obj.uda.height);	/*�����������С*/
	if (VsclLen)
	{
		if (lst->vscl == NULL)	/*�������������*/
		{
			argsv.style = SCRL_STYLE_VER;
			argsv.MsgProc = NULL;
			GCScrlCreate(&lst->vscl, &argsv, lst->obj.gid, &lst->obj, 0, lst->ItemCou, lst->CurPos, VsclLen, LstVsclProc);
		}
		else	/*���������������С*/
		{
			GCScrlSetSize(lst->vscl, argsv.x, argsv.y, argsv.width, argsv.height);
			GCScrlSetData(lst->vscl, 0, lst->ItemCou, lst->CurPos, VsclLen);
		}
		if (lst->hscl)
			GCScrlSetSize(lst->hscl, 1, lst->hscl->obj.y, lst->hscl->obj.uda.width - argsv.width, lst->hscl->obj.uda.height);
	}
	if (HsclLen)
	{
		if (lst->hscl == NULL)	/*���Ӻ��������*/
		{
			argsh.style = SCRL_STYLE_HOR;
			argsh.MsgProc = NULL;
			GCScrlCreate(&lst->hscl, &argsh, lst->obj.gid, &lst->obj, 0, lst->MaxWidth, lst->TextX, HsclLen, LstHsclProc);
		}
		else	/*���������������С*/
		{
			GCScrlSetSize(lst->hscl, argsh.x, argsh.y, argsh.width, argsh.height);
			GCScrlSetData(lst->hscl, 0, lst->MaxWidth, lst->TextX, HsclLen);
		}
	}
}

/*������*/
long GCLstInsertItem(CTRL_LST *lst, LIST_ITEM *pre, const char *text, LIST_ITEM **item)
{
	LIST_ITEM *NewItem, *nxt;
	DWORD len;

	if ((NewItem = (LIST_ITEM*)malloc(sizeof(LIST_ITEM))) == NULL)	/*��������*/
		return GC_ERR_OUT_OF_MEM;
	if (text)	/*�����ı�*/
	{
		DWORD MaxWidth;
		strncpy(NewItem->text, text, LSTITM_TXT_LEN - 1);
		NewItem->text[LSTITM_TXT_LEN - 1] = 0;
		MaxWidth = strlen(NewItem->text) * GCCharWidth;	/*�޸�������ؿ��*/
		if (lst->MaxWidth < MaxWidth)
			lst->MaxWidth = MaxWidth;
	}
	else
		NewItem->text[0] = 0;
	NewItem->attr = 0;
	nxt = (pre ? pre->nxt : lst->item);	/*�½ڵ��������*/
	NewItem->pre = pre;
	NewItem->nxt = nxt;
	if (pre)
		pre->nxt = NewItem;
	else
		lst->item = NewItem;
	if (nxt)
		nxt->pre = NewItem;
	if (lst->CurItem == NULL)	/*���õ�ǰ�λ��*/
	{
		nxt = lst->CurItem = NewItem;
		lst->CurPos = 0;
	}
	else if (NewItem->nxt && NewItem->pre != lst->CurItem)
	{
		for (nxt = lst->item;; nxt = nxt->nxt)
		{
			if (nxt == lst->CurItem)
				break;
			if (nxt == NewItem)
			{
				lst->CurPos++;
				break;
			}
		}
	}
	else
		nxt = lst->CurItem;
	lst->ItemCou++;	/*��������*/
	len = lst->cont.height / GCCharHeight;
	if (lst->ItemCou > len)
	{
		if (lst->vscl == NULL)
		{
			CTRL_ARGS args;	/*�������������*/
			args.width = (lst->obj.uda.width < SCRL_BTN_SIZE * 2) ? lst->obj.uda.width / 2 : SCRL_BTN_SIZE;
			args.height = lst->obj.uda.height - 2;
			args.x = lst->obj.uda.width - args.width - 1;
			args.y = 1;
			args.style = SCRL_STYLE_VER;
			args.MsgProc = NULL;
			GCScrlCreate(&lst->vscl, &args, lst->obj.gid, &lst->obj, 0, lst->ItemCou, lst->CurPos, len, LstVsclProc);
			lst->cont.width -= args.width;
			if (lst->hscl)
				GCScrlSetSize(lst->hscl, 1, lst->hscl->obj.y, lst->hscl->obj.uda.width - args.width, lst->hscl->obj.uda.height);
		}
		else
			GCScrlSetData(lst->vscl, 0, lst->ItemCou, lst->CurPos, len);
	}
	else
	{
		lst->CurItem = lst->item;
		lst->CurPos = 0;
	}
	len = lst->cont.width;
	if (lst->MaxWidth > len)
	{
		if (lst->hscl == NULL)
		{
			CTRL_ARGS args;	/*���Ӻ��������*/
			args.width = len;
			args.height = (lst->obj.uda.height < SCRL_BTN_SIZE * 2) ? lst->obj.uda.height / 2 : SCRL_BTN_SIZE;
			args.x = 1;
			args.y = lst->obj.uda.height - args.height - 1;
			args.style = SCRL_STYLE_HOR;
			args.MsgProc = NULL;
			GCScrlCreate(&lst->hscl, &args, lst->obj.gid, &lst->obj, 0, lst->MaxWidth, lst->TextX, len, LstHsclProc);
			lst->cont.height -= args.height;
		}
		else
			GCScrlSetData(lst->hscl, 0, lst->MaxWidth, lst->TextX, len);
	}
	if (nxt == lst->CurItem)
	{
		len = (lst->cont.height + GCCharHeight - 1) / GCCharHeight;
		while (len)
		{
			if (nxt == NewItem)
			{
				GCGobjDraw(&lst->obj);
				break;
			}
			nxt = nxt->nxt;
			len--;
		}
	}
	if (item)
		*item = NewItem;

	return NO_ERROR;
}

/*ɾ����*/
long GCLstDeleteItem(CTRL_LST *lst, LIST_ITEM *item)
{
	if (item->pre)
		item->pre->nxt = item->nxt;
	else
		lst->item = item->nxt;
	if (item->nxt)
		item->nxt->pre = item->pre;
	free(item);

	return NO_ERROR;
}

/*ɾ��������*/
long GCLstDelAllItem(CTRL_LST *lst)
{
	if (lst->vscl)
	{
		GUIdestroy(lst->vscl->obj.gid);	/*�Զ��������������*/
		lst->vscl = NULL;
		lst->cont.width = lst->obj.uda.width - 2;
	}
	if (lst->hscl)
	{
		GUIdestroy(lst->hscl->obj.gid);	/*�Զ����ٺ��������*/
		lst->hscl = NULL;
		lst->cont.height = lst->obj.uda.height - 2;
	}
	LstFreeItems(lst->item);
	lst->TextX = lst->MaxWidth = lst->ItemCou = 0;
	lst->SelItem = lst->CurItem = lst->item = NULL;
	GCGobjDraw(&lst->obj);
	return NO_ERROR;
}

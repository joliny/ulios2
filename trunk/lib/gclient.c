/*	gclient.c for ulios graphical user interface
	���ߣ�����
	���ܣ�GUI�ͻ��˹��ܿ�ʵ��
	����޸����ڣ�2011-08-15
*/

#include "gclient.h"
#include "malloc.h"
#include "../fs/fsapi.h"

/**********��ͼ����ʵ��**********/

const BYTE *GCfont;
DWORD GCwidth, GCheight;
DWORD GCCharWidth, GCCharHeight;
THREAD_ID GCGuiPtid, GCFontPtid;

/*��ʼ��GC��*/
long GCinit()
{
	long res;

	if ((res = KGetKptThed(SRV_GUI_PORT, &GCGuiPtid)) != NO_ERROR)
		return res;
	if ((res = GUIGetGinfo(GCGuiPtid, &GCwidth, &GCheight)) != NO_ERROR)
		return res;
	if ((res = KGetKptThed(SRV_FONT_PORT, &GCFontPtid)) != NO_ERROR)
		return res;
	if (GCfont == NULL && (res = FNTGetFont(GCFontPtid, &GCfont, &GCCharWidth, &GCCharHeight)) != NO_ERROR)
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
		float ydivx, xdivy;

		xdivy = ydivx = 0.f;
		if (x1 != x2)
			ydivx = (float)(y2 - y1) / (float)(x2 - x1);
		if (y1 != y2)
			xdivy = (float)(x2 - x1) / (float)(y2 - y1);
		do
		{
			if (mask1 & mask2)	/*�ڲü���һ��,��ȫ�ü���*/
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
				dx = (long)uda->width - 1;
				dy = y1 - (x1 + 1 - (long)uda->width) * ydivx;
			}
			if (mask & CS_TOP)
			{
				dy = 0;
				dx = x1 - y1 * xdivy;
			}
			else if (mask & CS_BOTTOM)
			{
				dy = (long)uda->height - 1;
				dx = x1 - (y1 + 1 - (long)uda->height) * xdivy;
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
long GCLoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height)
{
	BYTE BmpHead[32];
	THREAD_ID FsPtid;
	long bmpw, bmph, file, res;

	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)	/*ȡ���ļ�ϵͳ�����߳�*/
		return res;
	if ((file = FSopen(FsPtid, path, FS_OPEN_READ)) < 0)	/*��ȡBMP�ļ�*/
		return file;
	if (FSread(FsPtid, file, &BmpHead[2], 30) < 30 || *((WORD*)&BmpHead[2]) != 0x4D42 || *((WORD*)&BmpHead[30]) != 24)	/*��֤32λ�������*/
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

/**********�ؼ���ͼ��ɫ�ʶ���**********/

#define COL_WND_FLAT		0xCCCCCC	// ����ƽ����ɫ
#define COL_WND_BORDER		0x7B858E	// ���ڱ߿�ɫ
#define COL_CAP_GRADDARK	0x589FCE	// ���⽥��ɫ����
#define COL_CAP_GRADLIGHT	0xE1EEF6	// ���⽥��ɫ����
#define COL_CAP_NOFCDARK	0x8F8F8F	// �޽����⽥��ɫ����
#define COL_CAP_NOFCLIGHT	0xFBFBFB	// �޽����⽥��ɫ����
#define COL_BTN_BORDER		0x7B858E	// ��ť�߿�ɫ
#define COL_BTN_GRADDARK	0x89B0CD	// ��ť����ɫ����
#define COL_BTN_GRADLIGHT	0xD7E9F5	// ��ť����ɫ����
#define COL_ABTN_GRADDARK	0x6189A8	// ���ť����ɫ����
#define COL_ABTN_GRADLIGHT	0xDDEAF2	// ���ť����ɫ����
#define COL_TEXT_DARK		0x001C30	// �ı���ɫ
#define COL_TEXT_LIGHT		0xFFE3CF	// �ı���ɫ

/**********�ؼ���ͼ��**********/

/*���ƽ���ɫ��*/
static void FillGradRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c1, DWORD c2)
{
	long i;
	long BaseR, BaseG, BaseB, StepR, StepG, StepB;

	BaseR = c1 & 0xFF0000;
	BaseG = c1 & 0xFF00;
	BaseB = c1 & 0xFF;
	StepR = (c2 & 0xFF0000) - BaseR;
	StepG = (c2 & 0xFF00) - BaseG;
	StepB = (c2 & 0xFF) - BaseB;
	for (i = 0; i < h; i++)
		GCFillRect(uda, x, y + i, w, 1, ((BaseR + StepR * i / h) & 0xFF0000) | ((BaseG + StepG * i / h) & 0xFF00) | ((BaseB + StepB * i / h) & 0xFF));
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
	if ((res = GUIcreate(GCGuiPtid, pid, (DWORD)gobj, args->x, args->y, args->width, args->height, ParGobj == NULL ? gobj->uda.vbuf : NULL)) != NO_ERROR)
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
	CTRL_GOBJ *CurGobj;

	gobj->DrawProc(gobj);
	CurGobj = gobj->chl;
	while (CurGobj)
	{
		DrawGobjList(CurGobj);
		CurGobj = CurGobj->nxt;
	}
}

/*���ƴ�����*/
void GCGobjDraw(CTRL_GOBJ *gobj)
{
	DrawGobjList(gobj);
	GUIpaint(GCGuiPtid, gobj->gid, 0, 0, gobj->uda.width, gobj->uda.height);	/*��������ύ*/
}

/*GUI�ͻ�����Ϣ����*/
long GCDispatchMsg(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_GOBJ *gobj;
	long res;

	if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) != MSG_ATTR_GUI)	/*��ȡ�Ϸ���GUI��Ϣ*/
		return GC_ERR_INVALID_GUIMSG;
	if (data[MSG_RES_ID] != NO_ERROR)	/*��������GUI��Ϣ*/
		return GC_ERR_WRONG_GUIMSG;
	gobj = (CTRL_GOBJ*)data[GUIMSG_GOBJ_ID];
	if ((data[MSG_ATTR_ID] & MSG_API_MASK) == GM_CREATE)	/*��ȡ�´����GUI����˶���ID*/
		gobj->gid = data[5];
	res = gobj->MsgProc(ptid, data);	/*���ô������Ϣ������*/
	switch (data[MSG_ATTR_ID] & MSG_API_MASK)
	{
	case GM_CREATE:	/*�����´�����Gobj���ڵ�*/
		if (gobj->uda.root == &gobj->uda)
			GCGobjDraw(gobj);
		break;
	case GM_DESTROY:	/*���ٴ���*/
		GCGobjDelete(gobj);
		break;
	}
	return res;
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
#define WND_MIN_HEIGHT		(WND_CAP_HEIGHT + WND_SIZEBTN_SIZE)

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
	NewWnd->obj.style |= WND_STYLE_FOCUS;
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

/*�������Ŵ���*/
static void WndChangeSize(CTRL_WND *wnd, long x, long y, DWORD width, DWORD height)
{
	if ((long)width < WND_MIN_WIDTH && (long)height < WND_MIN_HEIGHT)
		return;
	if ((long)width < WND_MIN_WIDTH)	/*�����ߴ�,��ֹ��С*/
		width = WND_MIN_WIDTH;
	else if ((long)height < WND_MIN_HEIGHT)
		height = WND_MIN_HEIGHT;
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
	if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*�йرհ�ť*/
	{
		GCSetArea(&wnd->close->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, wnd->close->obj.x, wnd->close->obj.y);
		GCBtnDefDrawProc(wnd->close);
	}
	if (wnd->obj.style & WND_STYLE_MAXBTN)	/*����󻯰�ť*/
	{
		GCSetArea(&wnd->max->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, wnd->max->obj.x, wnd->max->obj.y);
		GCBtnDefDrawProc(wnd->max);
	}
	if (wnd->obj.style & WND_STYLE_MINBTN)	/*����С����ť*/
	{
		GCSetArea(&wnd->min->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, wnd->min->obj.x, wnd->min->obj.y);
		GCBtnDefDrawProc(wnd->min);
	}
	if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*�����Ű�ť*/
	{
		wnd->size->obj.x = wnd->obj.uda.width - WND_SIZEBTN_SIZE;
		wnd->size->obj.y = wnd->obj.uda.height - WND_SIZEBTN_SIZE;
		GCSetArea(&wnd->size->obj.uda, WND_SIZEBTN_SIZE, WND_SIZEBTN_SIZE, &wnd->obj.uda, wnd->size->obj.x, wnd->size->obj.y);
		GCBtnDefDrawProc(wnd->size);
	}
	GUIsize(GCGuiPtid, wnd->obj.gid, wnd->obj.uda.vbuf, x, y, width, height);	/*���贰�ڴ�С*/
	if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*�����Ű�ť*/
		GUImove(GCGuiPtid, wnd->size->obj.gid, wnd->size->obj.x, wnd->size->obj.y);
}

/*��������������л�*/
static void WndMaxOrNormal(CTRL_WND *wnd)
{
	if (wnd->obj.x != 0 || wnd->obj.y != 0 || wnd->obj.uda.width != GCwidth || wnd->obj.uda.height != GCheight)
		WndChangeSize(wnd, 0, 0, GCwidth, GCheight);	/*���*/
	else
		WndChangeSize(wnd, wnd->x0, wnd->y0, wnd->width0, wnd->height0);	/*������*/
}

/*������󻯰�ť������*/
static void WndMaxBtnProc(CTRL_BTN *btn)
{
	WndMaxOrNormal((CTRL_WND*)btn->obj.par);
}

/*������С����ť������*/
static void WndMinBtnProc(CTRL_BTN *btn)
{
	WndChangeSize((CTRL_WND*)btn->obj.par, 0, 0, WND_MIN_WIDTH, WND_MIN_HEIGHT);
}

/*�������Ű�ť��Ϣ����*/
static long WndSizeBtnProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	if ((data[MSG_API_ID] & MSG_API_MASK) == GM_LBUTTONDOWN)
		GUIdrag(GCGuiPtid, ((CTRL_GOBJ*)data[GUIMSG_GOBJ_ID])->par->gid, GM_DRAGMOD_SIZE);
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
				GCBtnCreate(&wnd->close, &args, wnd->obj.gid, &wnd->obj, "X", WndCloseBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MAXBTN)	/*����󻯰�ť*/
			{
				args.x = WND_CAP_HEIGHT + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->max, &args, wnd->obj.gid, &wnd->obj, "[]", WndMaxBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MINBTN)	/*����С����ť*/
			{
				args.x = WND_CAP_HEIGHT * 2 + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->min, &args, wnd->obj.gid, &wnd->obj, "_ ", WndMinBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*�����Ű�ť*/
			{
				args.height = args.width = WND_SIZEBTN_SIZE;
				args.x = wnd->obj.uda.width - WND_SIZEBTN_SIZE;
				args.y = wnd->obj.uda.height - WND_SIZEBTN_SIZE;
				args.MsgProc = WndSizeBtnProc;
				GCBtnCreate(&wnd->size, &args, wnd->obj.gid, &wnd->obj, ".:", NULL);
			}
		}
		break;
	case GM_MOVE:
		wnd->obj.x = data[1];
		wnd->obj.y = data[2];
	case GM_SIZE:
		if (wnd->obj.x != 0 || wnd->obj.y != 0 || wnd->obj.uda.width != GCwidth || wnd->obj.uda.height != GCheight)	/*��¼���ڷ����ʱ�Ĵ�С*/
		{
			wnd->x0 = wnd->obj.x;
			wnd->y0 = wnd->obj.y;
			wnd->width0 = wnd->obj.uda.width;
			wnd->height0 = wnd->obj.uda.height;
		}
		break;
	case GM_SETFOCUS:
		if (data[1])
			wnd->obj.style |= WND_STYLE_FOCUS;
		else
			wnd->obj.style &= (~WND_STYLE_FOCUS);
		if (wnd->obj.style & WND_STYLE_CAPTION)	/*�б�����*/
		{
			if (wnd->obj.style & WND_STYLE_FOCUS)
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*���Ʊ�����*/
			else
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*�����޽�������*/
			GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
			if (wnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
				GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*�ϱ߿�*/
			if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*�йرհ�ť*/
				GCBtnDefDrawProc(wnd->close);
			if (wnd->obj.style & WND_STYLE_MAXBTN)	/*����󻯰�ť*/
				GCBtnDefDrawProc(wnd->max);
			if (wnd->obj.style & WND_STYLE_MINBTN)	/*����С����ť*/
				GCBtnDefDrawProc(wnd->min);
			GUIpaint(GCGuiPtid, wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
		}
		break;
	case GM_DRAG:
		if (data[1] == GM_DRAGMOD_SIZE)
			WndChangeSize(wnd, wnd->obj.x, wnd->obj.y, data[2], data[3]);
		break;
	case GM_LBUTTONDOWN:
		if (wnd->obj.style & WND_STYLE_CAPTION && (data[5] >> 16) < WND_CAP_HEIGHT)	/*���������϶�����*/
			GUIdrag(GCGuiPtid, wnd->obj.gid, GM_DRAGMOD_MOVE);
		if (!(wnd->obj.style & WND_STYLE_FOCUS))
			GUISetFocus(GCGuiPtid, wnd->obj.gid, TRUE);
		break;
	case GM_LBUTTONDBCLK:
		if (wnd->obj.style & WND_STYLE_CAPTION && (data[5] >> 16) < WND_CAP_HEIGHT)	/*˫���������Ŵ���*/
			WndMaxOrNormal(wnd);
		break;
	case GM_CLOSE:
		GUIdestroy(GCGuiPtid, wnd->obj.gid);
		break;
	}
	return NO_ERROR;
}

/*���ڻ��ƴ���*/
void GCWndDefDrawProc(CTRL_WND *wnd)
{
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*�б�����*/
	{
		if (wnd->obj.style & WND_STYLE_FOCUS)
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
		if (wnd->obj.style & WND_STYLE_FOCUS)
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*���Ʊ�����*/
		else
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*�����޽�������*/
		GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
		if (wnd->obj.style & WND_STYLE_BORDER)	/*�б߿�*/
			GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*�ϱ߿�*/
		if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*�йرհ�ť*/
			GCBtnDefDrawProc(wnd->close);
		if (wnd->obj.style & WND_STYLE_MAXBTN)	/*����󻯰�ť*/
			GCBtnDefDrawProc(wnd->max);
		if (wnd->obj.style & WND_STYLE_MINBTN)	/*����С����ť*/
			GCBtnDefDrawProc(wnd->min);
		GUIpaint(GCGuiPtid, wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
	}
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
long GCBtnCreate(CTRL_BTN **btn, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, void (*PressProc)(CTRL_BTN *btn))
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
	NewBtn->isPressDown = FALSE;
	NewBtn->PressProc = PressProc;
	if (btn)
		*btn = NewBtn;
	return NO_ERROR;
}

/*��ť����*/
static void BtnDrawButton(UDI_AREA *uda, long x, long y, long w, long h, DWORD c1, DWORD c2, DWORD bc, const char *text, DWORD tc)
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
	GCDrawStr(uda, x + (w - (long)strlen(text) * (long)GCCharWidth) / 2, y + (h - (long)GCCharHeight) / 2, text, tc);
}

/*��ť��Ϣ����*/
long GCBtnDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_BTN *btn = (CTRL_BTN*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_LBUTTONUP:	/*���̧��״̬*/
		if (btn->isPressDown)
		{
			btn->isPressDown = FALSE;
			if (btn->PressProc)
				btn->PressProc(btn);
		}
	case GM_MOUSEENTER:	/*������״̬*/
		BtnDrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_ABTN_GRADLIGHT, COL_ABTN_GRADDARK, COL_BTN_BORDER, btn->text, COL_TEXT_DARK);	/*���ư�ť*/
		GUIpaint(GCGuiPtid, btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		break;
	case GM_MOUSELEAVE:	/*����뿪״̬*/
		btn->isPressDown = FALSE;
		GCBtnDefDrawProc(btn);
		GUIpaint(GCGuiPtid, btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		break;
	case GM_LBUTTONDOWN:	/*��갴��״̬*/
		btn->isPressDown = TRUE;
		BtnDrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_ABTN_GRADDARK, COL_ABTN_GRADLIGHT, COL_BTN_BORDER, btn->text, COL_TEXT_LIGHT);	/*���ư�ť*/
		GUIpaint(GCGuiPtid, btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		break;
	}
	return NO_ERROR;
}

/*��ť���ƴ���*/
void GCBtnDefDrawProc(CTRL_BTN *btn)
{
	BtnDrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_GRADLIGHT, COL_BTN_GRADDARK, COL_BTN_BORDER, btn->text, COL_TEXT_DARK);	/*���ư�ť*/
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
	GCDrawStr(&txt->obj.uda, ((long)txt->obj.uda.width - (long)strlen(txt->text) * (long)GCCharWidth) / 2, ((long)txt->obj.uda.height - (long)GCCharHeight) / 2, txt->text, COL_TEXT_DARK);
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
long GCSedtCreate(CTRL_SEDT **edt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text)
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
	if (edt)
		*edt = NewEdt;
	return NO_ERROR;
}

/*���б༭����Ϣ����*/
long GCSedtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	return NO_ERROR;
}

/*��ť���б༭��*/
static void SedtDrawSLEdit(UDI_AREA *uda, long x, long y, long w, long h, DWORD c, DWORD bc, const char *text, DWORD tc)
{
	GCFillRect(uda, x, y, w, 1, bc);	/*�ϱ߿�*/
	GCFillRect(uda, x, y + h - 1, w, 1, bc);	/*�±߿�*/
	GCFillRect(uda, x, y + 1, 1, h - 2, bc);	/*��߿�*/
	GCFillRect(uda, x + w - 1, y + 1, 1, h - 2, bc);	/*�ұ߿�*/
	GCFillRect(uda, x + 1, y + 1, w - 2, h - 2, c);	/*��ɫ*/
	GCDrawStr(uda, x + (w - (long)strlen(text) * (long)GCCharWidth) / 2, y + (h - (long)GCCharHeight) / 2, text, tc);
}

/*���б༭����ƴ���*/
void GCSedtDefDrawProc(CTRL_SEDT *edt)
{
	SedtDrawSLEdit(&edt->obj.uda, 0, 0, edt->obj.uda.width, edt->obj.uda.height, COL_BTN_GRADLIGHT, COL_BTN_BORDER, edt->text, COL_TEXT_DARK);	/*���ư�ť*/
}

/*���õ��б༭���ı�*/
void GCSedtSetText(CTRL_SEDT *edt, const char *text)
{
	if (text)
	{
		strncpy(edt->text, text, SEDT_TXT_LEN - 1);
		edt->text[SEDT_TXT_LEN - 1] = 0;
	}
	else
		edt->text[0] = 0;
	GCGobjDraw(&edt->obj);
}

/*	gclient.c for ulios graphical user interface
	作者：孙亮
	功能：GUI客户端功能库实现
	最后修改日期：2011-08-15
*/

#include "gclient.h"
#include "malloc.h"
#include "../fs/fsapi.h"

/**********绘图功能实现**********/

const BYTE *GCfont;
DWORD GCwidth, GCheight;
DWORD GCCharWidth, GCCharHeight;
THREAD_ID GCGuiPtid, GCFontPtid;

/*初始化GC库*/
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

/*撤销GC库*/
void GCrelease()
{
	DWORD data[MSG_DATA_LEN];

	if (GCfont)
	{
		KUnmapProcAddr((void*)GCfont, data);
		GCfont = NULL;
	}
}

/*设置绘图区域的参数和缓存,第一次设置无根绘图区时先清空uda->vbuf*/
long GCSetArea(UDI_AREA *uda, DWORD width, DWORD height, const UDI_AREA *par, long x, long y)
{
	if (par && par != uda)	/*具有根缓存*/
	{
		if (x < 0 || x + (long)width > (long)par->width || y < 0 || y + (long)height > (long)par->height)
			return GC_ERR_LOCATION;
		uda->vbuf = par->vbuf + x + y * par->root->width;
		uda->root = par->root;
	}
	else	/*重新申请缓存*/
	{
		DWORD *p;
		if ((uda->vbuf = (DWORD*)realloc(uda->vbuf, width * height * sizeof(DWORD))) == NULL)
			return GC_ERR_OUT_OF_MEM;
		for (p = uda->vbuf + width * height - 1; p >= uda->vbuf; p -= 512)	/*填充GUI共享区域,重要,因系统可能升级到2K页面,故地址递减值为2KB*/
			*p = 0;
		*uda->vbuf = 0;
		uda->root = uda;
	}
	uda->width = width;
	uda->height = height;
	return NO_ERROR;
}

/*回收绘图区域缓存*/
void GCFreeArea(UDI_AREA *uda)
{
	if (uda->root == uda && uda->vbuf)
	{
		free(uda->vbuf);
		uda->vbuf = NULL;
	}
}

/*画点*/
long GCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c)
{
	if (x >= uda->width || y >= uda->height)
		return GC_ERR_LOCATION;	/*位置越界*/
	uda->vbuf[x + y * uda->root->width] = c;
	return NO_ERROR;
}

/*取点*/
long GCGetPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD *c)
{
	if (x >= uda->width || y >= uda->height)
		return GC_ERR_LOCATION;	/*位置越界*/
	*c = uda->vbuf[x + y * uda->root->width];
	return NO_ERROR;
}

/*贴图*/
long GCPutImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h)
{
	long memw;
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
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

/*去背景色复制图像数据*/
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

/*去背景色贴图*/
long GCPutBCImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h, DWORD bc)
{
	long memw;
	DWORD *tmpvm, vbufw;
	
	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
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

/*截图*/
long GCGetImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h)
{
	long memw;
	DWORD *tmpvm, vbufw;
	
	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
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

/*填充矩形*/
long GCFillRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c)
{
	DWORD *tmpvm, vbufw;

	if (x >= (long)uda->width || x + w <= 0 || y >= (long)uda->height || y + h <= 0)
		return GC_ERR_LOCATION;	/*位置在显存外*/
	if (w <= 0 || h <= 0)
		return GC_ERR_AREASIZE;	/*非法尺寸*/
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

/*无越界判断画点*/
static inline void NCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c)
{
	uda->vbuf[x + y * uda->root->width] = c;
}

#define CS_LEFT		1
#define CS_RIGHT	2
#define CS_TOP		4
#define CS_BOTTOM	8

/*Cohen-Sutherland裁剪算法编码*/
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

/*Cohen-Sutherland算法裁剪Bresenham改进算法画线*/
long GCDrawLine(UDI_AREA *uda, long x1, long y1, long x2, long y2, DWORD c)
{
	DWORD mask, mask1, mask2;
	long dx, dy, dx2, dy2;
	long e, xinc, yinc, half;

	/*裁剪*/
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
		if (x1 == x2)	/*需多写一个中间点*/
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
		if (y1 == y2)	/*需多写一个中间点*/
			NCPutPixel(uda, x1, y1, c);
	}
	return NO_ERROR;
}

/*Bresenham算法画圆*/
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

/*绘制汉字*/
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

/*绘制ASCII字符*/
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

/*绘制字符串*/
long GCDrawStr(UDI_AREA *uda, long x, long y, const char *str, DWORD c)
{
	DWORD hzf;	/*汉字内码首字符标志*/

	for (hzf = 0; *str && x < (long)uda->width; str++)
	{
		if ((BYTE)(*str) > 160)
		{
			if (hzf)	/*显示汉字*/
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
			if (hzf)	/*有未显示的ASCII*/
			{
				GCDrawAscii(uda, x, y, hzf, c);
				x += GCCharWidth;
				hzf = 0;
			}
			GCDrawAscii(uda, x, y, (BYTE)(*str), c);	/*显示当前ASCII*/
			x += GCCharWidth;
		}
	}
	return NO_ERROR;
}

/*加载BMP图像文件*/
long GCLoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height)
{
	BYTE BmpHead[32];
	THREAD_ID FsPtid;
	long bmpw, bmph, file, res;

	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)	/*取得文件系统服务线程*/
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

/**********控件绘图库色彩定义**********/

#define COL_WND_FLAT		0xCCCCCC	// 窗口平面颜色
#define COL_WND_BORDER		0x7B858E	// 窗口边框色
#define COL_CAP_GRADDARK	0x589FCE	// 标题渐变色暗区
#define COL_CAP_GRADLIGHT	0xE1EEF6	// 标题渐变色亮区
#define COL_CAP_NOFCDARK	0xBFBFBF	// 无焦标题渐变色暗区
#define COL_CAP_NOFCLIGHT	0xFBFBFB	// 无焦标题渐变色暗区
#define COL_BTN_BORDER		0x7B858E	// 按钮边框色
#define COL_BTN_GRADDARK	0x89B0CD	// 按钮渐变色暗区
#define COL_BTN_GRADLIGHT	0xD7E9F5	// 按钮渐变色亮区
#define COL_ABTN_GRADDARK	0x6189A8	// 活动按钮渐变色暗区
#define COL_ABTN_GRADLIGHT	0xDDEAF2	// 活动按钮渐变色亮区
#define COL_TEXT_DARK		0x001C30	// 文本暗色
#define COL_TEXT_LIGHT		0xFFE3CF	// 文本亮色

/**********控件绘图库**********/

/*绘制渐变色块*/
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

/**********控件基类**********/

/*初始化CTRL_GOBJ结构*/
long GCGobjInit(CTRL_GOBJ *gobj, const CTRL_ARGS *args, MSGPROC MsgProc, DRAWPROC DrawProc, DWORD pid, CTRL_GOBJ *ParGobj)
{
	long res;

	gobj->uda.vbuf = NULL;
	if ((res = GCSetArea(&gobj->uda, args->width, args->height, &ParGobj->uda, args->x, args->y)) != NO_ERROR)	/*分配绘图区域*/
		return res;
	memcpy32(&gobj->x, &args->x, 4);	// 复制需要的参数
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
			while (CurGobj->nxt)	/*为了简化绘制窗体操作,采用后插法,连接兄弟节点*/
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

/*递归释放窗体树*/
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

/*删除窗体树*/
void GCGobjDelete(CTRL_GOBJ *gobj)
{
	CTRL_GOBJ *ParGobj;

	ParGobj = gobj->par;
	if (ParGobj)	// 解链
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

/*递归绘制窗体树*/
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

/*绘制窗体树*/
void GCGobjDraw(CTRL_GOBJ *gobj)
{
	DrawGobjList(gobj);
	GUIpaint(GCGuiPtid, gobj->gid, 0, 0, gobj->uda.width, gobj->uda.height);	/*绘制完后提交*/
}

/*GUI客户端消息调度*/
long GCDispatchMsg(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_GOBJ *gobj;
	long res;

	if ((data[MSG_ATTR_ID] & MSG_ATTR_MASK) != MSG_ATTR_GUI)	/*抽取合法的GUI消息*/
		return GC_ERR_INVALID_GUIMSG;
	if (data[MSG_RES_ID] != NO_ERROR)	/*分离出错的GUI消息*/
		return GC_ERR_WRONG_GUIMSG;
	gobj = (CTRL_GOBJ*)data[GUIMSG_GOBJ_ID];
	if ((data[MSG_ATTR_ID] & MSG_API_MASK) == GM_CREATE)	/*获取新窗体的GUI服务端对象ID*/
		gobj->gid = data[5];
	res = gobj->MsgProc(ptid, data);	/*调用窗体的消息处理函数*/
	switch (data[MSG_ATTR_ID] & MSG_API_MASK)
	{
	case GM_CREATE:	/*绘制新创建的Gobj根节点*/
		if (gobj->uda.root == &gobj->uda)
			GCGobjDraw(gobj);
		break;
	case GM_DESTROY:	/*销毁窗体*/
		GCGobjDelete(gobj);
		break;
	}
	return res;
}

/**********桌面**********/

/*创建桌面*/
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

/*桌面消息处理*/
long GCDskDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	return NO_ERROR;
}

/*桌面绘制处理*/
void GCDskDefDrawProc(CTRL_DSK *dsk)
{
}

/**********窗口**********/

#define WND_CAP_HEIGHT		20	/*窗口标题栏高度*/
#define WND_BORDER_WIDTH	1	/*窗口边框宽度*/
#define WND_CAPBTN_SIZE		16	/*标题栏按钮大小*/
#define WND_SIZEBTN_SIZE	14	/*缩放按钮大小*/
#define WND_MIN_WIDTH		128	/*窗口最小尺寸*/
#define WND_MIN_HEIGHT		WND_CAP_HEIGHT

/*创建窗口*/
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
	if (NewWnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (NewWnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width - WND_BORDER_WIDTH * 2, NewWnd->obj.uda.height - WND_CAP_HEIGHT - WND_BORDER_WIDTH, &NewWnd->obj.uda, WND_BORDER_WIDTH, WND_CAP_HEIGHT);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width, NewWnd->obj.uda.height - WND_CAP_HEIGHT, &NewWnd->obj.uda, 0, WND_CAP_HEIGHT);	/*创建客户区*/
	}
	else	/*无标题栏*/
	{
		if (NewWnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width - WND_BORDER_WIDTH * 2, NewWnd->obj.uda.height - WND_BORDER_WIDTH * 2, &NewWnd->obj.uda, WND_BORDER_WIDTH, WND_BORDER_WIDTH);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&NewWnd->client, NewWnd->obj.uda.width, NewWnd->obj.uda.height, &NewWnd->obj.uda, 0, 0);	/*创建客户区*/
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

/*窗口关闭按钮处理函数*/
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

/*窗口最大化正常化切换*/
static void WndMaxOrNormal(CTRL_WND *wnd)
{
	if (wnd->obj.uda.width == wnd->MaxWidth && wnd->obj.uda.height == wnd->MaxHeight)
		GCWndSetSize(wnd, wnd->x0, wnd->y0, wnd->width0, wnd->height0);	/*正常化*/
	else
		GCWndSetSize(wnd, 0, 0, wnd->MaxWidth, wnd->MaxHeight);	/*最大化*/
}

/*窗口最小化正常化切换*/
static void WndMinOrNormal(CTRL_WND *wnd)
{
	if (wnd->obj.uda.width == wnd->MinWidth && wnd->obj.uda.height == wnd->MinHeight)
		GCWndSetSize(wnd, wnd->x0, wnd->y0, wnd->width0, wnd->height0);	/*正常化*/
	else
		GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, wnd->MinWidth, wnd->MinHeight);	/*最小化*/
}

/*窗口最大化按钮处理函数*/
static void WndMaxBtnProc(CTRL_BTN *btn)
{
	WndMaxOrNormal((CTRL_WND*)btn->obj.par);
}

/*窗口最小化按钮处理函数*/
static void WndMinBtnProc(CTRL_BTN *btn)
{
	WndMinOrNormal((CTRL_WND*)btn->obj.par);
}

/*窗口缩放按钮消息处理*/
static long WndSizeBtnProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	if ((data[MSG_API_ID] & MSG_API_MASK) == GM_LBUTTONDOWN)
		GUIdrag(GCGuiPtid, ((CTRL_GOBJ*)data[GUIMSG_GOBJ_ID])->par->gid, GM_DRAGMOD_SIZE);
	return GCBtnDefMsgProc(ptid, data);
}

/*窗口消息处理*/
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
			if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*有关闭按钮*/
			{
				args.x = (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->close, &args, wnd->obj.gid, &wnd->obj, "X", WndCloseBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MAXBTN)	/*有最大化按钮*/
			{
				args.x = WND_CAP_HEIGHT + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->max, &args, wnd->obj.gid, &wnd->obj, "[]", WndMaxBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_MINBTN)	/*有最小化按钮*/
			{
				args.x = WND_CAP_HEIGHT * 2 + (WND_CAP_HEIGHT - WND_CAPBTN_SIZE) / 2;
				GCBtnCreate(&wnd->min, &args, wnd->obj.gid, &wnd->obj, "_ ", WndMinBtnProc);
			}
			if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*有缩放按钮*/
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
		if ((wnd->obj.uda.width != wnd->MinWidth || wnd->obj.uda.height != wnd->MinHeight) && (wnd->obj.uda.width != wnd->MaxWidth || wnd->obj.uda.height != wnd->MaxHeight))	/*记录窗口正常化时的大小*/
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
		if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
		{
			if (wnd->obj.style & WND_STATE_TOP)
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*绘制标题栏*/
			else
				FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*绘制无焦标题栏*/
			GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
			if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
				GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*上边框*/
			if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*有关闭按钮*/
				GCBtnDefDrawProc(wnd->close);
			if (wnd->obj.style & WND_STYLE_MAXBTN)	/*有最大化按钮*/
				GCBtnDefDrawProc(wnd->max);
			if (wnd->obj.style & WND_STYLE_MINBTN)	/*有最小化按钮*/
				GCBtnDefDrawProc(wnd->min);
			GUIpaint(GCGuiPtid, wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
		}
		break;
	case GM_DRAG:
		if (data[1] == GM_DRAGMOD_SIZE)
			GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, data[2], data[3]);
		break;
	case GM_LBUTTONDOWN:
		if (wnd->obj.style & WND_STYLE_CAPTION && (data[5] >> 16) < WND_CAP_HEIGHT)	/*单击标题拖动窗口*/
			GUIdrag(GCGuiPtid, wnd->obj.gid, GM_DRAGMOD_MOVE);
		if (!(wnd->obj.style & WND_STATE_TOP))
			GUISetTop(GCGuiPtid, wnd->obj.gid);
		break;
	case GM_LBUTTONDBCLK:
		if ((wnd->obj.style & (WND_STYLE_CAPTION | WND_STYLE_MAXBTN)) == (WND_STYLE_CAPTION | WND_STYLE_MAXBTN) && (data[5] >> 16) < WND_CAP_HEIGHT)	/*双击标题缩放窗口*/
			WndMaxOrNormal(wnd);
		break;
	case GM_CLOSE:
		GUIdestroy(GCGuiPtid, wnd->obj.gid);
		break;
	}
	return NO_ERROR;
}

/*窗口绘制处理*/
void GCWndDefDrawProc(CTRL_WND *wnd)
{
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (wnd->obj.style & WND_STATE_TOP)
			FillGradRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*绘制标题栏*/
		else
			FillGradRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*绘制无焦标题栏*/
		GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
	}
	if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
	{
		GCFillRect(&wnd->obj.uda, 0, 0, wnd->obj.uda.width, WND_BORDER_WIDTH, COL_WND_BORDER);	/*上边框*/
		GCFillRect(&wnd->obj.uda, 0, wnd->obj.uda.height - WND_BORDER_WIDTH, wnd->obj.uda.width, WND_BORDER_WIDTH, COL_WND_BORDER);	/*下边框*/
		GCFillRect(&wnd->obj.uda, 0, WND_BORDER_WIDTH, WND_BORDER_WIDTH, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, COL_WND_BORDER);	/*左边框*/
		GCFillRect(&wnd->obj.uda, wnd->obj.uda.width - WND_BORDER_WIDTH, WND_BORDER_WIDTH, WND_BORDER_WIDTH, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, COL_WND_BORDER);	/*右边框*/
	}
	GCFillRect(&wnd->client, 0, 0, wnd->client.width, wnd->client.height, COL_WND_FLAT);
}

/*设置窗口标题文本*/
void GCWndSetCaption(CTRL_WND *wnd, const char *caption)
{
	if (caption)
	{
		strncpy(wnd->caption, caption, WND_CAP_LEN - 1);
		wnd->caption[WND_CAP_LEN - 1] = 0;
	}
	else
		wnd->caption[0] = 0;
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (wnd->obj.style & WND_STATE_TOP)
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_GRADLIGHT, COL_CAP_GRADDARK);	/*绘制标题栏*/
		else
			FillGradRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT, COL_CAP_NOFCLIGHT, COL_CAP_NOFCDARK);	/*绘制无焦标题栏*/
		GCDrawStr(&wnd->obj.uda, ((long)wnd->obj.uda.width - (long)strlen(wnd->caption) * (long)GCCharWidth) / 2, (WND_CAP_HEIGHT - (long)GCCharHeight) / 2, wnd->caption, COL_TEXT_DARK);
		if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCFillRect(&wnd->obj.uda, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_BORDER_WIDTH, COL_WND_BORDER);	/*上边框*/
		if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*有关闭按钮*/
			GCBtnDefDrawProc(wnd->close);
		if (wnd->obj.style & WND_STYLE_MAXBTN)	/*有最大化按钮*/
			GCBtnDefDrawProc(wnd->max);
		if (wnd->obj.style & WND_STYLE_MINBTN)	/*有最小化按钮*/
			GCBtnDefDrawProc(wnd->min);
		GUIpaint(GCGuiPtid, wnd->obj.gid, WND_BORDER_WIDTH, 0, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, WND_CAP_HEIGHT);
	}
}

/*窗口检查最小最大尺寸*/
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

/*设置窗口位置大小*/
void GCWndSetSize(CTRL_WND *wnd, long x, long y, DWORD width, DWORD height)
{
	WndChkMinMax(wnd);
	if ((long)width < 0)
		width = 0;
	if ((long)height < 0)
		height = 0;
	if ((width < wnd->MinWidth || width > wnd->MaxWidth) && (height < wnd->MinHeight || height > wnd->MaxHeight))
		return;
	if (width < wnd->MinWidth)	/*修正宽度*/
		width = wnd->MinWidth;
	else if (width > wnd->MaxWidth)
		width = wnd->MaxWidth;
	if (height < wnd->MinHeight)	/*修正高度*/
		height = wnd->MinHeight;
	else if (height > wnd->MaxHeight)
		height = wnd->MaxHeight;
	if (GCSetArea(&wnd->obj.uda, width, height, &wnd->obj.par->uda, x, y) != NO_ERROR)
		return;
	wnd->obj.x = x;
	wnd->obj.y = y;
	if (wnd->obj.style & WND_STYLE_CAPTION)	/*有标题栏*/
	{
		if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, wnd->obj.uda.height - WND_CAP_HEIGHT - WND_BORDER_WIDTH, &wnd->obj.uda, WND_BORDER_WIDTH, WND_CAP_HEIGHT);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width, wnd->obj.uda.height - WND_CAP_HEIGHT, &wnd->obj.uda, 0, WND_CAP_HEIGHT);
	}
	else	/*无标题栏*/
	{
		if (wnd->obj.style & WND_STYLE_BORDER)	/*有边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width - WND_BORDER_WIDTH * 2, wnd->obj.uda.height - WND_BORDER_WIDTH * 2, &wnd->obj.uda, WND_BORDER_WIDTH, WND_BORDER_WIDTH);	/*创建客户区*/
		else	/*无边框*/
			GCSetArea(&wnd->client, wnd->obj.uda.width, wnd->obj.uda.height, &wnd->obj.uda, 0, 0);
	}
	GCWndDefDrawProc(wnd);
	if (wnd->obj.style & WND_STYLE_CLOSEBTN)	/*有关闭按钮*/
	{
		GCSetArea(&wnd->close->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, wnd->close->obj.x, wnd->close->obj.y);
		GCBtnDefDrawProc(wnd->close);
	}
	if (wnd->obj.style & WND_STYLE_MAXBTN)	/*有最大化按钮*/
	{
		GCSetArea(&wnd->max->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, wnd->max->obj.x, wnd->max->obj.y);
		GCBtnDefDrawProc(wnd->max);
	}
	if (wnd->obj.style & WND_STYLE_MINBTN)	/*有最小化按钮*/
	{
		GCSetArea(&wnd->min->obj.uda, WND_CAPBTN_SIZE, WND_CAPBTN_SIZE, &wnd->obj.uda, wnd->min->obj.x, wnd->min->obj.y);
		GCBtnDefDrawProc(wnd->min);
	}
	if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*有缩放按钮*/
	{
		wnd->size->obj.x = wnd->obj.uda.width - WND_SIZEBTN_SIZE;
		wnd->size->obj.y = wnd->obj.uda.height - WND_SIZEBTN_SIZE;
		GCSetArea(&wnd->size->obj.uda, WND_SIZEBTN_SIZE, WND_SIZEBTN_SIZE, &wnd->obj.uda, wnd->size->obj.x, wnd->size->obj.y);
		GCBtnDefDrawProc(wnd->size);
	}
	GUIsize(GCGuiPtid, wnd->obj.gid, wnd->obj.uda.vbuf, x, y, width, height);	/*重设窗口大小*/
	if (wnd->obj.style & WND_STYLE_SIZEBTN)	/*有缩放按钮*/
		GUImove(GCGuiPtid, wnd->size->obj.gid, wnd->size->obj.x, wnd->size->obj.y);
}

/*取得窗口客户区位置*/
void GCWndGetClientLoca(CTRL_WND *wnd, long *x, long *y)
{
	DWORD PixCou;

	PixCou = wnd->client.vbuf - wnd->client.root->vbuf;
	*x = PixCou % wnd->client.root->width;
	*y = PixCou / wnd->client.root->width;
}

/**********按钮**********/

/*创建按钮*/
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

/*按钮绘制*/
static void BtnDrawButton(UDI_AREA *uda, long x, long y, long w, long h, DWORD c1, DWORD c2, DWORD bc, const char *text, DWORD tc)
{
	FillGradRect(uda, x + 1, y + 1, w - 2, h - 2, c1, c2);
	/*绘制圆角矩形*/
	GCFillRect(uda, x + 2, y, w - 4, 1, bc);	/*上边框*/
	GCFillRect(uda, x + 2, y + h - 1, w - 4, 1, bc);	/*下边框*/
	GCFillRect(uda, x, y + 2, 1, h - 4, bc);	/*左边框*/
	GCFillRect(uda, x + w - 1, y + 2, 1, h - 4, bc);	/*右边框*/
	GCPutPixel(uda, x + 1, y + 1, bc);	/*左上角*/
	GCPutPixel(uda, x + w - 2, y + 1, bc);	/*右上角*/
	GCPutPixel(uda, x + 1, y + h - 2, bc);	/*左下角*/
	GCPutPixel(uda, x + w - 2, y + h - 2, bc);	/*右下角*/
	GCDrawStr(uda, x + (w - (long)strlen(text) * (long)GCCharWidth) / 2, y + (h - (long)GCCharHeight) / 2, text, tc);
}

/*按钮消息处理*/
long GCBtnDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_BTN *btn = (CTRL_BTN*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_LBUTTONUP:	/*鼠标抬起状态*/
		if (btn->isPressDown)
		{
			btn->isPressDown = FALSE;
			if (btn->PressProc)
				btn->PressProc(btn);	/*执行按钮功能*/
		}	/*继续绘制按钮*/
	case GM_MOUSEENTER:	/*鼠标进入状态*/
		BtnDrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_ABTN_GRADLIGHT, COL_ABTN_GRADDARK, COL_BTN_BORDER, btn->text, COL_TEXT_DARK);	/*绘制按钮*/
		GUIpaint(GCGuiPtid, btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		break;
	case GM_MOUSELEAVE:	/*鼠标离开状态*/
		btn->isPressDown = FALSE;
		GCBtnDefDrawProc(btn);
		GUIpaint(GCGuiPtid, btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		break;
	case GM_LBUTTONDOWN:	/*鼠标按下状态*/
		btn->isPressDown = TRUE;
		BtnDrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_ABTN_GRADDARK, COL_ABTN_GRADLIGHT, COL_BTN_BORDER, btn->text, COL_TEXT_LIGHT);	/*绘制按钮*/
		GUIpaint(GCGuiPtid, btn->obj.gid, 0, 0, btn->obj.uda.width, btn->obj.uda.height);
		break;
	}
	return NO_ERROR;
}

/*按钮绘制处理*/
void GCBtnDefDrawProc(CTRL_BTN *btn)
{
	BtnDrawButton(&btn->obj.uda, 0, 0, btn->obj.uda.width, btn->obj.uda.height, COL_BTN_GRADLIGHT, COL_BTN_GRADDARK, COL_BTN_BORDER, btn->text, COL_TEXT_DARK);	/*绘制按钮*/
}

/*设置按钮文本*/
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

/**********静态文本框**********/

/*创建静态文本框*/
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

/*静态文本框消息处理*/
long GCTxtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	return NO_ERROR;
}

/*静态文本框绘制处理*/
void GCTxtDefDrawProc(CTRL_TXT *txt)
{
	GCDrawStr(&txt->obj.uda, 0, ((long)txt->obj.uda.height - (long)GCCharHeight) / 2, txt->text, COL_TEXT_DARK);
}

/*设置文本框文本*/
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

/**********单行编辑框**********/

/*创建单行编辑框*/
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

/*单行编辑框文本区绘制*/
static void SedtDrawText(CTRL_SEDT *edt)
{
	GCFillRect(&edt->obj.uda, 1, (edt->obj.uda.height - (long)GCCharHeight) / 2, edt->obj.uda.width - 2, GCCharHeight, COL_BTN_GRADLIGHT);	/*底色*/
	GCDrawStr(&edt->obj.uda, 1, (edt->obj.uda.height - (long)GCCharHeight) / 2, edt->FstC, COL_TEXT_DARK);
	if (edt->obj.style & SEDT_STATE_FOCUS)	/*获得焦点时才显示光标*/
		GCFillRect(&edt->obj.uda, 1 + (edt->CurC - edt->FstC) * GCCharWidth, (edt->obj.uda.height - (long)GCCharHeight) / 2, 2, GCCharHeight, COL_TEXT_DARK);	/*光标*/
	GUIpaint(GCGuiPtid, edt->obj.gid, 1, (edt->obj.uda.height - (long)GCCharHeight) / 2, edt->obj.uda.width - 2, GCCharHeight);
}

/*单行编辑框消息处理*/
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
	case GM_LBUTTONDOWN:	/*鼠标按下*/
		if (!(edt->obj.style & SEDT_STATE_FOCUS))
			GUISetFocus(GCGuiPtid, edt->obj.gid);
		{
			DWORD len;
			len = ((data[5] & 0xFFFF) - 1 + GCCharWidth / 2) / GCCharWidth;
			for (edt->CurC = edt->FstC; *edt->CurC; edt->CurC++, len--)
				if (len == 0)
					break;
		}
		SedtDrawText(edt);
		break;
	case GM_KEY:	/*按键*/
		switch (data[1] & 0xFF)
		{
		case '\b':	/*退格键*/
			if (edt->CurC > edt->text)	/*光标前还有字符*/
			{
				strcpy(edt->CurC - 1, edt->CurC);	/*移动字符串*/
				edt->CurC--;	/*移动光标*/
				if (edt->FstC > edt->text)
					edt->FstC--;	/*移动首字符*/
			}
			break;
		case '\r':	/*回车键*/
			if (edt->EnterProc)
				edt->EnterProc(edt);	/*执行回车动作*/
			break;
		case '\0':	/*无可显符号按键*/
			switch ((data[1] >> 8) & 0xFF)
			{
			case 0x4B:	/*左箭头*/
				if (edt->CurC > edt->text)
				{
					edt->CurC--;	/*移动光标*/
					if (edt->FstC > edt->CurC)
						edt->FstC = edt->CurC;	/*移动首字符*/
				}
				break;
			case 0x4D:	/*右箭头*/
				if (*edt->CurC != '\0')
				{
					DWORD len;
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
					edt->CurC++;	/*移动光标*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*移动首字符*/
				}
				break;
			case 0x47:	/*HOME键*/
				edt->CurC = edt->FstC = edt->text;
				break;
			case 0x4F:	/*END键*/
				{
					DWORD len;
					len = strlen(edt->text);
					edt->CurC = edt->text + len;	/*移动光标*/
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*移动首字符*/
				}
				break;
			case 0x53:	/*DELETE键*/
				if (*edt->CurC != '\0')
					strcpy(edt->CurC, edt->CurC + 1);	/*移动字符串*/
				if (strlen(edt->FstC) < (edt->obj.uda.width - 4) / GCCharWidth && edt->FstC > edt->text)
					edt->FstC--;	/*移动首字符*/
				break;
			}
			break;
		default:	/*其它字符插入缓冲*/
			{
				DWORD len;
				len = strlen(edt->text);
				if (len < SEDT_TXT_LEN - 1)	/*缓冲没满*/
				{
					char *strp;
					strp = edt->text + len + 1;
					do
						*strp = *(strp - 1);
					while (--strp > edt->CurC);	/*反向搬移字符*/
					*edt->CurC = (char)(BYTE)data[1];
					edt->CurC++;	/*移动光标*/
					len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
					if (edt->FstC < edt->CurC - len)
						edt->FstC = edt->CurC - len;	/*移动首字符*/
				}
			}
		}
		SedtDrawText(edt);
		break;
	}
	return NO_ERROR;
}

/*单行编辑框绘制处理*/
void GCSedtDefDrawProc(CTRL_SEDT *edt)
{
	GCFillRect(&edt->obj.uda, 0, 0, edt->obj.uda.width, 1, COL_BTN_BORDER);	/*上边框*/
	GCFillRect(&edt->obj.uda, 0, edt->obj.uda.height - 1, edt->obj.uda.width, 1, COL_BTN_BORDER);	/*下边框*/
	GCFillRect(&edt->obj.uda, 0, 1, 1, edt->obj.uda.height - 2, COL_BTN_BORDER);	/*左边框*/
	GCFillRect(&edt->obj.uda, edt->obj.uda.width - 1, 1, 1, edt->obj.uda.height - 2, COL_BTN_BORDER);	/*右边框*/
	GCFillRect(&edt->obj.uda, 1, 1, edt->obj.uda.width - 2, edt->obj.uda.height - 2, (edt->obj.style & SEDT_STYLE_RDONLY) ? COL_BTN_GRADDARK : COL_BTN_GRADLIGHT);	/*底色*/
	GCDrawStr(&edt->obj.uda, 1, (edt->obj.uda.height - (long)GCCharHeight) / 2, edt->FstC, COL_TEXT_DARK);	/*文本*/
	if (edt->obj.style & SEDT_STATE_FOCUS)	/*获得焦点时才显示光标*/
		GCFillRect(&edt->obj.uda, 1 + (edt->CurC - edt->FstC) * GCCharWidth, (edt->obj.uda.height - (long)GCCharHeight) / 2, 2, GCCharHeight, COL_TEXT_DARK);	/*光标*/
}

/*设置单行编辑框文本*/
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
	edt->CurC = edt->text + len;	/*移动光标*/
	len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
	if (edt->FstC < edt->CurC - len)
		edt->FstC = edt->CurC - len;	/*移动首字符*/
	GCGobjDraw(&edt->obj);
}

/*追加单行编辑框文本*/
void GCSedtAddText(CTRL_SEDT *edt, const char *text)
{
	DWORD len;

	if (text == NULL)
		return;
	len = strlen(edt->text);
	strncpy(edt->text + len, text, SEDT_TXT_LEN - 1 - len);	/*追加字符串*/
	edt->text[SEDT_TXT_LEN - 1] = 0;
	len += strlen(edt->text + len);
	edt->CurC = edt->text + len;	/*移动光标*/
	len = (edt->obj.uda.width - 4) / GCCharWidth;	/*计算框内可显示字符数*/
	if (edt->FstC < edt->CurC - len)
		edt->FstC = edt->CurC - len;	/*移动首字符*/
	GCGobjDraw(&edt->obj);
}

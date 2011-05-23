/*	guirect.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面GUI剪切矩形基本操作
	最后修改日期：2010-10-05
*/

#include "gui.h"

CLIPRECT ClipRectt[CLIPRECTT_LEN], *FstClipRect;	/*剪切矩形管理表指针*/

/*分配剪切矩形结构*/
static CLIPRECT *AllocClipRect()
{
	CLIPRECT *clip;

	if (FstClipRect == NULL)
		return NULL;
	FstClipRect = (clip = FstClipRect)->nxt;
	return clip;
}

/*释放剪切矩形结构*/
static void FreeClipRect(CLIPRECT *clip)
{
	clip->nxt = FstClipRect;
	FstClipRect = clip;
}

/*释放剪切矩形链*/
static inline long FreeClipList(CLIPRECT *clip)
{
	if (clip)
	{
		CLIPRECT *CurClip;

		CurClip = clip;	/*查找链尾节点*/
		while (CurClip->nxt)
			CurClip = CurClip->nxt;
		CurClip->nxt = FstClipRect;
		FstClipRect = clip;
		return NO_ERROR;
	}
	else
		return GUI_ERR_NOCHG_CLIPRECT;
}

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

/*掩盖窗体的矩形外部*/
long CoverRectExter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	CLIPRECT *PreClip, *CurClip;

	if (xpos <= 0 && xend >= gobj->rect.xend - gobj->rect.xpos &&	/*完全覆盖区域*/
		ypos <= 0 && yend >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	for (PreClip = NULL, CurClip = gobj->ClipList; CurClip;)	/*处理父窗体*/
	{
		if (xend > CurClip->rect.xpos && xpos < CurClip->rect.xend &&
			yend > CurClip->rect.ypos && ypos < CurClip->rect.yend)	/*有重叠区域*/
		{
			if (CurClip->rect.xpos < xpos)
				CurClip->rect.xpos = xpos;
			if (CurClip->rect.ypos < ypos)
				CurClip->rect.ypos = ypos;
			if (CurClip->rect.xend > xend)
				CurClip->rect.xend = xend;
			if (CurClip->rect.yend > yend)
				CurClip->rect.yend = yend;
			CurClip = (PreClip = CurClip)->nxt;	/*继续处理后续节点*/
		}
		else
		{
			/*删除原剪切矩形*/
			if (PreClip)
			{
				PreClip->nxt = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = PreClip->nxt;
			}
			else
			{
				gobj->ClipList = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = gobj->ClipList;
			}
		}
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*递归处理子窗体*/
		CoverRectExter(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos);

	return NO_ERROR;
}

/*掩盖窗体的矩形内部*/
long CoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	CLIPRECT *PreClip, *CurClip, *NewClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*没有重叠区域*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	for (PreClip = NULL, CurClip = gobj->ClipList; CurClip;)	/*处理父窗体*/
	{
		if (xend > CurClip->rect.xpos && xpos < CurClip->rect.xend &&
			yend > CurClip->rect.ypos && ypos < CurClip->rect.yend)	/*有重叠区域*/
		{
			/*左边的不重叠块(不包括左上和左下)*/
			if (xpos > CurClip->rect.xpos)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = CurClip->rect.xpos;
				NewClip->rect.ypos = MAX(ypos, CurClip->rect.ypos);
				NewClip->rect.xend = xpos;
				NewClip->rect.yend = MIN(yend, CurClip->rect.yend);
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*右边的不重叠块(不包括右上和右下)*/
			if (xend < CurClip->rect.xend)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = xend;
				NewClip->rect.ypos = MAX(ypos, CurClip->rect.ypos);
				NewClip->rect.xend = CurClip->rect.xend;
				NewClip->rect.yend = MIN(yend, CurClip->rect.yend);
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*上边的不重叠块(包括左上和右上)*/
			if (ypos > CurClip->rect.ypos)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = CurClip->rect.xpos;
				NewClip->rect.ypos = CurClip->rect.ypos;
				NewClip->rect.xend = CurClip->rect.xend;
				NewClip->rect.yend = ypos;
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*下边的不重叠块(包括左上和右上)*/
			if (yend < CurClip->rect.yend)
			{
				NewClip = AllocClipRect();
				NewClip->rect.xpos = CurClip->rect.xpos;
				NewClip->rect.ypos = yend;
				NewClip->rect.xend = CurClip->rect.xend;
				NewClip->rect.yend = CurClip->rect.yend;
				NewClip->nxt = CurClip;
				if (PreClip)
					PreClip->nxt = NewClip;
				else
					gobj->ClipList = NewClip;
				PreClip = NewClip;
			}
			/*删除原剪切矩形*/
			if (PreClip)
			{
				PreClip->nxt = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = PreClip->nxt;
			}
			else
			{
				gobj->ClipList = CurClip->nxt;
				FreeClipRect(CurClip);
				CurClip = gobj->ClipList;
			}
		}
		else
			CurClip = (PreClip = CurClip)->nxt;	/*继续处理后续节点*/
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*递归处理子窗体*/
		CoverRectInter(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos);

	return NO_ERROR;
}

/*显露窗体的矩形内部*/
long DiscoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	GOBJ_DESC *ChlGobj;
	CLIPRECT *NewClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*没有重叠区域*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	if (xpos < 0)	/*处理父窗体*/
		xpos = 0;
	if (ypos < 0)
		ypos = 0;
	if (xend > gobj->rect.xend - gobj->rect.xpos)
		xend = gobj->rect.xend - gobj->rect.xpos;
	if (yend > gobj->rect.yend - gobj->rect.ypos)
		yend = gobj->rect.yend - gobj->rect.ypos;
	NewClip = AllocClipRect();
	NewClip->rect.xpos = xpos;
	NewClip->rect.ypos = ypos;
	NewClip->rect.xend = xend;
	NewClip->rect.yend = yend;
	NewClip->nxt = gobj->ClipList;
	gobj->ClipList = NewClip;
	if (gobj->chl == NULL)
		return NO_ERROR;
	ChlGobj = gobj->chl;
	while (ChlGobj->nxt)	/*查找最底层的窗体*/
		 ChlGobj = ChlGobj->nxt;
	while (ChlGobj)	/*递归处理子窗体*/
	{
		CoverRectInter(gobj, ChlGobj->rect.xpos, ChlGobj->rect.ypos, ChlGobj->rect.xend, ChlGobj->rect.yend);
		DiscoverRectInter(ChlGobj, xpos - ChlGobj->rect.xpos, ypos - ChlGobj->rect.ypos, xend - ChlGobj->rect.xpos, yend - ChlGobj->rect.ypos);
		ChlGobj = ChlGobj->pre;
	}

	return NO_ERROR;
}

/*被祖父和祖伯父窗体覆盖*/
long CoverRectByPar(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ParGobj, *PreGobj;
	long xpos, ypos;

	ypos = xpos = 0;
	ParGobj = gobj;
	for (;;)
	{
		xpos -= ParGobj->rect.xpos;
		ypos -= ParGobj->rect.ypos;
		for (PreGobj = ParGobj->pre; PreGobj; PreGobj = PreGobj->pre)	/*被祖伯父窗体覆盖内部*/
			CoverRectInter(gobj, xpos + PreGobj->rect.xpos, ypos + PreGobj->rect.ypos, xpos + PreGobj->rect.xend, ypos + PreGobj->rect.yend);
		if (ParGobj->par == NULL)
			break;
		ParGobj = ParGobj->par;	/*被祖父窗体覆盖外部*/
		CoverRectExter(gobj, xpos, ypos, xpos + ParGobj->rect.xend - ParGobj->rect.xpos, ypos + ParGobj->rect.yend - ParGobj->rect.ypos);
	}

	return NO_ERROR;
}

/*强行删除剪切矩形链表*/
long DeleteClipList(GOBJ_DESC *gobj)
{
	BOOL isNoChg;

	isNoChg = TRUE;
	if (FreeClipList(gobj->ClipList) == NO_ERROR)
		isNoChg = FALSE;
	gobj->ClipList = NULL;
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*递归处理子窗体*/
		if (DeleteClipList(gobj) == NO_ERROR)
			isNoChg = FALSE;
	if (isNoChg)
		return GUI_ERR_NOCHG_CLIPRECT;

	return NO_ERROR;
}

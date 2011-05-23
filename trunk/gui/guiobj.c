/*	guiobj.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面GUI对象(窗体)基本操作
	最后修改日期：2010-10-05
*/

#include "gui.h"
#include "../lib/gdi.h"

GOBJ_DESC gobjt[GOBJT_LEN], *FstGobj;		/*窗体描述符管理表指针*/

/*分配窗体结构*/
static GOBJ_DESC *AllocGobj()
{
	GOBJ_DESC *gobj;
	
	if (FstGobj == NULL)
		return NULL;
	FstGobj = (gobj = FstGobj)->nxt;
	return gobj;
}

/*释放窗体结构*/
static void FreeGobj(GOBJ_DESC *gobj)
{
	gobj->attr = INVALID;
	gobj->nxt = FstGobj;
	FstGobj = gobj;
}

/*递归释放窗体链*/
static void FreeGobjList(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ChlGobj;

	ChlGobj = gobj->chl;
	FreeGobj(gobj);
	while (ChlGobj)
	{
		gobj = ChlGobj->nxt;
		FreeGobjList(ChlGobj);
		ChlGobj = gobj;
	}
}

/*取得窗体的绝对坐标*/
static void GetGobjPos(GOBJ_DESC *gobj, long *GobjXpos, long *GobjYpos)
{
	for (*GobjYpos = *GobjXpos = 0; gobj; gobj = gobj->par)
	{
		*GobjXpos += gobj->rect.xpos;
		*GobjYpos += gobj->rect.ypos;
	}
}

/*根据绝对坐标查找窗体*/
GOBJ_DESC *FindGobj(long *GobjXpos, long *GobjYpos)
{
	GOBJ_DESC *gobj, *CurGobj;

	gobj = NULL;
	CurGobj = &gobjt[0];
	while (CurGobj)
	{
		if (*GobjXpos >= CurGobj->rect.xpos && *GobjXpos < CurGobj->rect.xend && *GobjYpos >= CurGobj->rect.ypos && *GobjYpos < CurGobj->rect.yend)
		{
			*GobjXpos -= CurGobj->rect.xpos;
			*GobjYpos -= CurGobj->rect.ypos;
			gobj = CurGobj;
			CurGobj = CurGobj->chl;
		}
		else
			CurGobj = CurGobj->nxt;
	}
	return gobj;
}

/*绘制窗体*/
long DrawGobj(GOBJ_DESC *gobj, long xpos, long ypos)
{
	CLIPRECT *clip;

	if (gobj->vbuf)
		for (clip = gobj->ClipList; clip; clip = clip->nxt)
		{
			BOOL isRefreshMouse;
			isRefreshMouse = CheckMousePos(xpos + clip->rect.xpos, ypos + clip->rect.ypos, xpos + clip->rect.xend, ypos + clip->rect.yend);
			if (isRefreshMouse)
				HidMouse();
			GDIBitBlt(xpos + clip->rect.xpos, ypos + clip->rect.ypos, gobj->vbuf, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos, clip->rect.xpos, clip->rect.ypos, clip->rect.xend - clip->rect.xpos, clip->rect.yend - clip->rect.ypos);
			if (isRefreshMouse)
				ShowMouse();
		}
	else
		for (clip = gobj->ClipList; clip; clip = clip->nxt)
		{
			BOOL isRefreshMouse;
			isRefreshMouse = CheckMousePos(xpos + clip->rect.xpos, ypos + clip->rect.ypos, xpos + clip->rect.xend, ypos + clip->rect.yend);
			if (isRefreshMouse)
				HidMouse();
			GDIFillRect(xpos + clip->rect.xpos, ypos + clip->rect.ypos, clip->rect.xend - clip->rect.xpos, clip->rect.yend - clip->rect.ypos, 0);
			if (isRefreshMouse)
				ShowMouse();
		}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)
		DrawGobj(gobj, xpos + gobj->rect.xpos, ypos + gobj->rect.ypos);
	return NO_ERROR;
}

/*创建主桌面*/
long CreateDesktop(THREAD_ID ptid, DWORD attr, long width, long height, DWORD *DesktopPic)
{
	GOBJ_DESC *gobj;
	long xpos, ypos;

	if ((gobj = AllocGobj()) == NULL)
		return GUI_ERR_HAVENO_MEMORY;
	memset32(gobj, 0, sizeof(GOBJ_DESC) / sizeof(DWORD));
	gobj->ptid = ptid;
	gobj->attr = attr;
	gobj->rect.xpos = 0;
	gobj->rect.ypos = 0;
	gobj->rect.xend = width;
	gobj->rect.yend = height;
	gobj->vbuf = DesktopPic;
	DiscoverRectInter(gobj, 0, 0, width, height);	/*为桌面创建最初的剪切矩形*/
	GetGobjPos(gobj, &xpos, &ypos);
	DrawGobj(gobj, xpos, ypos);

	return NO_ERROR;
}

/*创建窗体*/
long CreateGobj(THREAD_ID ptid, DWORD attr, DWORD pid, long xpos, long ypos, long width, long height, DWORD *id)
{
	GOBJ_DESC *gobj;
	GOBJ_DESC *ParGobj;	/*父节点*/
	long res;

	if (pid >= GOBJT_LEN)	/*检查父节点ID*/
		return GUI_ERR_WRONG_GOBJID;
	ParGobj = &gobjt[pid];
	if (ParGobj->attr == INVALID)	/*无效窗口*/
		return GUI_ERR_WRONG_GOBJID;
	if (width <= 0 || height <= 0)
		return GUI_ERR_WRONG_WNDSIZE;
	if ((gobj = AllocGobj()) == NULL)
		return GUI_ERR_HAVENO_MEMORY;

	memset32(gobj, 0, sizeof(GOBJ_DESC) / sizeof(DWORD));	/*初始化新窗体*/
	gobj->ptid = ptid;
	gobj->attr = attr;
	gobj->rect.xpos = xpos;
	gobj->rect.ypos = ypos;
	gobj->rect.xend = xpos + width;
	gobj->rect.yend = ypos + height;
	gobj->nxt = ParGobj->chl;
	gobj->par = ParGobj;
	res = CoverRectInter(ParGobj, gobj->rect.xpos, gobj->rect.ypos, gobj->rect.xend, gobj->rect.yend);	/*处理父窗体被新窗体掩盖的区域*/
	if (ParGobj->chl)	/*连接兄弟节点*/
		ParGobj->chl->pre = gobj;
	ParGobj->chl = gobj;
	if (res == NO_ERROR)
	{
		DiscoverRectInter(gobj, 0, 0, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos);	/*显露新窗体*/
		CoverRectByPar(gobj);	/*覆盖窗体*/
		GetGobjPos(gobj, &xpos, &ypos);
		DrawGobj(gobj, xpos, ypos);
	}
	*id = gobj - gobjt;

	return NO_ERROR;
}

/*删除窗体*/
long DeleteGobj(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ParGobj;	/*父节点*/
	long xpos, ypos, res;

	res = DeleteClipList(gobj);
	ParGobj = gobj->par;
	if (ParGobj->chl == gobj)
		ParGobj->chl = gobj->nxt;
	if (gobj->pre)
		gobj->pre->nxt = gobj->nxt;
	if (gobj->nxt)
		gobj->nxt->pre = gobj->pre;
	FreeGobjList(gobj);
	if (res == NO_ERROR)
	{
		DeleteClipList(ParGobj);
		DiscoverRectInter(ParGobj, 0, 0, ParGobj->rect.xend - ParGobj->rect.xpos, ParGobj->rect.yend - ParGobj->rect.ypos);	/*显露新窗体*/
		CoverRectByPar(ParGobj);	/*覆盖窗体*/
		GetGobjPos(ParGobj, &xpos, &ypos);
		DrawGobj(ParGobj, xpos, ypos);
	}

	return NO_ERROR;
}

/*设置窗体的位置大小*/
long MoveGobj(GOBJ_DESC *gobj, long xpos, long ypos, long width, long height)
{
	GOBJ_DESC *ParGobj;	/*父节点*/
	long res;

	if (width <= 0 || height <= 0)
		return GUI_ERR_WRONG_WNDSIZE;

	ParGobj = gobj->par;
	res = DeleteClipList(ParGobj);
	gobj->rect.xpos = xpos;
	gobj->rect.ypos = ypos;
	gobj->rect.xend = xpos + width;
	gobj->rect.yend = ypos + height;
	if (res == NO_ERROR)
	{
		DiscoverRectInter(ParGobj, 0, 0, ParGobj->rect.xend - ParGobj->rect.xpos, ParGobj->rect.yend - ParGobj->rect.ypos);	/*显露新窗体*/
		CoverRectByPar(ParGobj);	/*覆盖窗体*/
		GetGobjPos(ParGobj, &xpos, &ypos);
		DrawGobj(ParGobj, xpos, ypos);
	}

	return NO_ERROR;
}

/*设置窗体为活动*/
long ActiveGobj(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ParGobj;	/*父节点*/
	long xpos, ypos;

	if (gobj->pre == NULL)
		return GUI_ERR_NOCHG_CLIPRECT;

	for (ParGobj = gobj->pre; ParGobj; ParGobj = ParGobj->pre)
		CoverRectInter(ParGobj, gobj->rect.xpos - ParGobj->rect.xpos, gobj->rect.ypos - ParGobj->rect.ypos, gobj->rect.xend - ParGobj->rect.xpos, gobj->rect.yend - ParGobj->rect.ypos);
	ParGobj = gobj->par;
	if (gobj->pre)
		gobj->pre->nxt = gobj->nxt;
	if (gobj->nxt)
		gobj->nxt->pre = gobj->pre;
	ParGobj->chl->pre = gobj;
	gobj->nxt = ParGobj->chl;
	gobj->pre = NULL;
	ParGobj->chl = gobj;
	DeleteClipList(gobj);
	DiscoverRectInter(gobj, 0, 0, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos);	/*显露新窗体*/
	CoverRectByPar(gobj);	/*覆盖窗体*/
	GetGobjPos(gobj, &xpos, &ypos);
	DrawGobj(gobj, xpos, ypos);

	return NO_ERROR;
}

/*	guiobj.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面GUI对象(窗体)基本操作
	最后修改日期：2010-10-05
*/

#include "gui.h"

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
	while (ChlGobj)
	{
		GOBJ_DESC *TmpGobj;
		TmpGobj = ChlGobj->nxt;
		FreeGobjList(ChlGobj);
		ChlGobj = TmpGobj;
	}
	if (gobj->vbuf)
	{
		DWORD data[MSG_DATA_LEN];
		data[MSG_API_ID] = GM_DESTROY;
		data[GUIMSG_GOBJ_ID] = gobj - gobjt;
		KUnmapProcAddr(gobj->vbuf, data);
	}
	FreeGobj(gobj);
}

/*取得窗体的绝对坐标*/
static void GetGobjPos(GOBJ_DESC *gobj, long *AbsXpos, long *AbsYpos)
{
	for (*AbsYpos = *AbsXpos = 0; gobj; gobj = gobj->par)
	{
		*AbsXpos += gobj->rect.xpos;
		*AbsYpos += gobj->rect.ypos;
	}
}

/*根据绝对坐标查找窗体*/
GOBJ_DESC *FindGobj(long *AbsXpos, long *AbsYpos)
{
	GOBJ_DESC *gobj, *CurGobj;

	gobj = NULL;
	CurGobj = &gobjt[0];
	while (CurGobj)
	{
		if (*AbsXpos >= CurGobj->rect.xpos && *AbsXpos < CurGobj->rect.xend && *AbsYpos >= CurGobj->rect.ypos && *AbsYpos < CurGobj->rect.yend)
		{
			*AbsXpos -= CurGobj->rect.xpos;
			*AbsYpos -= CurGobj->rect.ypos;
			gobj = CurGobj;
			CurGobj = CurGobj->chl;
		}
		else
			CurGobj = CurGobj->nxt;
	}
	return gobj;
}

/*创建主桌面*/
long CreateDesktop(THREAD_ID ptid, DWORD attr, long width, long height, DWORD *vbuf)
{
	GOBJ_DESC *gobj;

	if ((gobj = AllocGobj()) == NULL)
		return GUI_ERR_HAVENO_MEMORY;
	memset32(gobj, 0, sizeof(GOBJ_DESC) / sizeof(DWORD));
	gobj->ptid = ptid;
	gobj->attr = attr;
	gobj->rect.xpos = 0;
	gobj->rect.ypos = 0;
	gobj->rect.xend = width;
	gobj->rect.yend = height;
	gobj->vbuf = vbuf;
	DiscoverRectInter(gobj, 0, 0, width, height);	/*为桌面创建最初的剪切矩形*/

	return NO_ERROR;
}

/*创建窗体*/
long CreateGobj(THREAD_ID ptid, DWORD pid, DWORD attr, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len, DWORD *gid)
{
	GOBJ_DESC *gobj;
	GOBJ_DESC *ParGobj;	/*父节点*/
	long res;

	if (pid >= GOBJT_LEN)	/*检查父节点ID*/
		return GUI_ERR_INVALID_GOBJID;
	ParGobj = &gobjt[pid];
	if (ParGobj->attr == INVALID)	/*无效窗口*/
		return GUI_ERR_INVALID_GOBJID;
	if (width <= 0 || height <= 0)	/*尺寸错误*/
		return GUI_ERR_WRONG_GOBJSIZE;
	if (vbuf && width * height > len)	/*显示缓冲太小*/
		return GUI_ERR_WRONG_VBUF;
	width += xpos;
	height += ypos;
	if (vbuf == NULL)	/*没有指定显示缓冲*/
	{
		if (ptid.ProcID == ParGobj->ptid.ProcID)	/*与父窗体所属进程相同*/
		{
			if (xpos < 0 || width > ParGobj->rect.xend - ParGobj->rect.xpos ||	/*无显示缓冲位置越界*/
				ypos < 0 || height > ParGobj->rect.yend - ParGobj->rect.ypos)
				return GUI_ERR_WRONG_GOBJLOC;
		}
		else
			return GUI_ERR_HAVENO_VBUF;
	}
	if ((gobj = AllocGobj()) == NULL)
		return GUI_ERR_HAVENO_MEMORY;

	memset32(gobj, 0, sizeof(GOBJ_DESC) / sizeof(DWORD));	/*初始化新窗体*/
	gobj->ptid = ptid;
	gobj->attr = attr;
	gobj->rect.xpos = xpos;
	gobj->rect.ypos = ypos;
	gobj->rect.xend = width;
	gobj->rect.yend = height;
	gobj->vbuf = vbuf;
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
		DrawGobj(gobj, 0, 0, width - xpos, height - ypos, xpos, ypos, NULL);
	}
	*gid = gobj - gobjt;

	return NO_ERROR;
}

/*删除窗体*/
long DeleteGobj(THREAD_ID ptid, DWORD gid)
{
	GOBJ_DESC *gobj;
	GOBJ_DESC *ParGobj;	/*父节点*/
	long TmpXpos, TmpYpos, TmpXend, TmpYend, xpos, ypos, res;

	if (gid >= GOBJT_LEN)	/*检查窗体ID*/
		return GUI_ERR_INVALID_GOBJID;
	gobj = &gobjt[gid];
	if (gobj->attr == INVALID)	/*无效窗口*/
		return GUI_ERR_INVALID_GOBJID;
	if (ptid.ProcID != gobj->ptid.ProcID)	/*不允许删除其他进程的窗体*/
		return GUI_ERR_INVALID_GOBJID;

	res = DeleteClipList(gobj);
	ParGobj = gobj->par;
	if (ParGobj->chl == gobj)
		ParGobj->chl = gobj->nxt;
	if (gobj->pre)
		gobj->pre->nxt = gobj->nxt;
	if (gobj->nxt)
		gobj->nxt->pre = gobj->pre;
	TmpXpos = gobj->rect.xpos;
	TmpYpos = gobj->rect.ypos;
	TmpXend = gobj->rect.xend;
	TmpYend = gobj->rect.yend;
	FreeGobjList(gobj);
	if (res == NO_ERROR)
	{
		DeleteClipList(ParGobj);
		DiscoverRectInter(ParGobj, 0, 0, ParGobj->rect.xend - ParGobj->rect.xpos, ParGobj->rect.yend - ParGobj->rect.ypos);	/*显露新窗体*/
		CoverRectByPar(ParGobj);	/*覆盖窗体*/
		GetGobjPos(ParGobj, &xpos, &ypos);
		DrawGobj(ParGobj, TmpXpos, TmpYpos, TmpXend, TmpYend, xpos, ypos, NULL);
	}

	return NO_ERROR;
}

/*移动窗体*/
long MoveGobj(THREAD_ID ptid, DWORD gid, long xpos, long ypos)
{
	GOBJ_DESC *gobj, *ParGobj;	/*父节点*/
	long xend, yend, TmpXpos, TmpYpos, TmpXend, TmpYend, res;

	if (gid >= GOBJT_LEN)	/*检查窗体ID*/
		return GUI_ERR_INVALID_GOBJID;
	gobj = &gobjt[gid];
	if (gobj->attr == INVALID)	/*无效窗口*/
		return GUI_ERR_INVALID_GOBJID;
	if (ptid.ProcID != gobj->ptid.ProcID)	/*不允许移动其他进程的窗体*/
		return GUI_ERR_INVALID_GOBJID;
	ParGobj = gobj->par;
	xend = gobj->rect.xend - gobj->rect.xpos + xpos;
	yend = gobj->rect.yend - gobj->rect.ypos + ypos;
	if (gobj->vbuf == NULL)
		if (xpos < 0 || xend > ParGobj->rect.xend - ParGobj->rect.xpos || ypos < 0 || yend > ParGobj->rect.yend - ParGobj->rect.ypos)	/*越界*/
			return GUI_ERR_WRONG_GOBJLOC;

	res = DeleteClipList(ParGobj);
	TmpXpos = gobj->rect.xpos;
	TmpYpos = gobj->rect.ypos;
	TmpXend = gobj->rect.xend;
	TmpYend = gobj->rect.yend;
	gobj->rect.xpos = xpos;
	gobj->rect.ypos = ypos;
	gobj->rect.xend = xend;
	gobj->rect.yend = yend;
	if (res == NO_ERROR)
	{
		DiscoverRectInter(ParGobj, 0, 0, ParGobj->rect.xend - ParGobj->rect.xpos, ParGobj->rect.yend - ParGobj->rect.ypos);	/*显露新窗体*/
		CoverRectByPar(ParGobj);	/*覆盖窗体*/
		GetGobjPos(ParGobj, &xpos, &ypos);
		DrawGobj(ParGobj, TmpXpos, TmpYpos, TmpXend, TmpYend, xpos, ypos, gobj);
		DrawGobj(gobj, 0, 0, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos, xpos + gobj->rect.xpos, ypos + gobj->rect.ypos, NULL);
	}

	return NO_ERROR;
}

/*设置窗体的大小*/
long SizeGobj(THREAD_ID ptid, DWORD gid, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len)
{
	GOBJ_DESC *gobj, *ParGobj;	/*父节点*/
	long TmpXpos, TmpYpos, TmpXend, TmpYend, res;

	if (gid >= GOBJT_LEN)	/*检查窗体ID*/
		return GUI_ERR_INVALID_GOBJID;
	gobj = &gobjt[gid];
	if (gobj->attr == INVALID)	/*无效窗口*/
		return GUI_ERR_INVALID_GOBJID;
	if (ptid.ProcID != gobj->ptid.ProcID)	/*不允许设置其他进程的窗体*/
		return GUI_ERR_INVALID_GOBJID;
	if (width <= 0 || height <= 0)	/*尺寸错误*/
		return GUI_ERR_WRONG_GOBJSIZE;
	if (vbuf && width * height > len)	/*显示缓冲太小*/
		return GUI_ERR_WRONG_VBUF;
	ParGobj = gobj->par;
	width += xpos;
	height += ypos;
	if (vbuf == NULL)	/*没有指定显示缓冲*/
	{
		if (ptid.ProcID == ParGobj->ptid.ProcID)	/*与父窗体所属进程相同*/
		{
			if (xpos < 0 || width > ParGobj->rect.xend - ParGobj->rect.xpos ||	/*无显示缓冲位置越界*/
				ypos < 0 || height > ParGobj->rect.yend - ParGobj->rect.ypos)
				return GUI_ERR_WRONG_GOBJLOC;
		}
		else
			return GUI_ERR_HAVENO_VBUF;
	}

	ParGobj = gobj->par;
	if (gobj->vbuf)
	{
		DWORD data[MSG_DATA_LEN];
		data[MSG_API_ID] = GM_UNMAPVBUF;
		data[GUIMSG_GOBJ_ID] = gobj - gobjt;
		KUnmapProcAddr(gobj->vbuf, data);
	}
	res = DeleteClipList(ParGobj);
	TmpXpos = gobj->rect.xpos;
	TmpYpos = gobj->rect.ypos;
	TmpXend = gobj->rect.xend;
	TmpYend = gobj->rect.yend;
	gobj->rect.xpos = xpos;
	gobj->rect.ypos = ypos;
	gobj->rect.xend = width;
	gobj->rect.yend = height;
	gobj->vbuf = vbuf;
	if (res == NO_ERROR)
	{
		DiscoverRectInter(ParGobj, 0, 0, ParGobj->rect.xend - ParGobj->rect.xpos, ParGobj->rect.yend - ParGobj->rect.ypos);	/*显露新窗体*/
		CoverRectByPar(ParGobj);	/*覆盖窗体*/
		GetGobjPos(ParGobj, &xpos, &ypos);
		DrawGobj(ParGobj, TmpXpos, TmpYpos, TmpXend, TmpYend, xpos, ypos, gobj);
		DrawGobj(gobj, 0, 0, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos, xpos + gobj->rect.xpos, ypos + gobj->rect.ypos, NULL);
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
	DrawGobj(gobj, 0, 0, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos, xpos, ypos, NULL);

	return NO_ERROR;
}

/*设置窗体显示缓冲*/
long SetGobjVbuf(GOBJ_DESC *gobj, DWORD *vbuf)
{
	long xpos, ypos;

	gobj->vbuf = vbuf;
	GetGobjPos(gobj, &xpos, &ypos);
	DrawGobj(gobj, 0, 0, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos, xpos, ypos, NULL);

	return NO_ERROR;
}

/*	guirect.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�����GUI���о��λ�������
	����޸����ڣ�2010-10-05
*/

#include "gui.h"

CLIPRECT ClipRectt[CLIPRECTT_LEN], *FstClipRect;	/*���о��ι����ָ��*/

/*������о��νṹ*/
static CLIPRECT *AllocClipRect()
{
	CLIPRECT *clip;

	if (FstClipRect == NULL)
		return NULL;
	FstClipRect = (clip = FstClipRect)->nxt;
	return clip;
}

/*�ͷż��о��νṹ*/
static void FreeClipRect(CLIPRECT *clip)
{
	clip->nxt = FstClipRect;
	FstClipRect = clip;
}

#define MIN(a, b)	((a) < (b) ? (a) : (b))
#define MAX(a, b)	((a) > (b) ? (a) : (b))

/*�ڸǴ���ľ����ⲿ*/
long CoverRectExter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	CLIPRECT *PreClip, *CurClip;

	if (xpos <= 0 && xend >= gobj->rect.xend - gobj->rect.xpos &&	/*��ȫ��������*/
		ypos <= 0 && yend >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	for (PreClip = NULL, CurClip = gobj->ClipList; CurClip;)	/*��������*/
	{
		if (xend > CurClip->rect.xpos && xpos < CurClip->rect.xend &&
			yend > CurClip->rect.ypos && ypos < CurClip->rect.yend)	/*���ص�����*/
		{
			if (CurClip->rect.xpos < xpos)
				CurClip->rect.xpos = xpos;
			if (CurClip->rect.ypos < ypos)
				CurClip->rect.ypos = ypos;
			if (CurClip->rect.xend > xend)
				CurClip->rect.xend = xend;
			if (CurClip->rect.yend > yend)
				CurClip->rect.yend = yend;
			CurClip = (PreClip = CurClip)->nxt;	/*������������ڵ�*/
		}
		else
		{
			/*ɾ��ԭ���о���*/
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
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*�ݹ鴦���Ӵ���*/
		CoverRectExter(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos);

	return NO_ERROR;
}

/*�ڸǴ���ľ����ڲ�*/
long CoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	CLIPRECT *PreClip, *CurClip, *NewClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*û���ص�����*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	for (PreClip = NULL, CurClip = gobj->ClipList; CurClip;)	/*��������*/
	{
		if (xend > CurClip->rect.xpos && xpos < CurClip->rect.xend &&
			yend > CurClip->rect.ypos && ypos < CurClip->rect.yend)	/*���ص�����*/
		{
			/*��ߵĲ��ص���(���������Ϻ�����)*/
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
			/*�ұߵĲ��ص���(���������Ϻ�����)*/
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
			/*�ϱߵĲ��ص���(�������Ϻ�����)*/
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
			/*�±ߵĲ��ص���(�������Ϻ�����)*/
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
			/*ɾ��ԭ���о���*/
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
			CurClip = (PreClip = CurClip)->nxt;	/*������������ڵ�*/
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*�ݹ鴦���Ӵ���*/
		CoverRectInter(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos);

	return NO_ERROR;
}

/*��¶����ľ����ڲ�*/
long DiscoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
{
	GOBJ_DESC *ChlGobj;
	CLIPRECT *NewClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*û���ص�����*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return GUI_ERR_NOCHG_CLIPRECT;
	if (xpos < 0)	/*��������*/
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
	while (ChlGobj->nxt)	/*������ײ�Ĵ���*/
		 ChlGobj = ChlGobj->nxt;
	while (ChlGobj)	/*�ݹ鴦���Ӵ���*/
	{
		if (ChlGobj->vbuf)	/*�޸����岻���Ǹ�����*/
			CoverRectInter(gobj, ChlGobj->rect.xpos, ChlGobj->rect.ypos, ChlGobj->rect.xend, ChlGobj->rect.yend);
		DiscoverRectInter(ChlGobj, xpos - ChlGobj->rect.xpos, ypos - ChlGobj->rect.ypos, xend - ChlGobj->rect.xpos, yend - ChlGobj->rect.ypos);
		ChlGobj = ChlGobj->pre;
	}

	return NO_ERROR;
}

/*���游���沮�����帲��*/
void CoverRectByPar(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ParGobj, *PreGobj;
	long xpos, ypos;

	ypos = xpos = 0;
	ParGobj = gobj;
	for (;;)
	{
		xpos -= ParGobj->rect.xpos;
		ypos -= ParGobj->rect.ypos;
		for (PreGobj = ParGobj->pre; PreGobj; PreGobj = PreGobj->pre)	/*���沮�����帲���ڲ�*/
			CoverRectInter(gobj, xpos + PreGobj->rect.xpos, ypos + PreGobj->rect.ypos, xpos + PreGobj->rect.xend, ypos + PreGobj->rect.yend);
		if (ParGobj->par == NULL)
			break;
		ParGobj = ParGobj->par;	/*���游���帲���ⲿ*/
		CoverRectExter(gobj, xpos, ypos, xpos + ParGobj->rect.xend - ParGobj->rect.xpos, ypos + ParGobj->rect.yend - ParGobj->rect.ypos);
	}
}

/*ɾ�����о�������*/
long DeleteClipList(GOBJ_DESC *gobj)
{
	BOOL isNoChg = TRUE;

	if (gobj->ClipList)
	{
		CLIPRECT *CurClip;

		CurClip = gobj->ClipList;	/*������β�ڵ�*/
		while (CurClip->nxt)
			CurClip = CurClip->nxt;
		CurClip->nxt = FstClipRect;
		FstClipRect = gobj->ClipList;
		gobj->ClipList = NULL;
		isNoChg = FALSE;
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*�ݹ鴦���Ӵ���*/
		if (DeleteClipList(gobj) == NO_ERROR)
			isNoChg = FALSE;
	if (isNoChg)
		return GUI_ERR_NOCHG_CLIPRECT;

	return NO_ERROR;
}

/*���ƴ�������ڲ�*/
void DrawGobj(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend, long AbsXpos, long AbsYpos, BOOL isDrawNoRoot)
{
	CLIPRECT *CurClip;

	if (xend <= 0 || xpos >= gobj->rect.xend - gobj->rect.xpos ||	/*û���ص�����*/
		yend <= 0 || ypos >= gobj->rect.yend - gobj->rect.ypos)
		return;
	if (gobj->vbuf || isDrawNoRoot)	/*��������������ǿ�ƻ����޸�����ʱ���л���*/
	{
		GOBJ_DESC *RootGobj;
		long RootXpos, RootYpos;
		for (RootGobj = gobj, RootYpos = RootXpos = 0; RootGobj->vbuf == NULL; RootGobj = RootGobj->par)	/*����ʾ������ֱ�ӻ�ͼ,û����ʹ���游�������ʾ����*/
		{
			RootXpos += RootGobj->rect.xpos;
			RootYpos += RootGobj->rect.ypos;
		}
		for (CurClip = gobj->ClipList; CurClip; CurClip = CurClip->nxt)	/*��������*/
		{
			long RectXpos, RectYpos, RectXend, RectYend;

			RectXpos = MAX(xpos, CurClip->rect.xpos);
			RectYpos = MAX(ypos, CurClip->rect.ypos);
			RectXend = MIN(xend, CurClip->rect.xend);
			RectYend = MIN(yend, CurClip->rect.yend);
			if (RectXpos < RectXend && RectYpos < RectYend)	/*���ص�����*/
			{
				BOOL isRefreshMouse;

				isRefreshMouse = CheckMousePos(AbsXpos + RectXpos, AbsYpos + RectYpos, AbsXpos + RectXend, AbsYpos + RectYend);
				if (isRefreshMouse)
					HideMouse();
				GuiPutImage(AbsXpos + RectXpos, AbsYpos + RectYpos, RootGobj->vbuf + (RootXpos + RectXpos) + (RootYpos + RectYpos) * (RootGobj->rect.xend - RootGobj->rect.xpos), RootGobj->rect.xend - RootGobj->rect.xpos, RectXend - RectXpos, RectYend - RectYpos);
				if (isRefreshMouse)
					ShowMouse();
			}
		}
	}
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*�ݹ鴦���Ӵ���*/
		DrawGobj(gobj, xpos - gobj->rect.xpos, ypos - gobj->rect.ypos, xend - gobj->rect.xpos, yend - gobj->rect.ypos, AbsXpos + gobj->rect.xpos, AbsYpos + gobj->rect.ypos, FALSE);
}

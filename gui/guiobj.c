/*	guiobj.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�����GUI����(����)��������
	����޸����ڣ�2010-10-05
*/

#include "gui.h"
#include "control.h"

/**********���о��ι������**********/

CLIPRECT *FstClipRect;	/*���о��ι����ָ��*/

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

/*�ͷż��о�����*/
inline void FreeClipList(CLIPRECT *clip)
{
	if (clip)
	{
		CLIPRECT *CurClip;

		CurClip = clip;	/*������β�ڵ�*/
		while (CurClip->nxt)
			CurClip = CurClip->nxt;
		CurClip->nxt = FstClipRect;
		FstClipRect = clip;
	}
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

/*��¶����,�������μ�������*/
long DiscoverClipRect(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend)
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
		CoverRectInter(gobj, ChlGobj->rect.xpos, ChlGobj->rect.ypos, ChlGobj->rect.xend, ChlGobj->rect.yend);
		DiscoverClipRect(ChlGobj, xpos - ChlGobj->rect.xpos, ypos - ChlGobj->rect.ypos, xend - ChlGobj->rect.xpos, yend - ChlGobj->rect.ypos);
		ChlGobj = ChlGobj->pre;
	}

	return NO_ERROR;
}

/*���游���沮�����帲��*/
long CoverClipRectByPar(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ParGobj, *PreGobj;
	long xpos, ypos;

	xpos = -gobj->rect.xpos;	/*���游���帲���ⲿ*/
	ypos = -gobj->rect.ypos;
	for (ParGobj = gobj->par; ParGobj; ParGobj = ParGobj->par)
	{
		CoverRectExter(gobj, xpos, ypos, xpos + ParGobj->rect.xend - ParGobj->rect.xpos, ypos + ParGobj->rect.yend - ParGobj->rect.ypos);
		xpos -= ParGobj->rect.xpos;
		ypos -= ParGobj->rect.ypos;
	}
	xpos = gobj->rect.xpos;	/*���沮�����帲���ڲ�*/
	ypos = gobj->rect.ypos;
	for (ParGobj = gobj->par; ParGobj; ParGobj = ParGobj->par)
	{
		xpos += ParGobj->rect.xpos;
		ypos += ParGobj->rect.ypos;
		for (PreGobj = ParGobj->pre; PreGobj; PreGobj = PreGobj->pre)
			CoverRectInter(gobj, PreGobj->rect.xpos - xpos, PreGobj->rect.ypos - ypos, PreGobj->rect.xend - xpos, PreGobj->rect.yend - ypos);
	}

	return NO_ERROR;
}

/*ǿ��ɾ�����о�������*/
void DeleteClipRect(GOBJ_DESC *gobj)
{
	FreeClipList(gobj->ClipList);
	gobj->ClipList = NULL;
	for (gobj = gobj->chl; gobj; gobj = gobj->nxt)	/*�ݹ鴦���Ӵ���*/
		DeleteClipRect(gobj);
}

/*���ݴ�������ϵ��������ļ��о�������*/
long CreateClipList(GOBJ_DESC *gobj)
{
	if (CoverRectInter(gobj->par, gobj->rect.xpos, gobj->rect.ypos, gobj->rect.xend, gobj->rect.yend) != NO_ERROR)	/*�������屻�´����ڸǵ�����*/
		return GUI_ERR_NOCHG_CLIPRECT;
	DiscoverClipRect(gobj, 0, 0, gobj->rect.xend - gobj->rect.xpos, gobj->rect.yend - gobj->rect.ypos);	/*��¶�´���*/
	CoverClipRectByPar(gobj);	/*���Ǵ���*/

	return NO_ERROR;
}

/**********����������**********/

GOBJ_DESC *gobjt[GOBJT_LEN];	/*����������ָ���*/
GOBJ_DESC **FstGobj;			/*��һ���ն���������ָ��*/

#define GUICIT_LEN	3	/*�ؼ��ӿڱ���*/

GCTRLI guicit[GUICIT_LEN] = {
	{sizeof(GUIOBJ_DESKTOP), InitDesktop, ReleaseDesktop},
	{sizeof(GUIOBJ_WINDOW), InitWindow, ReleaseWindow},
	{sizeof(GUIOBJ_BUTTON), InitButton, ReleaseButton},
};

/*����մ���ID*/
static long AllocGobj(GOBJ_DESC *gobj)
{
	if (FstGobj >= &gobjt[GOBJT_LEN])
		return GUI_ERR_HAVENO_GOBJ;
	gobj->id = FstGobj - gobjt;
	*FstGobj = gobj;
	do
		FstGobj++;
	while (FstGobj < &gobjt[GOBJT_LEN] && *FstGobj);
	return NO_ERROR;
}

/*�ͷſմ���ID*/
static void FreeGobj(WORD gid)
{
	GOBJ_DESC **gobj;

	gobj = &gobjt[gid];
	*gobj = NULL;
	if (FstGobj > gobj)
		FstGobj = gobj;
}

/*�ݹ��ͷŴ�����*/
static void FreeGobjList(GOBJ_DESC *gobj)
{
	GOBJ_DESC *ChlGobj;

	ChlGobj = gobj->chl;
	FreeGobj(gobj->id);
	ffree(gobj, guicit[gobj->type].ObjSize);
	while (ChlGobj)
	{
		gobj = ChlGobj->nxt;
		FreeGobjList(ChlGobj);
		ChlGobj = gobj;
	}
}

/*����������*/
long CreateDesktop(long width, long height)
{
	GOBJ_DESC *gobj;

	if ((gobj = (GOBJ_DESC*)falloc(sizeof(GUIOBJ_DESKTOP))) == NULL)
		return GUI_ERR_HAVENO_MEMORY;
	memset32(gobj, 0, sizeof(GUIOBJ_DESKTOP) / sizeof(DWORD));
	gobj->type = GUI_TYPE_DESKTOP;
	*((DWORD*)&gobj->ptid) = INVALID;	/*�����̵߳�PTID*/
	gobj->rect.xpos = 0;
	gobj->rect.ypos = 0;
	gobj->rect.xend = width;
	gobj->rect.yend = height;
	AllocGobj(gobj);
	DiscoverClipRect(gobj, 0, 0, width, height);	/*Ϊ���洴������ļ��о���*/
	guicit[GUI_TYPE_DESKTOP].InitCtrl(gobj);

	return NO_ERROR;
}

/*��������*/
long CreateGobj(THREAD_ID ptid, WORD pid, WORD type, long xpos, long ypos, long width, long height, DWORD attr, WORD *id)
{
	GOBJ_DESC *gobj;
	GOBJ_DESC *ParGobj;	/*���ڵ�*/
	long xend, yend;

	if (pid >= GOBJT_LEN)	/*��鸸�ڵ�ID*/
		return GUI_ERR_WRONG_GOBJID;
	ParGobj = gobjt[pid];
	if (ParGobj == NULL)
		return GUI_ERR_WRONG_GOBJID;
	if (type >= GUICIT_LEN)
		return GUI_ERR_WRONG_GOBJTYPE;
	if (width <= 0 || height <= 0)
		return GUI_ERR_WRONG_WNDSIZE;
	if ((gobj = (GOBJ_DESC*)falloc(guicit[type].ObjSize)) == NULL)
		return GUI_ERR_HAVENO_MEMORY;

	memset32(gobj, 0, guicit[type].ObjSize / sizeof(DWORD));	/*��ʼ���´���*/
	gobj->type = type;
	gobj->ptid = ptid;
	xend = xpos + width;
	yend = ypos + height;
	gobj->rect.xpos = xpos;
	gobj->rect.ypos = ypos;
	gobj->rect.xend = xend;
	gobj->rect.yend = yend;
	gobj->nxt = ParGobj->chl;
	gobj->par = ParGobj;
	gobj->attr = attr;
	if (AllocGobj(gobj) != NO_ERROR)
	{
		ffree(gobj, guicit[type].ObjSize);
		return GUI_ERR_HAVENO_GOBJ;
	}
	if (ParGobj->chl)	/*�����ֵܽڵ�*/
		ParGobj->chl->pre = gobj;
	ParGobj->chl = gobj;
	CreateClipList(gobj);
	guicit[type].InitCtrl(gobj);
	*id = gobj->id;

	return NO_ERROR;
}

/*ɾ������*/
long DeleteGobj(THREAD_ID ptid, WORD id)
{
	GOBJ_DESC *gobj;
	GOBJ_DESC *ParGobj;	/*���ڵ�*/

	if (id >= GOBJT_LEN)	/*���ڵ�ID*/
		return GUI_ERR_WRONG_GOBJID;
	gobj = gobjt[id];
	if (gobj == NULL)
		return GUI_ERR_WRONG_GOBJID;
	if (gobj->ptid.ProcID != ptid.ProcID)
		return GUI_ERR_WRONG_HANDLE;

	guicit[gobj->type].ReleaseCtrl(gobj);
	ParGobj = gobj->par;
	DeleteClipRect(ParGobj);
	if (ParGobj && ParGobj->chl == gobj)
		ParGobj->chl = gobj->nxt;
	if (gobj->pre)
		gobj->pre->nxt = gobj->nxt;
	if (gobj->nxt)
		gobj->nxt->pre = gobj->pre;
	FreeGobjList(gobj);
	CreateClipList(ParGobj);

	return NO_ERROR;
}

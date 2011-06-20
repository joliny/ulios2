/*	gui.h for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�������ؽṹ����������
	����޸����ڣ�2010-10-03
*/

#ifndef	_GUI_H_
#define	_GUI_H_

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "guiapi.h"

/**********ͼ���û�����ṹ����**********/

typedef struct _RECT
{
	long xpos, ypos, xend, yend;
}RECT;	/*���νṹ*/

#define CLIPRECTT_LEN	0x2000		/*���о��ι������*/

typedef struct _CLIPRECT
{
	RECT rect;						/*���о���*/
	struct _CLIPRECT *nxt;			/*����ָ��*/
}CLIPRECT;	/*���Ӽ��о��νڵ�,���ڴ�����ص�����*/

#define GOBJT_LEN		0x4000		/*�����������������*/

typedef struct _GOBJ_DESC
{
	THREAD_ID ptid;					/*�����߳�ID*/
	DWORD attr;						/*����*/
	RECT rect;						/*��Ը������λ��*/
	DWORD *vbuf;					/*�����ڴ滺��,NULLʹ�ø�������Ļ���*/
	struct _GOBJ_DESC *pre, *nxt;	/*��/�ܶ�����ָ��*/
	struct _GOBJ_DESC *par, *chl;	/*��/�Ӷ�����ָ��*/
	CLIPRECT *ClipList;				/*������о����б�*/
}GOBJ_DESC;	/*GUI����(����)������*/

/**********���о��ι������**********/

/*�ڸǴ���ľ����ⲿ*/
long CoverRectExter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*�ڸǴ���ľ����ڲ�*/
long CoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*��¶����ľ����ڲ�*/
long DiscoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*���游���沮�����帲��*/
void CoverRectByPar(GOBJ_DESC *gobj);

/*ɾ�����о�������*/
long DeleteClipList(GOBJ_DESC *gobj);

/*���ƴ�������ڲ�*/
void DrawGobj(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend, long AbsXpos, long AbsYpos, GOBJ_DESC *ExcludeGobj);

/**********����������**********/

/*���ݾ���������Ҵ���*/
GOBJ_DESC *FindGobj(long *AbsXpos, long *AbsYpos);

/*����������*/
long CreateDesktop(THREAD_ID ptid, DWORD attr, long width, long height, DWORD *vbuf);

/*��������*/
long CreateGobj(THREAD_ID ptid, DWORD pid, DWORD attr, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len, DWORD *gid);

/*ɾ������*/
long DeleteGobj(THREAD_ID ptid, DWORD gid);

/*���ô����λ�ô�С*/
long MoveGobj(THREAD_ID ptid, DWORD gid, long xpos, long ypos);

/*���ô���Ĵ�С*/
long SizeGobj(THREAD_ID ptid, DWORD gid, long xpos, long ypos, long width, long height, DWORD *vbuf, DWORD len);

/*���ô���Ϊ�*/
long ActiveGobj(GOBJ_DESC *gobj);

/*���ô�����ʾ����*/
long SetGobjVbuf(GOBJ_DESC *gobj, DWORD *vbuf);

/**********���ܿ����**********/

/*GUI���ο���ͼ*/
void GuiPutImage(long x, long y, DWORD *img, long memw, long w, long h);

/*����BMPͼ���ļ�*/
long LoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height);

/**********�����߳����**********/

/*������Ϣ�����߳�*/
void DesktopThread(GOBJ_DESC *gobj);

/**********��깦�����**********/

/*����ʼ��*/
long InitMouse();

/*�������ָ���������Ƿ��ص�*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend);

/*�������ָ��*/
void HidMouse();

/*��ʾ���ָ��*/
void ShowMouse();

/*�����Ϣ����*/
void MouseProc(DWORD key, long x, long y, long z);

#endif

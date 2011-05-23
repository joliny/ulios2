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
	struct _GOBJ_DESC *pre, *nxt;	/*��/�ܶ�����ָ��*/
	struct _GOBJ_DESC *par, *chl;	/*��/�Ӷ�����ָ��*/
	CLIPRECT *ClipList;				/*������о����б�*/
	DWORD *vbuf;					/*�����ڴ滺��*/
}GOBJ_DESC;	/*GUI����(����)������*/

/**********���о��ι������**********/

/*�ڸǴ���ľ����ⲿ*/
long CoverRectExter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*�ڸǴ���ľ����ڲ�*/
long CoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*��¶����ľ����ڲ�*/
long DiscoverRectInter(GOBJ_DESC *gobj, long xpos, long ypos, long xend, long yend);

/*���游���沮�����帲��*/
long CoverRectByPar(GOBJ_DESC *gobj);

/*ǿ��ɾ�����о�������*/
long DeleteClipList(GOBJ_DESC *gobj);

/**********����������**********/

/*���ݾ���������Ҵ���*/
GOBJ_DESC *FindGobj(long *GobjXpos, long *GobjYpos);

/*���ƴ���*/
long DrawGobj(GOBJ_DESC *gobj, long xpos, long ypos);

/*����������*/
long CreateDesktop(THREAD_ID ptid, DWORD attr, long width, long height, DWORD *DesktopPic);

/*��������*/
long CreateGobj(THREAD_ID ptid, DWORD attr, DWORD pid, long xpos, long ypos, long width, long height, DWORD *id);

/*ɾ������*/
long DeleteGobj(GOBJ_DESC *gobj);

/*���ô����λ�ô�С*/
long MoveGobj(GOBJ_DESC *gobj, long xpos, long ypos, long width, long height);

/*���ô���Ϊ�*/
long ActiveGobj(GOBJ_DESC *gobj);

/**********���ܿ����**********/

long LoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height);

/**********�����߳����**********/

void DesktopThread(void *args);

/**********��깦�����**********/

extern long MouX, MouY;

/*����ʼ��*/
long InitMouse();

/*�������ָ���������Ƿ��ص�*/
BOOL CheckMousePos(long xpos, long ypos, long xend, long yend);

/*�������ָ��*/
void HidMouse();

/*��ʾ���ָ��*/
void ShowMouse();

/*�������λ��*/
void SetMousePos(long x, long y);

#endif

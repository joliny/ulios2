/*	control.h for ulios graphical user interface
	���ߣ�����
	���ܣ��ؼ���ؽṹ����������
	����޸����ڣ�2010-10-05
*/

#ifndef	_CONTROL_H_
#define	_CONTROL_H_

#include "gui.h"

typedef struct _GUIOBJ_DESKTOP
{
	GOBJ_DESC obj;
}GUIOBJ_DESKTOP;	/*����ṹ*/

typedef struct _GUIOBJ_WINDOW
{
	GOBJ_DESC obj;
}GUIOBJ_WINDOW;		/*���ڽṹ*/

#define BUTTON_CAPTION_LEN 256
typedef struct _GUIOBJ_BUTTON
{
	GOBJ_DESC obj;
	char caption[BUTTON_CAPTION_LEN];
}GUIOBJ_BUTTON;		/*��ť�ṹ*/

long InitDesktop(GUIOBJ_DESKTOP *dsk);

long InitWindow(GUIOBJ_WINDOW *wnd);

long InitButton(GUIOBJ_BUTTON *btn);

long ReleaseDesktop(GUIOBJ_DESKTOP *dsk);

long ReleaseWindow(GUIOBJ_WINDOW *wnd);

long ReleaseButton(GUIOBJ_BUTTON *btn);

#endif

/*	control.h for ulios graphical user interface
	���ߣ�����
	���ܣ��ؼ���ؽṹ����������
	����޸����ڣ�2010-10-05
*/

#ifndef	_CONTROL_H_
#define	_CONTROL_H_

#include "gui.h"

typedef struct _GUIDSK_VRECT
{
	RECT rect;					/*���о���*/
	GUIOBJ_DESC *obj;			/*������������ָ��*/
	struct _GUIDSK_VRECT *nxt;	/*����ָ��*/
}GUIDSK_VRECT;	/*���Ӽ��о��νڵ�,���������ڴ��ڵ��ص�����*/

typedef struct _GUIOBJ_DESKTOP
{
	GUIOBJ_DESC obj;
	GUIDSK_VRECT *RectList;		/*���о����б�*/
}GUIOBJ_DESKTOP;	/*����ṹ*/

typedef struct _GUIOBJ_WINDOW
{
	GUIOBJ_DESC obj;
}GUIOBJ_WINDOW;		/*���ڽṹ*/

#define BUTTON_CAPTION_LEN 1024
typedef struct _GUIOBJ_BUTTON
{
	GUIOBJ_DESC obj;
	char caption[BUTTON_CAPTION_LEN];
}GUIOBJ_BUTTON;		/*��ť�ṹ*/

#endif

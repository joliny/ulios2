/*	guiapi.h for ulios graphical user interface
	���ߣ�����
	���ܣ�ulios GUI����API�ӿڶ��壬����GUI������Ҫ�������ļ�
	����޸����ڣ�2010-10-05
*/

#ifndef	_GUIAPI_H_
#define	_GUIAPI_H_

#include "../MkApi/ulimkapi.h"

#define GUI_TYPE_DESKTOP	0	/*��������*/
#define GUI_TYPE_WINDOW		1	/*��������*/
#define GUI_TYPE_BUTTON		2	/*��ť����*/

/*������*/
#define GUI_ERR_HAVENO_GOBJ		-2560	/*��������������*/
#define GUI_ERR_HAVENO_MEMORY	-2561	/*�ڴ治��*/
#define GUI_ERR_HAVENO_CLIPRECT	-2562	/*���о��β���,����ϵͳ���ڸ���*/
#define GUI_ERR_WRONG_GOBJTYPE	-2563	/*����Ĵ�������*/
#define GUI_ERR_WRONG_WNDSIZE	-2564	/*����Ĵ��ڴ�С*/
#define GUI_ERR_WRONG_GOBJID	-2565	/*����Ĵ���ID*/
#define GUI_ERR_NOT_OVERLAP		-2566	/*���β��ص�*/
#define GUI_ERR_NOCHG_CLIPRECT	-2567	/*���о���û�б仯*/
#define GUI_ERR_WRONG_HANDLE	-2568	/*����Ĵ�����*/

#define SRV_GUI_PORT		10	/*GUI����˿�*/

#endif

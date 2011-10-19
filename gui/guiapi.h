/*	guiapi.h for ulios graphical user interface
	���ߣ�����
	���ܣ�ulios GUI����API�ӿڶ��壬����GUI������Ҫ�������ļ�
	����޸����ڣ�2010-10-05
*/

#ifndef	_GUIAPI_H_
#define	_GUIAPI_H_

#include "../driver/basesrv.h"

/*������*/
#define GUI_ERR_HAVENO_GOBJ		-2560	/*��������������*/
#define GUI_ERR_HAVENO_MEMORY	-2561	/*�ڴ治��*/
#define GUI_ERR_HAVENO_CLIPRECT	-2562	/*���о��β���,����ϵͳ���ڸ���*/
#define GUI_ERR_WRONG_GOBJTYPE	-2563	/*����Ĵ�������*/
#define GUI_ERR_WRONG_GOBJLOC	-2564	/*����Ĵ���λ��*/
#define GUI_ERR_WRONG_GOBJSIZE	-2565	/*����Ĵ����С*/
#define GUI_ERR_INVALID_GOBJID	-2566	/*����Ĵ���ID*/
#define GUI_ERR_NOT_OVERLAP		-2567	/*���β��ص�*/
#define GUI_ERR_NOCHG_CLIPRECT	-2568	/*���о���û�б仯*/
#define GUI_ERR_WRONG_HANDLE	-2569	/*����Ĵ�����*/
#define GUI_ERR_HAVENO_VBUF		-2570	/*��ʾ���岻����*/
#define GUI_ERR_WRONG_VBUF		-2571	/*��ʾ�������*/
#define GUI_ERR_WRONG_ARGS		-2572	/*��������*/
#define GUI_ERR_NOCHG_FOCUS		-2573	/*�����ޱ仯*/
#define GUI_ERR_BEING_DRAGGED	-2574	/*�д������ڱ���ק*/

#define SRV_GUI_PORT		10	/*GUI����˿�*/

#define MSG_ATTR_GUI	0x010A0000	/*GUI��Ϣ*/

#define GUIMSG_GOBJ_ID	6	/*GUI��Ϣ�����������*/

#define GM_GETGINFO		0x00	/*ȡ��GUI��Ϣ*/
#define GM_CREATE		0x01	/*��������*/
#define GM_DESTROY		0x02	/*���ٴ���*/
#define GM_MOVE			0x03	/*�ƶ�����*/
#define GM_SIZE			0x04	/*���Ŵ���*/
#define GM_PAINT		0x05	/*���ƴ���*/
#define GM_SETFOCUS		0x06	/*������Ϣ*/
#define GM_DRAG			0x07	/*��ק��Ϣ*/

#define GM_DRAGMOD_NONE	0		/*ȡ����קģʽ*/
#define GM_DRAGMOD_MOVE	1		/*��ק�ƶ�ģʽ*/
#define GM_DRAGMOD_SIZE	2		/*��ק����ģʽ*/

#define GM_MOUSEENTER	0x10	/*�������*/
#define GM_MOUSELEAVE	0x11	/*����Ƴ�*/
#define GM_MOUSEMOVE	0x12	/*����ƶ�*/
#define GM_LBUTTONDOWN	0x13	/*�������*/
#define GM_RBUTTONDOWN	0x14	/*�Ҽ�����*/
#define GM_MBUTTONDOWN	0x15	/*�м�����*/
#define GM_LBUTTONUP	0x16	/*���̧��*/
#define GM_RBUTTONUP	0x17	/*�Ҽ�̧��*/
#define GM_MBUTTONUP	0x18	/*�м�̧��*/
#define GM_LBUTTONCLICK	0x19	/*�������*/
#define GM_RBUTTONCLICK	0x1A	/*�Ҽ�����*/
#define GM_MBUTTONCLICK	0x1B	/*�м�����*/
#define GM_LBUTTONDBCLK	0x1C	/*���˫��*/
#define GM_RBUTTONDBCLK	0x1D	/*�Ҽ�˫��*/
#define GM_MBUTTONDBCLK	0x1E	/*�м�˫��*/
#define GM_MOUSEWHEEL	0x1F	/*������*/

#define GM_UNMAPVBUF	0x0100	/*������ʾ����ӳ��*/

/*ȡ��GUI��Ϣ*/
static inline long GUIGetGinfo(THREAD_ID ptid, DWORD *GUIwidth, DWORD *GUIheight)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_GETGINFO;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*GUIwidth = data[1];
	*GUIheight = data[2];
	return data[MSG_RES_ID];
}

/*��������*/
static inline long GUIcreate(THREAD_ID ptid, DWORD pid, DWORD ClintSign, short x, short y, WORD width, WORD height, DWORD *vbuf)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_CREATE;
	data[3] = (WORD)x | ((DWORD)(WORD)y << 16);
	data[4] = width | ((DWORD)height << 16);
	data[5] = pid;
	data[GUIMSG_GOBJ_ID] = ClintSign;
	if (vbuf)
		return KWriteProcAddr(vbuf, (DWORD)width * height * sizeof(DWORD), &ptid, data, 0);
	else
		return KSendMsg(&ptid, data, 0);
}

/*���ٴ���*/
static inline long GUIdestroy(THREAD_ID ptid, DWORD gid)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_DESTROY;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*�ƶ�����*/
static inline long GUImove(THREAD_ID ptid, DWORD gid, long x, long y)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_MOVE;
	data[1] = x;
	data[2] = y;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*���ô����С*/
static inline long GUIsize(THREAD_ID ptid, DWORD gid, DWORD *vbuf, short x, short y, WORD width, WORD height)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_SIZE;
	data[3] = (WORD)x | ((DWORD)(WORD)y << 16);
	data[4] = width | ((DWORD)height << 16);
	data[GUIMSG_GOBJ_ID] = gid;
	if (vbuf)
		return KWriteProcAddr(vbuf, (DWORD)width * height * sizeof(DWORD), &ptid, data, 0);
	else
		return KSendMsg(&ptid, data, 0);
}

/*�ػ洰������*/
static inline long GUIpaint(THREAD_ID ptid, DWORD gid, short x, short y, WORD width, WORD height)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_PAINT;
	data[1] = (WORD)x | ((DWORD)(WORD)y << 16);
	data[2] = width | ((DWORD)height << 16);
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*���ô��役��*/
static inline long GUISetFocus(THREAD_ID ptid, DWORD gid, BOOL isFocus)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_SETFOCUS;
	data[1] = isFocus;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

/*������ק��Ϣ*/
static inline long GUIdrag(THREAD_ID ptid, DWORD gid, DWORD mode)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_DRAG;
	data[1] = mode;
	data[GUIMSG_GOBJ_ID] = gid;
	return KSendMsg(&ptid, data, 0);
}

#endif

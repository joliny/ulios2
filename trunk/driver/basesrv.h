/*	bsrvapi.h for ulios driver
	���ߣ�����
	���ܣ�ulios��������API�ӿڶ��壬���û���������Ҫ�������ļ�
	����޸����ڣ�2010-03-09
*/

#ifndef	_BSRVAPI_H_
#define	_BSRVAPI_H_

#include "../MkApi/ulimkapi.h"

#define SRV_OUT_TIME	100	/*������ó�ʱ������INVALID:���޵ȴ�*/

/**********ATӲ�����**********/
#define SRV_ATHD_PORT	1	/*ATӲ����������˿�*/
#define ATHD_BPS		512	/*����ÿ�����ֽ���*/
#define ATHD_OUT_TIME	6000	/*��ʱ������INVALID:���޵ȴ�*/

#define ATHD_API_READSECTOR		0	/*��Ӳ���������ܺ�*/
#define ATHD_API_WRITESECTOR	1	/*дӲ���������ܺ�*/

#define ATHD_ERR_WRONG_DRV		-512	/*�������������*/
#define ATHD_ERR_HAVENO_REQ		-513	/*�޷����ܸ���ķ�������*/

/*��Ӳ������*/
static inline long HDReadSector(THREAD_ID ptid, DWORD drv, DWORD sec, BYTE cou, void *buf)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = ATHD_API_READSECTOR;
	data[1] = drv;
	data[2] = sec;
	if ((data[0] = KReadProcAddr(buf, ATHD_BPS * cou, ptid, data, ATHD_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*дӲ������*/
static inline long HDWriteSector(THREAD_ID ptid, DWORD drv, DWORD sec, BYTE cou, void *buf)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = ATHD_API_WRITESECTOR;
	data[1] = drv;
	data[2] = sec;
	if ((data[0] = KWriteProcAddr(buf, ATHD_BPS * cou, ptid, data, ATHD_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[2];
}

/**********ʱ��������**********/
#define SRV_TIME_PORT	2	/*ʱ�����˿�*/

#define TIME_API_CURSECOND		0	/*ȡ��1970�꾭�����빦�ܺ�*/
#define TIME_API_CURTIME		1	/*ȡ�õ�ǰʱ�书�ܺ�*/
#define TIME_API_MKTIME			2	/*TM�ṹת��Ϊ�빦�ܺ�*/
#define TIME_API_LOCALTIME		3	/*��ת��ΪTM�ṹ���ܺ�*/

#define TIME_ERR_WRONG_TM		-768	/*�Ƿ�TM�ṹ*/

typedef struct _TM
{
	BYTE sec;	/*��[0,59]*/
	BYTE min;	/*��[0,59]*/
	BYTE hor;	/*ʱ[0,23]*/
	BYTE day;	/*��[1,31]*/
	BYTE mon;	/*��[1,12]*/
	BYTE wday;	/*����[0,6]*/
	WORD yday;	/*һ���еĵڼ���[0,365]*/
	WORD yer;	/*��[1970,...]*/
	WORD mil;	/*����[0,999]*/
}TM;	/*ʱ��ṹ*/

/*ȡ��1970�꾭������*/
static inline long TMCurSecond(THREAD_ID ptid, DWORD *sec)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = TIME_API_CURSECOND;
	if ((data[0] = KSendMsg(ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*sec = data[1];
	return NO_ERROR;
}

/*ȡ�õ�ǰʱ��*/
static inline long TMCurTime(THREAD_ID ptid, TM *tm)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = TIME_API_CURTIME;
	if ((data[0] = KSendMsg(ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	memcpy32(tm, &data[1], sizeof(TM) / sizeof(DWORD));
	return NO_ERROR;
}

/*TM�ṹת��Ϊ��*/
static inline long TMMkTime(THREAD_ID ptid, DWORD *sec, const TM *tm)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = TIME_API_MKTIME;
	memcpy32(&data[2], tm, sizeof(TM) / sizeof(DWORD));
	if ((data[0] = KSendMsg(ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*sec = data[1];
	return NO_ERROR;
}

/*��ת��ΪTM�ṹ*/
static inline long TMLocalTime(THREAD_ID ptid, DWORD sec, TM *tm)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = TIME_API_LOCALTIME;
	data[2] = sec;
	if ((data[0] = KSendMsg(ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	memcpy32(tm, &data[1], sizeof(TM) / sizeof(DWORD));
	return NO_ERROR;
}

/**********�������������**********/
#define SRV_KBDMUS_PORT	3	/*����������˿�*/

#define KBD_STATE_LSHIFT	0x00010000
#define KBD_STATE_RSHIFT	0x00020000
#define KBD_STATE_LCTRL		0x00040000
#define KBD_STATE_RCTRL		0x00080000
#define KBD_STATE_LALT		0x00100000
#define KBD_STATE_RALT		0x00200000
#define KBD_STATE_PAUSE		0x00400000
#define KBD_STATE_PRTSC		0x00800000
#define KBD_STATE_SCRLOCK	0x01000000
#define KBD_STATE_NUMLOCK	0x02000000
#define KBD_STATE_CAPSLOCK	0x04000000
#define KBD_STATE_INSERT	0x08000000

#define MUS_STATE_LBUTTON	0x01
#define MUS_STATE_RBUTTON	0x02
#define MUS_STATE_MBUTTON	0x04
#define MUS_STATE_XSIGN		0x10
#define MUS_STATE_YSIGN		0x20
#define MUS_STATE_XOVRFLW	0x40
#define MUS_STATE_YOVRFLW	0x80

#define KBDMUS_API_SETRECV		0	/*ע����ռ��������Ϣ���̹߳��ܺ�*/

#define MSG_ATTR_KBD	(MSG_ATTR_USER)		/*���̰�����Ϣ*/
#define MSG_ATTR_MUS	(MSG_ATTR_USER + 1)	/*���״̬��Ϣ*/

/*ע����ռ��������Ϣ���߳�*/
static inline long KMSetRecv(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = KBDMUS_API_SETRECV;
	return KSendMsg(ptid, data, 0);
}

/**********VESA�Կ������������**********/
#define SRV_VESA_PORT	4	/*VESA�Կ�����˿�*/
#define VESA_MAX_MODE	512	/*��ʾģʽ�б��������*/

#define VESA_API_CURMODE	0	/*ȡ�õ�ǰ��ʾģʽ���ܺ�*/
#define VESA_API_GETMODE	1	/*ȡ��ģʽ�б��ܺ�*/
#define VESA_API_PUTPIXEL	2	/*���㹦�ܺ�*/
#define VESA_API_GETPIXEL	3	/*ȡ�㹦�ܺ�*/
#define VESA_API_PUTIMAGE	4	/*��ͼ���ܺ�*/
#define VESA_API_GETIMAGE	5	/*��ͼ���ܺ�*/
#define VESA_API_FILLRECT	6	/*�����ι��ܺ�*/
#define VESA_API_DRAWLINE	7	/*���߹��ܺ�*/
#define VESA_API_CIRCLE		8	/*��Բ���ܺ�*/
#define VESA_API_DRAWSTR	9	/*����ַ������ܺ�*/

#define VESA_ERR_LOCATION	-1280	/*�������*/
#define VESA_ERR_SIZE		-1281	/*�ߴ����*/
#define VESA_ERR_ARGS		-1282	/*��������*/

/*ȡ�õ�ǰ��ʾģʽ*/
static inline long VSCurMode(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = VESA_API_CURMODE;
	if ((data[0] = KSendMsg(ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[1];
}

/*ȡ��ģʽ�б�*/
static inline long VSGetMode(THREAD_ID ptid, WORD mode[VESA_MAX_MODE])
{
	DWORD data[MSG_DATA_LEN];
	data[0] = VESA_API_GETMODE;
	if ((data[0] = KReadProcAddr(mode, VESA_MAX_MODE * sizeof(WORD), ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*����*/
static inline long VSPutPixel(THREAD_ID ptid, long x, long y, long c)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = x;
	data[2] = y;
	data[3] = VESA_API_PUTPIXEL;
	data[4] = c;
	return KSendMsg(ptid, data, 0);
}

/*ȡ��*/
static inline long VSGetPixel(THREAD_ID ptid, long x, long y)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = x;
	data[2] = y;
	data[3] = VESA_API_GETPIXEL;
	if ((data[0] = KSendMsg(ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[1] != NO_ERROR)
		return data[1];
	return data[2];
}

/*��ͼ*/
static inline long VSPutImage(THREAD_ID ptid, long x, long y, DWORD *img, long w, long h)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = VESA_API_PUTIMAGE;
	data[1] = x;
	data[2] = y;
	data[3] = w;
	data[4] = h;
	if ((data[0] = KWriteProcAddr(img, w * h * sizeof(DWORD), ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*��ͼ*/
static inline long VSGetImage(THREAD_ID ptid, long x, long y, DWORD *img, long w, long h)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = VESA_API_GETIMAGE;
	data[1] = x;
	data[2] = y;
	data[3] = w;
	data[4] = h;
	if ((data[0] = KReadProcAddr(img, w * h * sizeof(DWORD), ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*������*/
static inline long VSFillRect(THREAD_ID ptid, long x, long y, long w, long h, DWORD c)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = x;
	data[2] = y;
	data[3] = VESA_API_FILLRECT;
	data[4] = w;
	data[5] = h;
	data[6] = c;
	return KSendMsg(ptid, data, 0);
}

/*����*/
static inline long VSDrawLine(THREAD_ID ptid, long x1, long y1, long x2, long y2, DWORD c)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = x1;
	data[2] = y1;
	data[3] = VESA_API_DRAWLINE;
	data[4] = x2;
	data[5] = y2;
	data[6] = c;
	return KSendMsg(ptid, data, 0);
}

/*��Բ*/
static inline long VSCircle(THREAD_ID ptid, long cx, long cy, long r, long c)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = cx;
	data[2] = cy;
	data[3] = VESA_API_CIRCLE;
	data[4] = r;
	data[5] = c;
	return KSendMsg(ptid, data, 0);
}

/*����ַ���*/
static inline long VSDrawStr(THREAD_ID ptid, long x, long y, const BYTE *str, DWORD c)
{
	DWORD data[MSG_DATA_LEN];
	BYTE buf[1024];
	data[0] = VESA_API_DRAWSTR;
	data[1] = x;
	data[2] = y;
	data[3] = c;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, str) + 1 - buf, ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[2];
}

#endif

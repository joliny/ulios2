/*	bsrvapi.h for ulios driver
	���ߣ�����
	���ܣ�ulios��������API�ӿڶ��壬���û���������Ҫ�������ļ�
	����޸����ڣ�2010-03-09
*/

#ifndef	_BSRVAPI_H_
#define	_BSRVAPI_H_

#include "../MkApi/ulimkapi.h"

#define SRV_OUT_TIME	6000	/*������ó�ʱ������INVALID:���޵ȴ�*/

/**********�����쳣����**********/
#define SRV_REP_PORT	0	/*�����쳣�������˿�*/

/**********ATӲ�����**********/
#define SRV_ATHD_PORT	2	/*ATӲ����������˿�*/
#define ATHD_BPS		512	/*����ÿ�����ֽ���*/

#define ATHD_API_WRITESECTOR	0	/*дӲ���������ܺ�*/
#define ATHD_API_READSECTOR		1	/*��Ӳ���������ܺ�*/

#define ATHD_ERR_WRONG_DRV		-512	/*�������������*/
#define ATHD_ERR_HAVENO_REQ		-513	/*�޷����ܸ���ķ�������*/

/*дӲ������*/
static inline long HDWriteSector(THREAD_ID ptid, DWORD drv, DWORD sec, BYTE cou, void *buf)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = ATHD_API_WRITESECTOR;
	data[3] = drv;
	data[4] = sec;
	if ((data[0] = KWriteProcAddr(buf, ATHD_BPS * cou, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*��Ӳ������*/
static inline long HDReadSector(THREAD_ID ptid, DWORD drv, DWORD sec, BYTE cou, void *buf)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = ATHD_API_READSECTOR;
	data[3] = drv;
	data[4] = sec;
	if ((data[0] = KReadProcAddr(buf, ATHD_BPS * cou, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/**********ʱ��������**********/
#define SRV_TIME_PORT	3	/*ʱ�����˿�*/

#define MSG_ATTR_TIME	0x01030000	/*ʱ�������Ϣ*/

#define TIME_API_CURSECOND		0	/*ȡ��1970�꾭�����빦�ܺ�*/
#define TIME_API_CURTIME		1	/*ȡ�õ�ǰʱ�书�ܺ�*/
#define TIME_API_MKTIME			2	/*TM�ṹת��Ϊ�빦�ܺ�*/
#define TIME_API_LOCALTIME		3	/*��ת��ΪTM�ṹ���ܺ�*/
#define TIME_API_GETRAND		4	/*ȡ����������ܺ�*/

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
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_CURSECOND;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*sec = data[1];
	return NO_ERROR;
}

/*ȡ�õ�ǰʱ��*/
static inline long TMCurTime(THREAD_ID ptid, TM *tm)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_CURTIME;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	memcpy32(tm, &data[1], sizeof(TM) / sizeof(DWORD));
	return NO_ERROR;
}

/*TM�ṹת��Ϊ��*/
static inline long TMMkTime(THREAD_ID ptid, DWORD *sec, const TM *tm)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_MKTIME;
	memcpy32(&data[1], tm, sizeof(TM) / sizeof(DWORD));
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*sec = data[1];
	return NO_ERROR;
}

/*��ת��ΪTM�ṹ*/
static inline long TMLocalTime(THREAD_ID ptid, DWORD sec, TM *tm)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_LOCALTIME;
	data[1] = sec;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	memcpy32(tm, &data[1], sizeof(TM) / sizeof(DWORD));
	return NO_ERROR;
}

/*ȡ�������*/
static inline long TMGetRand(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_TIME | TIME_API_GETRAND;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[1];
}

/**********�������������**********/
#define SRV_KBDMUS_PORT	4	/*����������˿�*/

#define MSG_ATTR_KBDMUS	0x01040000	/*���������Ϣ*/
#define MSG_ATTR_KBD	0x01040001	/*���̰�����Ϣ*/
#define MSG_ATTR_MUS	0x01040002	/*���״̬��Ϣ*/

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

/*ע����ռ��������Ϣ���߳�*/
static inline long KMSetRecv(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_KBDMUS | KBDMUS_API_SETRECV;
	return KSendMsg(&ptid, data, 0);
}

/**********VESA�Կ����������GDI�����**********/
#define SRV_VESA_PORT	5	/*VESA�Կ�����˿�*/
#define VESA_MAX_MODE	512	/*��ʾģʽ�б��������*/

#define MSG_ATTR_VESA	0x01050000	/*VESA�Կ���Ϣ*/

#define VESA_API_GETVMEM	0	/*ȡ���Դ�ӳ�书�ܺ�*/
#define VESA_API_GETFONT	1	/*ȡ������ӳ�书�ܺ�*/
#define VESA_API_GETMODE	2	/*ȡ��ģʽ�б��ܺ�*/

#define VESA_ERR_ARGS		-1280	/*��������*/
#define VESA_ERR_TEXTMODE	-1281	/*�ı�ģʽ*/

/*ȡ���Դ�ӳ��*/
static inline long VSGetVmem(THREAD_ID ptid, void **vm, DWORD *width, DWORD *height, DWORD *PixBits)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_VESA | VESA_API_GETVMEM;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*vm = (void*)data[MSG_ADDR_ID];
	*width = data[3];
	*height = data[4];
	*PixBits = data[5];
	return NO_ERROR;
}

/*ȡ������ӳ��*/
static inline long VSGetFont(THREAD_ID ptid, const BYTE **font, DWORD *CharWidth, DWORD *CharHeight)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_VESA | VESA_API_GETFONT;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*font = (const BYTE*)data[MSG_ADDR_ID];
	*CharWidth = data[3];
	*CharHeight = data[4];
	return NO_ERROR;
}

/*ȡ��ģʽ�б�*/
static inline long VSGetMode(THREAD_ID ptid, WORD mode[VESA_MAX_MODE], DWORD *ModeCou, DWORD *CurMode)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = VESA_API_GETMODE;
	if ((data[0] = KReadProcAddr(mode, VESA_MAX_MODE * sizeof(WORD), &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	*ModeCou = data[MSG_SIZE_ID];
	*CurMode = data[3];
	return NO_ERROR;
}

/**********CUI�ַ�����������**********/
#define SRV_CUI_PORT	6	/*CUI�ַ��������˿�*/

#define MSG_ATTR_CUI	0x01060000	/*CUI�ַ�������Ϣ*/

#define CUI_API_GETCOL	0	/*ȡ���ַ�������ɫ���ܺ�*/
#define CUI_API_SETCOL	1	/*�����ַ�������ɫ���ܺ�*/
#define CUI_API_GETCUR	2	/*ȡ�ù��λ�ù��ܺ�*/
#define CUI_API_SETCUR	3	/*���ù��λ�ù��ܺ�*/
#define CUI_API_CLRSCR	4	/*�������ܺ�*/
#define CUI_API_PUTC	5	/*����ַ����ܺ�*/
#define CUI_API_PUTS	6	/*����ַ������ܺ�*/

#define CUI_ERR_ARGS	-1536	/*��������*/

/*ȡ���ַ�������ɫ*/
static inline long CUIGetCol(THREAD_ID ptid, DWORD *CharColor, DWORD *BgColor)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_GETCOL;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*CharColor = data[1];
	*BgColor = data[2];
	return NO_ERROR;
}

/*�����ַ�������ɫ*/
static inline long CUISetCol(THREAD_ID ptid, DWORD CharColor, DWORD BgColor)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_SETCOL;
	data[1] = CharColor;
	data[2] = BgColor;
	return KSendMsg(&ptid, data, 0);
}

/*ȡ�ù��λ��*/
static inline long CUIGetCur(THREAD_ID ptid, DWORD *CursX, DWORD *CursY)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_GETCUR;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*CursX = data[1];
	*CursY = data[2];
	return NO_ERROR;
}

/*���ù��λ��*/
static inline long CUISetCur(THREAD_ID ptid, DWORD CursX, DWORD CursY)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_SETCUR;
	data[1] = CursX;
	data[2] = CursY;
	return KSendMsg(&ptid, data, 0);
}

/*����*/
static inline long CUIClrScr(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_CLRSCR;
	return KSendMsg(&ptid, data, 0);
}

/*����ַ�*/
static inline long CUIPutC(THREAD_ID ptid, char c)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_CUI | CUI_API_PUTC;
	data[1] = c;
	return KSendMsg(&ptid, data, 0);
}

/*����ַ���*/
static inline long CUIPutS(THREAD_ID ptid, const char *str)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = CUI_API_PUTS;
	if ((data[0] = KWriteProcAddr((void*)str, strlen(str) + 1, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/**********ϵͳ���ȷ������**********/
#define SRV_SPK_PORT	7	/*ϵͳ���ȷ���˿�*/

#define MSG_ATTR_SPK	0x01070000	/*ϵͳ������Ϣ*/

#define SPK_API_SOUND	0	/*�������ܺ�*/
#define SPK_API_NOSOUND	1	/*ֹͣ�������ܺ�*/

/*����*/
static inline long SPKSound(THREAD_ID ptid, DWORD freq)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_SPK | SPK_API_SOUND;
	data[1] = freq;
	return KSendMsg(&ptid, data, 0);
}

/*ֹͣ����*/
static inline long SPKNosound(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_SPK | SPK_API_NOSOUND;
	return KSendMsg(&ptid, data, 0);
}

/**********COM���ڷ������**********/
#define SRV_UART_PORT	8	/*COM���ڷ���˿�*/

#define MSG_ATTR_UART	0x01080000	/*COM������Ϣ*/

#define UART_API_OPENCOM	0	/*�򿪴��ڹ��ܺ�*/
#define UART_API_CLOSECOM	1	/*�رմ��ڹ��ܺ�*/
#define UART_API_WRITECOM	2	/*д���ڹ��ܺ�*/
#define UART_API_READCOM	3	/*�����ڹ��ܺ�*/

#define UART_ARGS_BITS_5		0x00	/*5������λ*/
#define UART_ARGS_BITS_6		0x01	/*6������λ*/
#define UART_ARGS_BITS_7		0x02	/*7������λ*/
#define UART_ARGS_BITS_8		0x03	/*8������λ*/
#define UART_ARGS_STOP_1		0x00	/*1��ֹͣλ*/
#define UART_ARGS_STOP_1_5		0x04	/*5������λʱ1.5��ֹͣλ*/
#define UART_ARGS_STOP_2		0x04	/*2��ֹͣλ*/
#define UART_ARGS_PARITY_NONE	0x00	/*����żУ��*/
#define UART_ARGS_PARITY_ODD	0x08	/*��λУ��*/
#define UART_ARGS_PARITY_EVEN	0x18	/*żλУ��*/

#define UART_ERR_NOPORT	-2048	/*COM�˿ڲ�����*/
#define UART_ERR_BAUD	-2049	/*�����ʴ���*/
#define UART_ERR_NOTIME	-2050	/*��ʱ����*/
#define UART_ERR_BUSY	-2051	/*�˿�������æ*/

/*�򿪴���*/
static inline long UARTOpenCom(THREAD_ID ptid, DWORD com, DWORD baud, DWORD args)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_UART | UART_API_OPENCOM;
	data[1] = com;
	data[2] = baud;
	data[3] = args;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*�رմ���*/
static inline long UARTCloseCom(THREAD_ID ptid, DWORD com)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = MSG_ATTR_UART | UART_API_CLOSECOM;
	data[1] = com;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*д����*/
static inline long UARTWriteCom(THREAD_ID ptid, DWORD com, void *buf, DWORD siz, DWORD time)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = UART_API_WRITECOM;
	data[3] = com;
	data[4] = time;
	if ((data[0] = KWriteProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*������*/
static inline long UARTReadCom(THREAD_ID ptid, DWORD com, void *buf, DWORD siz, DWORD time)
{
	DWORD data[MSG_DATA_LEN];
	data[MSG_API_ID] = UART_API_READCOM;
	data[3] = com;
	data[4] = time;
	if ((data[0] = KReadProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

#endif

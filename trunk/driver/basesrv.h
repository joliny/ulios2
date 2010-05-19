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
#define ATHD_OUT_TIME	100	/*��ʱ������INVALID:���޵ȴ�*/

#define ATHD_API_READSECTOR		0	/*��Ӳ���������ܺ�*/
#define ATHD_API_WRITESECTOR	1	/*дӲ���������ܺ�*/

#define ATHD_ERR_WRONG_DRV		-100	/*�������������*/
#define ATHD_ERR_HAVENO_REQ		-101	/*�޷����ܸ���ķ�������*/

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

#define TIME_ERR_WRONG_TM		-200	/*�Ƿ�TM�ṹ*/

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

#endif

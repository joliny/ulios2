/*	bsrvapi.h for ulios driver
	作者：孙亮
	功能：ulios基础服务API接口定义，调用基础服务需要包含此文件
	最后修改日期：2010-03-09
*/

#ifndef	_BSRVAPI_H_
#define	_BSRVAPI_H_

#include "../MkApi/ulimkapi.h"

#define SRV_OUT_TIME	100	/*服务调用超时厘秒数INVALID:无限等待*/

/**********AT硬盘相关**********/
#define SRV_ATHD_PORT	1	/*AT硬盘驱动服务端口*/
#define ATHD_BPS		512	/*磁盘每扇区字节数*/
#define ATHD_OUT_TIME	100	/*超时厘秒数INVALID:无限等待*/

#define ATHD_API_READSECTOR		0	/*读硬盘扇区功能号*/
#define ATHD_API_WRITESECTOR	1	/*写硬盘扇区功能号*/

#define ATHD_ERR_WRONG_DRV		-100	/*错误的驱动器号*/
#define ATHD_ERR_HAVENO_REQ		-101	/*无法接受更多的服务请求*/

/*读硬盘扇区*/
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

/*写硬盘扇区*/
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

/**********时间服务相关**********/
#define SRV_TIME_PORT	2	/*时间服务端口*/

#define TIME_API_CURSECOND		0	/*取得1970年经过的秒功能号*/
#define TIME_API_CURTIME		1	/*取得当前时间功能号*/
#define TIME_API_MKTIME			2	/*TM结构转换为秒功能号*/
#define TIME_API_LOCALTIME		3	/*秒转换为TM结构功能号*/

#define TIME_ERR_WRONG_TM		-200	/*非法TM结构*/

typedef struct _TM
{
	BYTE sec;	/*秒[0,59]*/
	BYTE min;	/*分[0,59]*/
	BYTE hor;	/*时[0,23]*/
	BYTE day;	/*日[1,31]*/
	BYTE mon;	/*月[1,12]*/
	BYTE wday;	/*星期[0,6]*/
	WORD yday;	/*一年中的第几天[0,365]*/
	WORD yer;	/*年[1970,...]*/
	WORD mil;	/*毫秒[0,999]*/
}TM;	/*时间结构*/

/*取得1970年经过的秒*/
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

/*取得当前时间*/
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

/*TM结构转换为秒*/
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

/*秒转换为TM结构*/
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

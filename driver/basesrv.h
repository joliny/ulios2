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
#define ATHD_OUT_TIME	6000	/*超时厘秒数INVALID:无限等待*/

#define ATHD_API_READSECTOR		0	/*读硬盘扇区功能号*/
#define ATHD_API_WRITESECTOR	1	/*写硬盘扇区功能号*/

#define ATHD_ERR_WRONG_DRV		-512	/*错误的驱动器号*/
#define ATHD_ERR_HAVENO_REQ		-513	/*无法接受更多的服务请求*/

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

#define TIME_ERR_WRONG_TM		-768	/*非法TM结构*/

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

/**********键盘鼠标服务相关**********/
#define SRV_KBDMUS_PORT	3	/*键盘鼠标服务端口*/

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

#define KBDMUS_API_SETRECV		0	/*注册接收键盘鼠标消息的线程功能号*/

#define MSG_ATTR_KBD	(MSG_ATTR_USER)		/*键盘按键消息*/
#define MSG_ATTR_MUS	(MSG_ATTR_USER + 1)	/*鼠标状态消息*/

/*注册接收键盘鼠标消息的线程*/
static inline long KMSetRecv(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = KBDMUS_API_SETRECV;
	return KSendMsg(ptid, data, 0);
}

/**********VESA显卡驱动服务相关**********/
#define SRV_VESA_PORT	4	/*VESA显卡服务端口*/
#define VESA_MAX_MODE	512	/*显示模式列表最大数量*/

#define VESA_API_CURMODE	0	/*取得当前显示模式功能号*/
#define VESA_API_GETMODE	1	/*取得模式列表功能号*/
#define VESA_API_PUTPIXEL	2	/*画点功能号*/
#define VESA_API_GETPIXEL	3	/*取点功能号*/
#define VESA_API_PUTIMAGE	4	/*贴图功能号*/
#define VESA_API_GETIMAGE	5	/*截图功能号*/
#define VESA_API_FILLRECT	6	/*填充矩形功能号*/
#define VESA_API_DRAWLINE	7	/*画线功能号*/
#define VESA_API_CIRCLE		8	/*画圆功能号*/
#define VESA_API_DRAWSTR	9	/*输出字符串功能号*/

#define VESA_ERR_LOCATION	-1280	/*坐标错误*/
#define VESA_ERR_SIZE		-1281	/*尺寸错误*/
#define VESA_ERR_ARGS		-1282	/*参数错误*/

/*取得当前显示模式*/
static inline long VSCurMode(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = VESA_API_CURMODE;
	if ((data[0] = KSendMsg(ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[1];
}

/*取得模式列表*/
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

/*画点*/
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

/*取点*/
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

/*贴图*/
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

/*截图*/
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

/*填充矩形*/
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

/*画线*/
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

/*画圆*/
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

/*输出字符串*/
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

/*	bsrvapi.h for ulios driver
	作者：孙亮
	功能：ulios基础服务API接口定义，调用基础服务需要包含此文件
	最后修改日期：2010-03-09
*/

#ifndef	_BSRVAPI_H_
#define	_BSRVAPI_H_

#include "../MkApi/ulimkapi.h"

#define SRV_OUT_TIME	6000	/*服务调用超时厘秒数INVALID:无限等待*/

/**********进程异常报告**********/
#define SRV_REP_PORT	1	/*进程异常报告服务端口*/

/**********AT硬盘相关**********/
#define SRV_ATHD_PORT	2	/*AT硬盘驱动服务端口*/
#define ATHD_BPS		512	/*磁盘每扇区字节数*/

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
	if ((data[0] = KReadProcAddr(buf, ATHD_BPS * cou, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
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
	if ((data[0] = KWriteProcAddr(buf, ATHD_BPS * cou, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[2];
}

/**********时间服务相关**********/
#define SRV_TIME_PORT	3	/*时间服务端口*/

#define TIME_API_CURSECOND		0	/*取得1970年经过的秒功能号*/
#define TIME_API_CURTIME		1	/*取得当前时间功能号*/
#define TIME_API_MKTIME			2	/*TM结构转换为秒功能号*/
#define TIME_API_LOCALTIME		3	/*秒转换为TM结构功能号*/
#define TIME_API_GETRAND		4	/*取得随机数功能号*/

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
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
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
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
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
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
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
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	memcpy32(tm, &data[1], sizeof(TM) / sizeof(DWORD));
	return NO_ERROR;
}

/*取得随机数*/
static inline long TMGetRand(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = TIME_API_GETRAND;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[1];
}

/**********键盘鼠标服务相关**********/
#define SRV_KBDMUS_PORT	4	/*键盘鼠标服务端口*/

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

#define MSG_ATTR_KBD	(MSG_ATTR_USER + 0x40000)	/*键盘按键消息*/
#define MSG_ATTR_MUS	(MSG_ATTR_USER + 0x40001)	/*鼠标状态消息*/

/*注册接收键盘鼠标消息的线程*/
static inline long KMSetRecv(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = KBDMUS_API_SETRECV;
	return KSendMsg(&ptid, data, 0);
}

/**********VESA显卡驱动服务和GDI库相关**********/
#define SRV_VESA_PORT	5	/*VESA显卡服务端口*/
#define VESA_MAX_MODE	512	/*显示模式列表最大数量*/

#define VESA_API_GETVMEM	0	/*取得显存映射功能号*/
#define VESA_API_GETFONT	1	/*取得字体映射功能号*/
#define VESA_API_GETMODE	2	/*取得模式列表功能号*/

#define VESA_ERR_ARGS		-1280	/*参数错误*/
#define VESA_ERR_TEXTMODE	-1281	/*文本模式*/

/*取得显存映射*/
static inline long VSGetVmem(THREAD_ID ptid, void **vm, DWORD *width, DWORD *height, DWORD *PixBits)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = VESA_API_GETVMEM;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[3] != NO_ERROR)
		return data[3];
	*vm = (void*)data[2];
	*width = data[4];
	*height = data[5];
	*PixBits = data[6];
	return NO_ERROR;
}

/*取得字体映射*/
static inline long VSGetFont(THREAD_ID ptid, const BYTE **font, DWORD *CharWidth, DWORD *CharHeight)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = VESA_API_GETFONT;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[3] != NO_ERROR)
		return data[3];
	*font = (const BYTE*)data[2];
	*CharWidth = data[4];
	*CharHeight = data[5];
	return NO_ERROR;
}

/*取得模式列表*/
static inline long VSGetMode(THREAD_ID ptid, WORD mode[VESA_MAX_MODE], DWORD *ModeCou, DWORD *CurMode)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = VESA_API_GETMODE;
	if ((data[0] = KReadProcAddr(mode, VESA_MAX_MODE * sizeof(WORD), &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	*ModeCou = data[3];
	*CurMode = data[5];
	return NO_ERROR;
}

/**********CUI字符界面服务相关**********/
#define SRV_CUI_PORT	6	/*CUI字符界面服务端口*/

#define CUI_API_GETCOL	0	/*取得字符界面颜色功能号*/
#define CUI_API_SETCOL	1	/*设置字符界面颜色功能号*/
#define CUI_API_GETCUR	2	/*取得光标位置功能号*/
#define CUI_API_SETCUR	3	/*设置光标位置功能号*/
#define CUI_API_CLRSCR	4	/*清屏功能号*/
#define CUI_API_PUTC	5	/*输出字符功能号*/
#define CUI_API_PUTS	6	/*输出字符串功能号*/

#define CUI_ERR_ARGS	-1536	/*参数错误*/

/*取得字符界面颜色*/
static inline long CUIGetCol(THREAD_ID ptid, DWORD *CharColor, DWORD *BgColor)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = CUI_API_GETCOL;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*CharColor = data[1];
	*BgColor = data[2];
	return NO_ERROR;
}

/*设置字符界面颜色*/
static inline long CUISetCol(THREAD_ID ptid, DWORD CharColor, DWORD BgColor)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = CharColor;
	data[2] = BgColor;
	data[3] = CUI_API_SETCOL;
	return KSendMsg(&ptid, data, 0);
}

/*取得光标位置*/
static inline long CUIGetCur(THREAD_ID ptid, DWORD *CursX, DWORD *CursY)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = CUI_API_GETCUR;
	if ((data[0] = KSendMsg(&ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	*CursX = data[1];
	*CursY = data[2];
	return NO_ERROR;
}

/*设置光标位置*/
static inline long CUISetCur(THREAD_ID ptid, DWORD CursX, DWORD CursY)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = CursX;
	data[2] = CursY;
	data[3] = CUI_API_SETCUR;
	return KSendMsg(&ptid, data, 0);
}

/*清屏*/
static inline long CUIClrScr(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = CUI_API_CLRSCR;
	return KSendMsg(&ptid, data, 0);
}

/*输出字符*/
static inline long CUIPutC(THREAD_ID ptid, char c)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = c;
	data[3] = CUI_API_PUTC;
	return KSendMsg(&ptid, data, 0);
}

/*输出字符串*/
static inline long CUIPutS(THREAD_ID ptid, const char *str)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = CUI_API_PUTS;
	if ((data[0] = KWriteProcAddr((void*)str, strlen(str) + 1, &ptid, data, SRV_OUT_TIME)) != NO_ERROR)
		return data[0];
	return data[2];
}

/**********系统喇叭服务相关**********/
#define SRV_SPK_PORT	7	/*系统喇叭服务端口*/

#define SPK_API_SOUND	0	/*发声功能号*/
#define SPK_API_NOSOUND	1	/*停止发声功能号*/

/*发声*/
static inline long SPKSound(THREAD_ID ptid, DWORD freq)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = SPK_API_SOUND;
	data[2] = freq;
	return KSendMsg(&ptid, data, 0);
}

/*停止发声*/
static inline long SPKNosound(THREAD_ID ptid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = SPK_API_NOSOUND;
	return KSendMsg(&ptid, data, 0);
}

/**********COM串口服务相关**********/
#define SRV_UART_PORT	8	/*COM串口服务端口*/

#define UART_API_OPENCOM	0	/*打开串口功能号*/
#define UART_API_CLOSECOM	1	/*关闭串口功能号*/
#define UART_API_READCOM	2	/*读串口功能号*/
#define UART_API_WRITECOM	3	/*写串口功能号*/

#define UART_ARGS_BITS_5		0x00	/*5个数据位*/
#define UART_ARGS_BITS_6		0x01	/*6个数据位*/
#define UART_ARGS_BITS_7		0x02	/*7个数据位*/
#define UART_ARGS_BITS_8		0x03	/*8个数据位*/
#define UART_ARGS_STOP_1		0x00	/*1个停止位*/
#define UART_ARGS_STOP_1_5		0x04	/*5个数据位时1.5个停止位*/
#define UART_ARGS_STOP_2		0x04	/*2个停止位*/
#define UART_ARGS_PARITY_NONE	0x00	/*无奇偶校验*/
#define UART_ARGS_PARITY_ODD	0x08	/*奇位校验*/
#define UART_ARGS_PARITY_EVEN	0x18	/*偶位校验*/

#define UART_ERR_NOPORT	-2048	/*COM端口不存在*/
#define UART_ERR_BAUD	-2049	/*波特率错误*/
#define UART_ERR_NOTIME	-2050	/*超时错误*/
#define UART_ERR_BUSY	-2051	/*端口驱动正忙*/

/*打开串口*/
static inline long UARTOpenCom(THREAD_ID ptid, DWORD com, DWORD baud, DWORD args)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = com;
	data[2] = baud;
	data[3] = UART_API_OPENCOM;
	data[4] = args;
	return KSendMsg(&ptid, data, 0);
}

/*关闭串口*/
static inline long UARTCloseCom(THREAD_ID ptid, DWORD com)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[1] = com;
	data[3] = UART_API_CLOSECOM;
	return KSendMsg(&ptid, data, 0);
}

/*读串口*/
static inline long UARTReadCom(THREAD_ID ptid, DWORD com, void *buf, DWORD siz, DWORD time)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = UART_API_READCOM;
	data[1] = com;
	data[2] = time;
	if ((data[0] = KReadProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*写串口*/
static inline long UARTWriteCom(THREAD_ID ptid, DWORD com, void *buf, DWORD siz, DWORD time)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = UART_API_WRITECOM;
	data[1] = com;
	data[2] = time;
	if ((data[0] = KWriteProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

#endif

/*	kbdmus.c for ulios driver
	作者：孙亮
	功能：8042键盘鼠标控制器驱动服务程序
	最后修改日期：2010-05-18
*/

#include "basesrv.h"

#define KBD_IRQ	0x1	/*键盘中断请求号*/
#define MUS_IRQ	0xC	/*鼠标中断请求号*/

/*键盘控制器等待*/
static inline void Wait8042()
{
	while (inb(0x64) & 0x02);
}

/*键盘控制器回复*/
static inline void Ack8042()
{
	while (inb(0x60) != 0xFA);
}

BYTE KeyMap[] = {  0,	/*美国英语键盘主键码表*/
	27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,	/*(15-28)TAB--ENTER*/
	0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'',			/*(29-40)CTRL--'*/
	'`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	/*(41-54)`--RSHIFT*/
	0,   0,  ' ',  0,													/*(55-58)PrtSc--CapsLock*/
	2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0,   0,			/*(59-70)F1--ScrLock*/
	1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   1,		/*(71-83)HOME--DEL*/
	0,   0,   0,   2,   2,   0,   0,   2,   2,   2						/*(84-93)F11--APPS*/
};

BYTE KeyMapEx[] = {0,	/*美国英语键盘扩展键码表*/
	27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13,	/*(15-28)TAB--ENTER*/
	0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',			/*(29-40)<CTRL>*/
	'~', 0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,	/*(41-54)<LSHIFT><RSHIFT>*/
	0,   0,  ' ',  0,													/*(55-58)<ALT><CapsLock>*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,			/*(59-70)<NumLock><ScrLock>*/
	'7','8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',		/*(71-83)HOME--DEL*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0						/*(84-93)F11--APPS*/
};

#define VS_MODE_ADDR	0x90500	/*显卡模式信息物理地址*/

int main()
{
	DWORD code;	/*端口取码*/
	DWORD KbdFlag, KbdState;	/*键码扫描码,前缀标志,控制键状态*/
	DWORD MusId, MusCou, MusKey;		/*鼠标ID,鼠标数据包计数,键状态*/
	long MusX, MusY, MusZ, MusMaxX, MusMaxY;	/*移动范围,位置*/
	THREAD_ID RecvPtid;	/*接受键盘鼠标消息的线程ID*/
	void *addr;
	long res;	/*返回结果*/

	if ((res = KRegKnlPort(SRV_KBDMUS_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	/*键盘初始化*/
	Wait8042();
	outb(0x60, 0xED);	/*命令:设置LED灯*/
	Ack8042();
	outb(0x60, 0x00);	/*设置LED状态:全部关闭*/
	Ack8042();
	/*鼠标初始化*/
	Wait8042();
	outb(0x64, 0xA8);	/*允许鼠标接口*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0xF4);	/*允许鼠标发数据*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0xF3);	/*设置鼠标采样率,开启三维模式*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0xC8);	/*采样率200*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0xF3);	/*设置鼠标采样率,开启三维模式*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0x64);	/*采样率100*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0xF3);	/*设置鼠标采样率,开启三维模式*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0x50);	/*采样率80*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	outb(0x60, 0xF2);	/*取得鼠标ID*/
	Ack8042();
	MusId = inb(0x60);	/*取得鼠标ID*/
	Wait8042();
	outb(0x64, 0x60);	/*通知8042数据将要发向8042命令寄存器*/
	outb(0x60, 0x47);	/*许可键盘,鼠标接口及中断*/
	if ((res = KRegIrq(KBD_IRQ)) != NO_ERROR)	/*注册键盘中断请求号*/
		return res;
	if ((res = KRegIrq(MUS_IRQ)) != NO_ERROR)	/*注册鼠标中断请求号*/
		return res;
	if ((res = KMapPhyAddr(&addr, VS_MODE_ADDR, 0x100)) != NO_ERROR)	/*从显卡启动数据里获得屏幕像素数*/
		return res;
	MusMaxX = *((WORD*)(addr + 18)) - 1;
	MusMaxY = *((WORD*)(addr + 20)) - 1;
	KFreeAddr(addr);
	KbdState = KbdFlag = 0;
	MusKey = MusCou = 0;
	MusZ = MusY = MusX = 0;
	*((DWORD*)&RecvPtid) = INVALID;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (data[0] == MSG_ATTR_IRQ)
		{
			code = inb(0x60);
			if (data[1] == KBD_IRQ)	/*键盘中断消息*/
			{
				if (code == 0xE0)	/*前缀,后面跟1个键码*/
				{
					KbdFlag = 1;
					continue;
				}
				if (code == 0xE1)	/*前缀,后面跟2个键码*/
				{
					KbdFlag = 2;
					continue;
				}
				if (KbdFlag == 2)
				{
					if (code == 0xC5)	/*pause键按下*/
					{
						KbdState |= KBD_STATE_PAUSE;
						KbdFlag = 0;
					}
					continue;
				}
				if (KbdState & KBD_STATE_PAUSE)	/*pause键退出*/
				{
					KbdState &= (~KBD_STATE_PAUSE);
					continue;
				}
				if (code & 0x80)	/*松开键*/
				{
					code &= 0x7F;
					if (code < sizeof(KeyMap) && KeyMap[code] == 0)	/*松开控制键*/
					{
						switch (code)
						{
						case 29:
							if (KbdFlag)
								KbdState &= (~KBD_STATE_RCTRL);	/*右ctrl*/
							else
								KbdState &= (~KBD_STATE_LCTRL);	/*左ctrl*/
							break;
						case 54:
							KbdState &= (~KBD_STATE_RSHIFT);	/*右shift*/
							break;
						case 42:
							if (!KbdFlag)
								KbdState &= (~KBD_STATE_LSHIFT);	/*左shift*/
							break;
						case 56:
							if (KbdFlag)
								KbdState &= (~KBD_STATE_RALT);	/*右alt*/
							else
								KbdState &= (~KBD_STATE_LALT);	/*左alt*/
							break;
						case 55:
							KbdState &= (~KBD_STATE_PRTSC);	/*PrtSc*/
							break;
						case 82:
							KbdState &= (~(KBD_STATE_INSERT << 4));	/*Insert*/
							break;
						case 58:
							KbdState &= (~(KBD_STATE_CAPSLOCK << 4));	/*CapsLock*/
							break;
						case 69:
							KbdState &= (~(KBD_STATE_NUMLOCK << 4));	/*NumLock*/
							break;
						case 70:
							KbdState &= (~(KBD_STATE_SCRLOCK << 4));	/*ScrLock*/
							break;
						}
					}
				}
				else	/*按下键*/
				{
					DWORD KbdKey;

					if (code < sizeof(KeyMap))
					{
						if (KeyMap[code] > 2)	/*主键盘键*/
						{
							if (KeyMap[code] >= 'a' && KeyMap[code] <= 'z')	/*按了字母键*/
							{
								if (((KbdState & KBD_STATE_CAPSLOCK) != 0) ^ ((KbdState & (KBD_STATE_LSHIFT | KBD_STATE_RSHIFT)) != 0))	/*caps开或shift按下其一符合*/
									KbdKey = KeyMapEx[code];
								else
									KbdKey = KeyMap[code];
							}
							else if (KbdState & (KBD_STATE_LSHIFT | KBD_STATE_RSHIFT))	/*有shift按下*/
								KbdKey = KeyMapEx[code];
							else
								KbdKey = KeyMap[code];
						}
						else if (KeyMap[code] == 2)	/*功能键*/
							KbdKey = code << 8;
						else if (KeyMap[code] == 1)	/*小键盘*/
						{
							if (KbdState & KBD_STATE_NUMLOCK)	/*NumLock开*/
								KbdKey = KeyMapEx[code];
							else
								KbdKey = code << 8;
						}
						else	/*控制键*/
						{
							KbdKey = 0;
							switch (code)
							{
							case 29:
								if (KbdFlag)
									KbdState |= KBD_STATE_RCTRL;	/*右ctrl*/
								else
									KbdState |= KBD_STATE_LCTRL;	/*左ctrl*/
								break;
							case 54:
								KbdState |= KBD_STATE_RSHIFT;	/*右shift*/
								break;
							case 42:
								if (!KbdFlag)
									KbdState |= KBD_STATE_LSHIFT;	/*左shift*/
								break;
							case 56:
								if (KbdFlag)
									KbdState |= KBD_STATE_RALT;	/*右alt*/
								else
									KbdState |= KBD_STATE_LALT;	/*左alt*/
								break;
							case 55:	/*PrtSc*/
								KbdState |= KBD_STATE_PRTSC;
								KbdKey = code << 8;
								break;
							case 82:	/*Insert*/
								if (KbdState & KBD_STATE_NUMLOCK)	/*numlock开*/
									KbdKey = KeyMapEx[code];
								else if (!(KbdState & (KBD_STATE_INSERT << 4)))	/*没有按住*/
								{
									KbdState |= (KBD_STATE_INSERT << 4);
									KbdState ^= KBD_STATE_INSERT;
								}
								break;
							case 58:	/*CapsLock*/
								if (!(KbdState & (KBD_STATE_CAPSLOCK << 4)))
								{
									KbdState |= (KBD_STATE_CAPSLOCK << 4);
									KbdState ^= KBD_STATE_CAPSLOCK;
								}
								break;
							case 69:	/*NumLock*/
								if (!(KbdState & (KBD_STATE_NUMLOCK << 4)))
								{
									KbdState |= (KBD_STATE_NUMLOCK << 4);
									KbdState ^= KBD_STATE_NUMLOCK;
								}
								break;
							case 70:	/*ScrLock*/
								if (!(KbdState & (KBD_STATE_SCRLOCK << 4)))
								{
									KbdState |= (KBD_STATE_SCRLOCK << 4);
									KbdState ^= KBD_STATE_SCRLOCK;
								}
								break;
							}
						}
					}
					else
						KbdKey = code << 8;	/*不能处理的键直接放入队列*/
					KbdKey |= KbdState;
					if (*((DWORD*)&RecvPtid) != INVALID)
					{
						DWORD data[MSG_DATA_LEN];

						data[0] = MSG_ATTR_KBD;
						data[1] = KbdKey;
						KSendMsg(RecvPtid, data, 0);
					}
				}
				KbdFlag = 0;
			}
			else if (data[1] == MUS_IRQ)
			{
				switch (MusCou)
				{
				case 0:	/*收到第一字节,按键信息*/
					MusKey = code;
					MusCou = 1;
					continue;
				case 1:	/*收到第二字节,x的位移量*/
//					if (MusKey & MUS_STATE_XOVRFLW)
//						code += 0x100;
					if (MusKey & MUS_STATE_XSIGN)
						code = (char)code;
					MusX += (long)code;
					if (MusX < 0)
						MusX = 0;
					else if (MusX > MusMaxX)
						MusX = MusMaxX;
					MusCou = 2;
					continue;
				case 2:	/*收到第三字节,y的位移量*/
//					if (MusKey & MUS_STATE_YOVRFLW)
//						code += 0x100;
					if (MusKey & MUS_STATE_YSIGN)
						code = (char)code;
					MusY -= (long)code;
					if (MusY < 0)
						MusY = 0;
					else if (MusY > MusMaxY)
						MusY = MusMaxY;
					if (MusId)
					{
						MusCou = 3;
						continue;
					}
					MusCou = 0;
					break;
				case 3:	/*收到第四字节,滚轮位移量*/
					MusZ = (char)code;
					MusCou = 0;
					break;
				}
				if (*((DWORD*)&RecvPtid) != INVALID)
				{
					DWORD data[MSG_DATA_LEN];

					data[0] = MSG_ATTR_MUS;
					data[1] = MusKey;
					data[2] = MusX;
					data[3] = MusY;
					data[4] = MusZ;
					KSendMsg(RecvPtid, data, 0);
				}
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_USER)	/*应用请求消息*/
		{
			if (data[1] == KBDMUS_API_SETRECV)	/*注册接收键盘鼠标消息的线程*/
				RecvPtid = ptid;
		}
	}
	KUnregIrq(KBD_IRQ);
	KUnregIrq(MUS_IRQ);
	KUnregKnlPort(SRV_KBDMUS_PORT);
	return NO_ERROR;
}

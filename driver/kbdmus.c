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

BYTE KeyMap[] = {  0,	/*主键码表*/
	27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,	/*(15-28)TAB--ENTER*/
	0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',			/*(29-40)CTRL--'*/
	'`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	/*(41-54)`--RSHIFT*/
	0,   0,  ' ',  0,													/*(55-58)PrtSc--CapsLock*/
	2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0,   0,			/*(59-70)F1--ScrLock*/
	1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   1,		/*(71-83)HOME--DEL*/
	0,   0,   0,   2,   2,   0,   0,   2,   2,   2						/*(84-93)F11--APPS*/
};
BYTE KeyMapEx[] = {0,	/*扩展键码表*/
	27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13,	/*(15-28)TAB--ENTER*/
	0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',			/*(29-40)<CTRL>*/
	'~', 0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,	/*(41-54)<LSHIFT><RSHIFT>*/
	0,   0,  ' ',  0,													/*(55-58)<ALT><CapsLock>*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,			/*(59-70)<NumLock><ScrLock>*/
	'7','8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',		/*(71-83)HOME--DEL*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0						/*(84-93)F11--APPS*/
};

int main()
{
	DWORD code;	/*端口取码*/
	DWORD KbdFlag, KbdCtrl, KbdState;	/*键码扫描码,前缀标志,控制键状态(左右shift ctrl alt,Pause,PrtSc),键盘功能状态scr num caps ins高四位对应键状态*/
	long MouCou, MouKey, MouMaxX, MouMaxY, MouX, MouY;	/*鼠标数据包计数,键状态,移动范围,位置*/
	void *addr;
	long res;	/*返回结果*/

	if ((res = KRegKnlPort(SRV_KBDMUS_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	if ((res = KRegIrq(KBD_IRQ)) != NO_ERROR)	/*注册键盘中断请求号*/
		return res;
	if ((res = KRegIrq(MUS_IRQ)) != NO_ERROR)	/*注册鼠标中断请求号*/
		return res;
	/*键盘初始化*/
	Wait8042();
	outb(0x60, 0xED);	/*命令:设置LED灯*/
	Ack8042();
	Wait8042();
	outb(0x60, 0x00);	/*设置LED状态:全部关闭*/
	Ack8042();
	/*鼠标初始化*/
	Wait8042();
	outb(0x64, 0xA8);	/*允许鼠标接口*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*通知8042数据将要发向鼠标*/
	Ack8042();
	Wait8042();
	outb(0x60, 0xF4);	/*允许鼠标发数据*/
	Ack8042();
	Wait8042();
	outb(0x64, 0x60);	/*通知8042数据将要发向8042命令寄存器*/
	Ack8042();
	Wait8042();
	outb(0x60, 0x47);	/*许可键盘,鼠标接口及中断*/
	Ack8042();
	inb(0x60);	/*清空寄存器*/
	if ((res = KMapPhyAddr(&addr, 0x90300, 0x100)) != NO_ERROR)	/*从显卡启动数据里获得屏幕像素数*/
		return res;
	MouMaxX = *((WORD*)(addr + 18)) - 1;
	MouMaxY = *((WORD*)(addr + 20)) - 1;
	KFreeAddr(addr);
	MouY = MouX = 0;
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
						KbdCtrl |= 0x40;
						KbdFlag = 0;
					}
					continue;
				}
				if (KbdCtrl & 0x40)	/*pause键退出*/
				{
					KbdCtrl &= 0xBF;
					continue;
				}
				if (code & 0x80)	/*松开键*/
				{
					if ((code & 0x7F) < sizeof(KeyMap) && KeyMap[code & 0x7F] == 0)	/*松开控制键*/
					{
						switch (code)
						{
						case 29 + 0x80:
							if (KbdFlag)
								KbdCtrl &= 0xF7;	/*右ctrl*/
							else
								KbdCtrl &= 0xFB;	/*左ctrl*/
							break;
						case 54 + 0x80:
							KbdCtrl &= 0xFD;	/*右shift*/
							break;
						case 42 + 0x80:
							if (!KbdFlag)
								KbdCtrl &= 0xFE;	/*左shift*/
							break;
						case 56 + 0x80:
							if (KbdFlag)
								KbdCtrl &= 0xDF;	/*右alt*/
							else
								KbdCtrl &= 0xEF;	/*左alt*/
							break;
						case 82 + 0x80:
							KbdState &= 0x7F;	/*Insert*/
							break;
						case 58 + 0x80:
							KbdState &= 0xBF;	/*CapsLock*/
							break;
						case 69 + 0x80:
							KbdState &= 0xDF;	/*NumLock*/
							break;
						case 70 + 0x80:
							KbdState &= 0xEF;	/*ScrLock*/
							break;
						case 55 + 0x80:
							KbdCtrl &= 0x7F;	/*PrtSc*/
							break;
						}
					}
				}
				else	/*按下键*/
				{
					if (code < sizeof(KeyMap))
					{
						if (KeyMap[code] > 2)	/*主键盘键*/
						{
							if (KeyMap[code] >= 'a' && KeyMap[code] <= 'z')	/*按了字母键*/
							{
								if ((KbdState & 0x04) != 0 ^ (KbdCtrl & 0x03) != 0)	/*caps开或shift按下其一符合*/
									putkbque(KeyMapEx[code]);
								else
									putkbque(KeyMap[code]);
							}
							else if (KbdCtrl & 0x03)	/*有shift按下*/
								putkbque(KeyMapEx[code]);
							else
								putkbque(KeyMap[code]);
						}
						else if (KeyMap[code]==2)	/*功能键*/
							putkbque(code<<8);
						else if (KeyMap[code]==1)	/*小键盘*/
						{
							if (KbdState & 0x02)	/*NumLock开*/
								putkbque(KeyMapEx[code]);
							else
								putkbque(code<<8);
						}
						else	/*控制键*/
						{
							switch (code)
							{
							case 29:
								if (KbdFlag)
									KbdCtrl |= 0x08;	/*右ctrl*/
								else
									KbdCtrl |= 0x04;	/*左ctrl*/
								break;
							case 54:
								KbdCtrl |= 0x02;	/*右shift*/
								break;
							case 42:
								if (!KbdFlag)
									KbdCtrl |= 0x01;	/*左shift*/
								break;
							case 56:
								if (KbdFlag)
									KbdCtrl |= 0x20;	/*右alt*/
								else
									KbdCtrl |= 0x10;	/*左alt*/
								break;
							case 82:	/*Insert*/
								if (KbdState & 0x02)	/*numlock开*/
									putkbque(KeyMapEx[code]);
								else if ((KbdState & 0x80)==0)	/*没有按住*/
								{
									KbdState |= 0x80;
									KbdState ^= 0x08;
								}
								break;
							case 58:	/*CapsLock*/
								if ((KbdState & 0x40)==0)
								{
									KbdState |= 0x40;
									KbdState ^= 0x04;
								}
								break;
							case 69:	/*NumLock*/
								if ((KbdState & 0x20)==0)
								{
									KbdState |= 0x20;
									KbdState ^= 0x02;
								}
								break;
							case 70:	/*ScrLock*/
								if ((KbdState & 0x10)==0)
								{
									KbdState |= 0x10;
									KbdState ^= 0x01;
								}
								break;
							case 55:	/*PrtSc*/
								KbdCtrl |= 0x80;
								putkbque(0x3700);
								break;
							}
						}
					}
					else
						putkbque(code << 8);	/*不能处理的键直接放入队列*/
				}
				KbdFlag = 0;
			}
			else if (data[1] == MUS_IRQ)
			{
				switch (MouCou)
				{
				case 0:	/*收到第一字节,按键信息*/
					MouKey = code;
					MouCou = 1;
					break;
				case 1:	/*收到第二字节,x的位移量*/
					if (MouKey & 0x10)
						MouX -= code;
					else
						MouX += code;
					if (MouX < 0)
						MouX = 0;
					else if (MouX > MouMaxX)
						MouX = MouMaxX;
					MouCou = 2;
					break;
				case 2:	/*收到第三字节,y的位移量*/
					if (MouKey & 0x20)
						MouY += code;
					else
						MouY -= code;
					if (MouY < 0)
						MouY = 0;
					else if (MouY > MouMaxY)
						MouY = MouMaxY;
					MouCou = 0;
					break;
				}
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_USER)	/*应用请求消息*/
		{
		}
	}
	KUnregIrq(KBD_IRQ);
	KUnregIrq(MUS_IRQ);
	KUnregKnlPort(SRV_KBDMUS_PORT);
	return NO_ERROR;
}

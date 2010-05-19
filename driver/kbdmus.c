/*	kbdmus.c for ulios driver
	���ߣ�����
	���ܣ�8042�����������������������
	����޸����ڣ�2010-05-18
*/

#include "basesrv.h"

#define KBD_IRQ	0x1	/*�����ж������*/
#define MUS_IRQ	0xC	/*����ж������*/

/*���̿������ȴ�*/
static inline void Wait8042()
{
	while (inb(0x64) & 0x02);
}

/*���̿������ظ�*/
static inline void Ack8042()
{
	while (inb(0x60) != 0xFA);
}

BYTE KeyMap[] = {  0,	/*�������*/
	27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,	/*(15-28)TAB--ENTER*/
	0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',			/*(29-40)CTRL--'*/
	'`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	/*(41-54)`--RSHIFT*/
	0,   0,  ' ',  0,													/*(55-58)PrtSc--CapsLock*/
	2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0,   0,			/*(59-70)F1--ScrLock*/
	1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   1,		/*(71-83)HOME--DEL*/
	0,   0,   0,   2,   2,   0,   0,   2,   2,   2						/*(84-93)F11--APPS*/
};
BYTE KeyMapEx[] = {0,	/*��չ�����*/
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
	DWORD code;	/*�˿�ȡ��*/
	DWORD KbdFlag, KbdCtrl, KbdState;	/*����ɨ����,ǰ׺��־,���Ƽ�״̬(����shift ctrl alt,Pause,PrtSc),���̹���״̬scr num caps ins����λ��Ӧ��״̬*/
	long MouCou, MouKey, MouMaxX, MouMaxY, MouX, MouY;	/*������ݰ�����,��״̬,�ƶ���Χ,λ��*/
	void *addr;
	long res;	/*���ؽ��*/

	if ((res = KRegKnlPort(SRV_KBDMUS_PORT)) != NO_ERROR)	/*ע�����˿ں�*/
		return res;
	if ((res = KRegIrq(KBD_IRQ)) != NO_ERROR)	/*ע������ж������*/
		return res;
	if ((res = KRegIrq(MUS_IRQ)) != NO_ERROR)	/*ע������ж������*/
		return res;
	/*���̳�ʼ��*/
	Wait8042();
	outb(0x60, 0xED);	/*����:����LED��*/
	Ack8042();
	Wait8042();
	outb(0x60, 0x00);	/*����LED״̬:ȫ���ر�*/
	Ack8042();
	/*����ʼ��*/
	Wait8042();
	outb(0x64, 0xA8);	/*�������ӿ�*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	Ack8042();
	Wait8042();
	outb(0x60, 0xF4);	/*������귢����*/
	Ack8042();
	Wait8042();
	outb(0x64, 0x60);	/*֪ͨ8042���ݽ�Ҫ����8042����Ĵ���*/
	Ack8042();
	Wait8042();
	outb(0x60, 0x47);	/*��ɼ���,���ӿڼ��ж�*/
	Ack8042();
	inb(0x60);	/*��ռĴ���*/
	if ((res = KMapPhyAddr(&addr, 0x90300, 0x100)) != NO_ERROR)	/*���Կ���������������Ļ������*/
		return res;
	MouMaxX = *((WORD*)(addr + 18)) - 1;
	MouMaxY = *((WORD*)(addr + 20)) - 1;
	KFreeAddr(addr);
	MouY = MouX = 0;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (data[0] == MSG_ATTR_IRQ)
		{
			code = inb(0x60);
			if (data[1] == KBD_IRQ)	/*�����ж���Ϣ*/
			{
				if (code == 0xE0)	/*ǰ׺,�����1������*/
				{
					KbdFlag = 1;
					continue;
				}
				if (code == 0xE1)	/*ǰ׺,�����2������*/
				{
					KbdFlag = 2;
					continue;
				}
				if (KbdFlag == 2)
				{
					if (code == 0xC5)	/*pause������*/
					{
						KbdCtrl |= 0x40;
						KbdFlag = 0;
					}
					continue;
				}
				if (KbdCtrl & 0x40)	/*pause���˳�*/
				{
					KbdCtrl &= 0xBF;
					continue;
				}
				if (code & 0x80)	/*�ɿ���*/
				{
					if ((code & 0x7F) < sizeof(KeyMap) && KeyMap[code & 0x7F] == 0)	/*�ɿ����Ƽ�*/
					{
						switch (code)
						{
						case 29 + 0x80:
							if (KbdFlag)
								KbdCtrl &= 0xF7;	/*��ctrl*/
							else
								KbdCtrl &= 0xFB;	/*��ctrl*/
							break;
						case 54 + 0x80:
							KbdCtrl &= 0xFD;	/*��shift*/
							break;
						case 42 + 0x80:
							if (!KbdFlag)
								KbdCtrl &= 0xFE;	/*��shift*/
							break;
						case 56 + 0x80:
							if (KbdFlag)
								KbdCtrl &= 0xDF;	/*��alt*/
							else
								KbdCtrl &= 0xEF;	/*��alt*/
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
				else	/*���¼�*/
				{
					if (code < sizeof(KeyMap))
					{
						if (KeyMap[code] > 2)	/*�����̼�*/
						{
							if (KeyMap[code] >= 'a' && KeyMap[code] <= 'z')	/*������ĸ��*/
							{
								if ((KbdState & 0x04) != 0 ^ (KbdCtrl & 0x03) != 0)	/*caps����shift������һ����*/
									putkbque(KeyMapEx[code]);
								else
									putkbque(KeyMap[code]);
							}
							else if (KbdCtrl & 0x03)	/*��shift����*/
								putkbque(KeyMapEx[code]);
							else
								putkbque(KeyMap[code]);
						}
						else if (KeyMap[code]==2)	/*���ܼ�*/
							putkbque(code<<8);
						else if (KeyMap[code]==1)	/*С����*/
						{
							if (KbdState & 0x02)	/*NumLock��*/
								putkbque(KeyMapEx[code]);
							else
								putkbque(code<<8);
						}
						else	/*���Ƽ�*/
						{
							switch (code)
							{
							case 29:
								if (KbdFlag)
									KbdCtrl |= 0x08;	/*��ctrl*/
								else
									KbdCtrl |= 0x04;	/*��ctrl*/
								break;
							case 54:
								KbdCtrl |= 0x02;	/*��shift*/
								break;
							case 42:
								if (!KbdFlag)
									KbdCtrl |= 0x01;	/*��shift*/
								break;
							case 56:
								if (KbdFlag)
									KbdCtrl |= 0x20;	/*��alt*/
								else
									KbdCtrl |= 0x10;	/*��alt*/
								break;
							case 82:	/*Insert*/
								if (KbdState & 0x02)	/*numlock��*/
									putkbque(KeyMapEx[code]);
								else if ((KbdState & 0x80)==0)	/*û�а�ס*/
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
						putkbque(code << 8);	/*���ܴ���ļ�ֱ�ӷ������*/
				}
				KbdFlag = 0;
			}
			else if (data[1] == MUS_IRQ)
			{
				switch (MouCou)
				{
				case 0:	/*�յ���һ�ֽ�,������Ϣ*/
					MouKey = code;
					MouCou = 1;
					break;
				case 1:	/*�յ��ڶ��ֽ�,x��λ����*/
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
				case 2:	/*�յ������ֽ�,y��λ����*/
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
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_USER)	/*Ӧ��������Ϣ*/
		{
		}
	}
	KUnregIrq(KBD_IRQ);
	KUnregIrq(MUS_IRQ);
	KUnregKnlPort(SRV_KBDMUS_PORT);
	return NO_ERROR;
}

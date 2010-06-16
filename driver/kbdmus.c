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

BYTE KeyMap[] = {  0,	/*����Ӣ������������*/
	27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13,	/*(15-28)TAB--ENTER*/
	0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'',			/*(29-40)CTRL--'*/
	'`', 0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,	/*(41-54)`--RSHIFT*/
	0,   0,  ' ',  0,													/*(55-58)PrtSc--CapsLock*/
	2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   0,   0,			/*(59-70)F1--ScrLock*/
	1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   1,		/*(71-83)HOME--DEL*/
	0,   0,   0,   2,   2,   0,   0,   2,   2,   2						/*(84-93)F11--APPS*/
};

BYTE KeyMapEx[] = {0,	/*����Ӣ�������չ�����*/
	27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8,	/*(1-14)ESC--BACKSPACE*/
	9,  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13,	/*(15-28)TAB--ENTER*/
	0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',			/*(29-40)<CTRL>*/
	'~', 0,  '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,	/*(41-54)<LSHIFT><RSHIFT>*/
	0,   0,  ' ',  0,													/*(55-58)<ALT><CapsLock>*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,			/*(59-70)<NumLock><ScrLock>*/
	'7','8', '9', '-', '4', '5', '6', '+', '1', '2', '3', '0', '.',		/*(71-83)HOME--DEL*/
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0						/*(84-93)F11--APPS*/
};

#define VS_MODE_ADDR	0x90500	/*�Կ�ģʽ��Ϣ�����ַ*/

int main()
{
	DWORD code;	/*�˿�ȡ��*/
	DWORD KbdFlag, KbdState;	/*����ɨ����,ǰ׺��־,���Ƽ�״̬*/
	DWORD MusId, MusCou, MusKey;		/*���ID,������ݰ�����,��״̬*/
	long MusX, MusY, MusZ, MusMaxX, MusMaxY;	/*�ƶ���Χ,λ��*/
	THREAD_ID RecvPtid;	/*���ܼ��������Ϣ���߳�ID*/
	void *addr;
	long res;	/*���ؽ��*/

	if ((res = KRegKnlPort(SRV_KBDMUS_PORT)) != NO_ERROR)	/*ע�����˿ں�*/
		return res;
	/*���̳�ʼ��*/
	Wait8042();
	outb(0x60, 0xED);	/*����:����LED��*/
	Ack8042();
	outb(0x60, 0x00);	/*����LED״̬:ȫ���ر�*/
	Ack8042();
	/*����ʼ��*/
	Wait8042();
	outb(0x64, 0xA8);	/*�������ӿ�*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0xF4);	/*������귢����*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0xF3);	/*������������,������άģʽ*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0xC8);	/*������200*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0xF3);	/*������������,������άģʽ*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0x64);	/*������100*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0xF3);	/*������������,������άģʽ*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0x50);	/*������80*/
	Ack8042();
	Wait8042();
	outb(0x64, 0xD4);	/*֪ͨ8042���ݽ�Ҫ�������*/
	outb(0x60, 0xF2);	/*ȡ�����ID*/
	Ack8042();
	MusId = inb(0x60);	/*ȡ�����ID*/
	Wait8042();
	outb(0x64, 0x60);	/*֪ͨ8042���ݽ�Ҫ����8042����Ĵ���*/
	outb(0x60, 0x47);	/*��ɼ���,���ӿڼ��ж�*/
	if ((res = KRegIrq(KBD_IRQ)) != NO_ERROR)	/*ע������ж������*/
		return res;
	if ((res = KRegIrq(MUS_IRQ)) != NO_ERROR)	/*ע������ж������*/
		return res;
	if ((res = KMapPhyAddr(&addr, VS_MODE_ADDR, 0x100)) != NO_ERROR)	/*���Կ���������������Ļ������*/
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
						KbdState |= KBD_STATE_PAUSE;
						KbdFlag = 0;
					}
					continue;
				}
				if (KbdState & KBD_STATE_PAUSE)	/*pause���˳�*/
				{
					KbdState &= (~KBD_STATE_PAUSE);
					continue;
				}
				if (code & 0x80)	/*�ɿ���*/
				{
					code &= 0x7F;
					if (code < sizeof(KeyMap) && KeyMap[code] == 0)	/*�ɿ����Ƽ�*/
					{
						switch (code)
						{
						case 29:
							if (KbdFlag)
								KbdState &= (~KBD_STATE_RCTRL);	/*��ctrl*/
							else
								KbdState &= (~KBD_STATE_LCTRL);	/*��ctrl*/
							break;
						case 54:
							KbdState &= (~KBD_STATE_RSHIFT);	/*��shift*/
							break;
						case 42:
							if (!KbdFlag)
								KbdState &= (~KBD_STATE_LSHIFT);	/*��shift*/
							break;
						case 56:
							if (KbdFlag)
								KbdState &= (~KBD_STATE_RALT);	/*��alt*/
							else
								KbdState &= (~KBD_STATE_LALT);	/*��alt*/
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
				else	/*���¼�*/
				{
					DWORD KbdKey;

					if (code < sizeof(KeyMap))
					{
						if (KeyMap[code] > 2)	/*�����̼�*/
						{
							if (KeyMap[code] >= 'a' && KeyMap[code] <= 'z')	/*������ĸ��*/
							{
								if (((KbdState & KBD_STATE_CAPSLOCK) != 0) ^ ((KbdState & (KBD_STATE_LSHIFT | KBD_STATE_RSHIFT)) != 0))	/*caps����shift������һ����*/
									KbdKey = KeyMapEx[code];
								else
									KbdKey = KeyMap[code];
							}
							else if (KbdState & (KBD_STATE_LSHIFT | KBD_STATE_RSHIFT))	/*��shift����*/
								KbdKey = KeyMapEx[code];
							else
								KbdKey = KeyMap[code];
						}
						else if (KeyMap[code] == 2)	/*���ܼ�*/
							KbdKey = code << 8;
						else if (KeyMap[code] == 1)	/*С����*/
						{
							if (KbdState & KBD_STATE_NUMLOCK)	/*NumLock��*/
								KbdKey = KeyMapEx[code];
							else
								KbdKey = code << 8;
						}
						else	/*���Ƽ�*/
						{
							KbdKey = 0;
							switch (code)
							{
							case 29:
								if (KbdFlag)
									KbdState |= KBD_STATE_RCTRL;	/*��ctrl*/
								else
									KbdState |= KBD_STATE_LCTRL;	/*��ctrl*/
								break;
							case 54:
								KbdState |= KBD_STATE_RSHIFT;	/*��shift*/
								break;
							case 42:
								if (!KbdFlag)
									KbdState |= KBD_STATE_LSHIFT;	/*��shift*/
								break;
							case 56:
								if (KbdFlag)
									KbdState |= KBD_STATE_RALT;	/*��alt*/
								else
									KbdState |= KBD_STATE_LALT;	/*��alt*/
								break;
							case 55:	/*PrtSc*/
								KbdState |= KBD_STATE_PRTSC;
								KbdKey = code << 8;
								break;
							case 82:	/*Insert*/
								if (KbdState & KBD_STATE_NUMLOCK)	/*numlock��*/
									KbdKey = KeyMapEx[code];
								else if (!(KbdState & (KBD_STATE_INSERT << 4)))	/*û�а�ס*/
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
						KbdKey = code << 8;	/*���ܴ���ļ�ֱ�ӷ������*/
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
				case 0:	/*�յ���һ�ֽ�,������Ϣ*/
					MusKey = code;
					MusCou = 1;
					continue;
				case 1:	/*�յ��ڶ��ֽ�,x��λ����*/
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
				case 2:	/*�յ������ֽ�,y��λ����*/
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
				case 3:	/*�յ������ֽ�,����λ����*/
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
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_USER)	/*Ӧ��������Ϣ*/
		{
			if (data[1] == KBDMUS_API_SETRECV)	/*ע����ռ��������Ϣ���߳�*/
				RecvPtid = ptid;
		}
	}
	KUnregIrq(KBD_IRQ);
	KUnregIrq(MUS_IRQ);
	KUnregKnlPort(SRV_KBDMUS_PORT);
	return NO_ERROR;
}

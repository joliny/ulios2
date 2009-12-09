/*	athd.c for ulios driver
	���ߣ�����
	���ܣ�LBAģʽӲ��������������жϿ��Ʒ�ʽ
	����޸����ڣ�2009-07-22
*/

#include "../MkApi/ulimkapi.h"

#define ATHD_IRQ	0xE

/*�˿ڶ�ȡ����*/
static inline void ReadPort(void *buf, WORD port, DWORD n)
{
	void *_buf;
	DWORD _n;
	__asm__ __volatile__("cld;rep insw": "=&D"(_buf), "=&c"(_n): "0"(buf), "d"(port), "1"(n): "flags", "memory");
}

/*�˿�д������*/
static inline void WritePort(void *buf, WORD port, DWORD n)
{
	void *_buf;
	DWORD _n;
	__asm__ __volatile__("cld;rep outsw": "=&S"(_buf), "=&c"(_n): "0"(buf), "d"(port), "1"(n): "flags");
}

typedef struct _PART_DESC
{
	BYTE bootindi;	/*����ָʾ�������Ϊ0x80*/
	BYTE FstHsc[3];	/*��ʼ��ͷ��������*/
	BYTE fsid;		/*�ļ�ϵͳID*/
	BYTE EndHsc[3];	/*������ͷ��������*/
	DWORD relsec;	/*�Ӵ��̿�ʼ������������*/
	DWORD totsec;	/*��������*/
}PART_DESC;	/*Ӳ�̷�����*/

long RwAthd(DWORD dev, BOOL isWrite, DWORD sec, DWORD cou, BYTE *buf)
{
	while ((inb(0x1F7) & 0xC0) != 0x40);	/*�ȴ����������к�����������*/
	cli();
	outb(0x1F1, 0);			/*Ԥ���������*/
	outb(0x1F2, cou);		/*����������*/
	outb(0x1F3, sec);		/*�����ŵ�8λ*/
	outb(0x1F4, sec >> 8);	/*�����Ŵ�8λ*/
	outb(0x1F5, sec >> 16);	/*��������8λ*/
	outb(0x1F6, 0xE0 | (dev << 4) | (sec >> 24) & 0x0F);	/*��������,�����Ÿ�4λ*/
	if (isWrite)
		outb(0x1F7, 0x30);	/*дӲ��*/
	else
		outb(0x1F7, 0x20);	/*��Ӳ��*/
	sti();
	while (cou)
	{
		while ((inb(0x1F7) & 0x0F) != 0x08);
		if (isWrite)
			WritePort(buf, 0x1F0, 256);	/*д�˿�����*/
		else
			ReadPort(buf, 0x1F0, 256);	/*���˿�����*/
		buf+=512;
		cou--;
	}
}

int main()
{
	long res;

	res = KRegIrq(ATHD_IRQ);
	if (res != NO_ERROR)
		return res;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD c, d, src, dest;

		if ((res = KWaitMsg(0, &ptid, &c, &d, &src, &dest)) != NO_ERROR)
			break;
		if (c == (MSG_ATTR_IRQ | ATHD_IRQ))
		{
			
		}
	}
	KUnregIrq(ATHD_IRQ);
	return NO_ERROR;
}

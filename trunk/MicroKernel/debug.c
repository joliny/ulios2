/*	debug.c for ulios
	���ߣ�����
	���ܣ��ں˵������
	����޸����ڣ�2009-05-29
*/

#include "knldef.h"

#define XCOU	80	/*�����ַ���*/
#define YCOU	25	/*�����ַ���*/

/*����ַ�*/
void PutChar(char c)
{
	DWORD x, y;

	outb(0x3D4, 0xF);	/*���ֽ�*/
	x = inb(0x3D5);
	outb(0x3D4, 0xE);	/*���ֽ�*/
	y = inb(0x3D5);
	x |= (y << 8);
	y = x / XCOU;
	x %= XCOU;

	if (c == '\n')	/*���з�*/
	{
		x = 0;
		y++;
	}
	else if (c == '\t')	/*�Ʊ��*/
	{
		while (x & 7)
			txtmem[(x++ + y * XCOU) << 1] = ' ';
	}
	else
		txtmem[(x++ + y * XCOU) << 1] = c;
	if (x == XCOU)	/*��������һ�У��ص�ǰ��*/
	{
		x = 0;
		y++;
	}
	if (y == YCOU)	/*��������һ�У����Ϲ���*/
	{
		DWORD i;
		memcpy32(txtmem, &txtmem[XCOU * 2], XCOU * 2 * (YCOU - 1) / sizeof(DWORD));
		memset32(&txtmem[XCOU * 2 * (YCOU - 1)], 0x07200720, XCOU / 2);
		y--;
	}

	x += y * XCOU;
	y = (x >> 8);
	x &= 0xFF;
	outb(0x3D4, 0xF);
	outb(0x3D5, x);
	outb(0x3D4, 0xE);
	outb(0x3D5, y);
}

/*�����ֵ*/
void PutNum(DWORD n, DWORD r)
{
	static char num[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	char buf[32], *p;

	p = buf;
	for (;;)
	{
		*p = num[n % r];
		n /= r;
		if (n == 0)
			break;
		p++;
	}
	for (;;)
	{
		PutChar(*p);
		if (p <= buf)
			break;
		p--;
	}
}

/*����ַ���*/
void PutS(const char *str)
{
	while (*str)
		PutChar(*str++);
}

/*��ʽ�����*/
void Print(const char *fmtstr, ...)
{
	long num;
	const DWORD *args = (DWORD*)(&fmtstr);

	while (*fmtstr)
	{
		if (*fmtstr == '%')
		{
			fmtstr++;
			switch (*fmtstr)
			{
			case 'd':
				num = *((long*)++args);
				if (num < 0)
				{
					PutChar('-');
					PutNum(-num, 10);
				}
				else
					PutNum(num, 10);
				break;
			case 'u':
				PutNum(*((DWORD*)++args), 10);
				break;
			case 'x':
			case 'X':
				PutNum(*((DWORD*)++args), 16);
				break;
			case 'o':
				PutNum(*((DWORD*)++args), 8);
				break;
			case 's':
				PutS(*((char**)++args));
				break;
			case 'c':
				PutChar(*((char*)++args));
				break;
			default:
				PutChar(*fmtstr);
			}
		}
		else
			PutChar(*fmtstr);
		fmtstr++;
	}

	return;
}

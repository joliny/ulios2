/*	debug.c for ulios
	作者：孙亮
	功能：内核调试相关
	最后修改日期：2009-05-29
*/

#include "knldef.h"

#define XCOU	80	/*横向字符数*/
#define YCOU	25	/*纵向字符数*/

/*输出字符*/
void PutChar(char c)
{
	DWORD x, y;

	outb(0x3D4, 0xF);	/*低字节*/
	x = inb(0x3D5);
	outb(0x3D4, 0xE);	/*高字节*/
	y = inb(0x3D5);
	x |= (y << 8);
	y = x / XCOU;
	x %= XCOU;

	if (c == '\n')	/*换行符*/
	{
		x = 0;
		y++;
	}
	else if (c == '\t')	/*制表符*/
	{
		while (x & 7)
			txtmem[(x++ + y * XCOU) << 1] = ' ';
	}
	else
		txtmem[(x++ + y * XCOU) << 1] = c;
	if (x == XCOU)	/*光标在最后一列，回到前面*/
	{
		x = 0;
		y++;
	}
	if (y == YCOU)	/*光标在最后一行，向上滚屏*/
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

/*输出数值*/
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

/*输出字符窗*/
void PutS(const char *str)
{
	while (*str)
		PutChar(*str++);
}

/*格式化输出*/
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

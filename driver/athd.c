/*	athd.c for ulios driver
	作者：孙亮
	功能：LBA模式硬盘驱动服务程序，中断控制方式
	最后修改日期：2009-07-22
*/

#include "../MkApi/ulimkapi.h"

#define ATHD_IRQ	0xE

/*端口读取数据*/
static inline void ReadPort(void *buf, WORD port, DWORD n)
{
	void *_buf;
	DWORD _n;
	__asm__ __volatile__("cld;rep insw": "=&D"(_buf), "=&c"(_n): "0"(buf), "d"(port), "1"(n): "flags", "memory");
}

/*端口写入数据*/
static inline void WritePort(void *buf, WORD port, DWORD n)
{
	void *_buf;
	DWORD _n;
	__asm__ __volatile__("cld;rep outsw": "=&S"(_buf), "=&c"(_n): "0"(buf), "d"(port), "1"(n): "flags");
}

typedef struct _PART_DESC
{
	BYTE bootindi;	/*引导指示符活动分区为0x80*/
	BYTE FstHsc[3];	/*开始磁头扇区柱面*/
	BYTE fsid;		/*文件系统ID*/
	BYTE EndHsc[3];	/*结束磁头扇区柱面*/
	DWORD relsec;	/*从磁盘开始到分区扇区数*/
	DWORD totsec;	/*总扇区数*/
}PART_DESC;	/*硬盘分区表*/

long RwAthd(DWORD dev, BOOL isWrite, DWORD sec, DWORD cou, BYTE *buf)
{
	while ((inb(0x1F7) & 0xC0) != 0x40);	/*等待控制器空闲和驱动器就绪*/
	cli();
	outb(0x1F1, 0);			/*预补偿柱面号*/
	outb(0x1F2, cou);		/*设置扇区数*/
	outb(0x1F3, sec);		/*扇区号低8位*/
	outb(0x1F4, sec >> 8);	/*扇区号次8位*/
	outb(0x1F5, sec >> 16);	/*扇区号中8位*/
	outb(0x1F6, 0xE0 | (dev << 4) | (sec >> 24) & 0x0F);	/*驱动器号,扇区号高4位*/
	if (isWrite)
		outb(0x1F7, 0x30);	/*写硬盘*/
	else
		outb(0x1F7, 0x20);	/*读硬盘*/
	sti();
	while (cou)
	{
		while ((inb(0x1F7) & 0x0F) != 0x08);
		if (isWrite)
			WritePort(buf, 0x1F0, 256);	/*写端口数据*/
		else
			ReadPort(buf, 0x1F0, 256);	/*读端口数据*/
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

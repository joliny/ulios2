/*	athd.c for ulios driver
	作者：孙亮
	功能：LBA模式硬盘驱动服务程序，中断控制PIO方式
	最后修改日期：2009-07-22
*/

#include "basesrv.h"

#define SER_RBF		0
#define SER_THR		0
#define SER_IER		1
#define SER_IIR		2
#define SER_LCR		3
#define SER_MCR		4
#define SER_LSR		5
#define SER_MSR		6
#define SER_DLL		0
#define SER_DLH		1

#define SER_BAUD_1200		96
#define SER_BAUD_2400		48
#define SER_BAUD_9600		12
#define SER_BAUD_19200		6
#define SER_GP02			8
#define COM_1				0x3F8
#define COM_2				0x2F8	/*/ base port address of port 1*/
#define SER_STOP_1			0		/*/ 1 stop bit per character*/
#define SER_STOP_2			4		/*/ 2 stop bits per character*/
#define SER_BITS_5			0		/*/ send 5 bit characters*/
#define SER_BITS_6			1		/*/ send 6 bit characters*/
#define SER_BITS_7			2		/*/ send 7 bit characters*/
#define SER_BITS_8			3		/*/ send 8 bit characters*/
#define SER_PARITY_NONE		0		/*/ no parity*/
#define SER_PARITY_ODD		8		/*/ odd parity*/
#define SER_PARITY_EVEN		24		/*/ even parity*/
#define SER_DIV_LATCH_ON	128		/*/ used to turn reg 0,1 into divisor latch*/
#define PIC_IMR				0x21	/*/ pic's interrupt mask reg.*/
#define PIC_ICR				0x20	/*/ pic's interupt control reg.*/
#define INT_SER_PORT_0		0x0C	/*/ port 0 interrupt com 1 & 3*/
#define INT_SER_PORT_1		0x0B	/*/ port 0 interrupt com 2 & 4*/
#define SERIAL_BUFF_SIZE	128		/*/ current size of circulating receive buffer*/

char ser_buffer[SERIAL_BUFF_SIZE];	/*/ the receive buffer*/

int ser_end = -1,ser_start = -1;	/*/ indexes into receive buffer*/
int ser_ch, char_ready = 0;			/*/ current character and ready flag*/
int old_int_mask;					/*/ the old interrupt mask on the PIC*/
int open_port;						/*/ the currently open port*/
int serial_lock = 0;				/*/ serial ISR semaphore so the buffer*/
		/*/ isn't altered will it is being written*/
									/*/ to by the ISR*/


/*-------------写串口-----------------*/
void interrupt far Serial_Isr(__CPPARGS)
{
	serial_lock = 1;
	ser_ch = inb(open_port + SER_RBF);
	if (++ser_end > SERIAL_BUFF_SIZE - 1)
		ser_end = 0;
	ser_buffer[ser_end] = ser_ch;

	++char_ready;
	outb(PIC_ICR, 0x20);
	serial_lock = 0;
}


int Ready_Serial()
{
	return(char_ready);
}

/*--------------读串口--------------*/

int Serial_Read()
{
	int ch;
	while (serial_lock){}
	if (ser_end != ser_start)
	{
		if (++ser_start > SERIAL_BUFF_SIZE - 1)
			ser_start = 0;
		ch = ser_buffer[ser_start];
		if (char_ready > 0)
			--char_ready;
		return(ch);
	}
	else
		return(0);

}

/*--------------写串口-----------------*/
Serial_Write(char ch)
{
	while (!(inb(open_port + SER_LSR) & 0x20)){}
	cli();
	outb(open_port + SER_THR, ch);
	sti();
}

/*-----------初始化串口---------------*/
Open_Serial(int port_base, int baud, int configuration)
{
	open_port = port_base;
	outb(port_base + SER_LCR, SER_DIV_LATCH_ON);
	outb(port_base + SER_DLL, baud);
	outb(port_base + SER_DLH, 0);
	outb(port_base + SER_LCR, configuration);
	outb(port_base + SER_MCR, SER_GP02);
	outb(port_base + SER_IER, 1);
	if (port_base == COM_1)
	{
		Old_Isr = _dos_getvect(INT_SER_PORT_0);
		_dos_setvect(INT_SER_PORT_0, Serial_Isr);
		printf("\nOpening Communications Channel Com Port #1...\n");
	}
	else
	{
		Old_Isr = _dos_getvect(INT_SER_PORT_1);
		_dos_setvect(INT_SER_PORT_1, Serial_Isr);
		printf("\nOpening Communications Channel Com Port #2...\n");
	}
	old_int_mask = inb(PIC_IMR);
	outb(PIC_IMR, (port_base==COM_1) ? (old_int_mask & 0xEF) : (old_int_mask & 0xF7 ));
}

/*-------------关闭串口--------------*/
Close_Serial(int port_base)
{
	outb(port_base + SER_MCR, 0);
	outb(port_base + SER_IER, 0);
	outb(PIC_IMR, old_int_mask );
	if (port_base == COM_1)
	{
		_dos_setvect(INT_SER_PORT_0, Old_Isr);
		printf("\nClosing Communications Channel Com Port #1.\n");
	}
	else
	{
		_dos_setvect(INT_SER_PORT_1, Old_Isr);
		printf("\nClosing Communications Channel Com Port #2.\n");
	}
}

/*-------------发送应用----------------*/

void main(int argc,char *argv[])
{
	Open_Serial(COM_1, SER_BAUD_9600, SER_PARITY_NONE | SER_BITS_8 | SER_STOP_1);
	Serial_Write(' ');
	while (!done&&ch != EOF)
	{
		ch = fgetc(fp);
		if (ch==EOF)
			Serial_Write(27);
		Serial_Write(ch);
		if (kbhit())
		{
			press=getch();
			if (press==27)
			{
				Serial_Write(27);
				done=1;
			}
		}
	}
	Close_Serial(COM_1);
}

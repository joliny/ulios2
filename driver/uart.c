/*	uart.c for ulios driver
	���ߣ�����
	���ܣ�COM��������
	����޸����ڣ�2010-08-24
*/

#include "basesrv.h"

typedef struct _UART_CLIENT
{
	BYTE *addr;		/*��ʼ��ַ*/
	BYTE *CurAddr;	/*��ǰ��ַ*/
	DWORD cou;		/*ʣ���ֽ���*/
	DWORD clock;	/*��ʱʱ����*/
}UART_CLIENT;	/*���ڿͻ��˽ṹ*/

#define QUE_LEN		0x400	/*���ݶ��г���*/
typedef struct _UART_REQ
{
	DWORD com;			/*�˿ں�*/
	BYTE que[QUE_LEN];	/*���ݶ���*/
	BYTE *head;			/*����ͷ*/
	BYTE *tail;			/*����β*/
	volatile DWORD quel;/*������*/
	UART_CLIENT reader;	/*����*/
	UART_CLIENT writer;	/*д��*/
}UART_REQ;	/*COM��������ṹ*/

#define COM1_IRQ	0x4	/*COM1�ж������*/
#define COM2_IRQ	0x3	/*COM2�ж������*/

#define PORT_LEN	2	/*��������*/
const WORD ComPort[PORT_LEN] = {0x3F8, 0x2F8};	/*COM�˿ڻ���ַ*/

#define SER_DLL		0	/*�����ʷ�Ƶ���Ͱ�λ*/
#define SER_DLH		1	/*�����ʷ�Ƶ���߰�λ*/
#define SER_DR		0	/*���ݼĴ���*/
#define SER_IER		1	/*�ж������Ĵ���*/
#define SER_IIR		2	/*�жϱ�ʶ�Ĵ���*/
#define SER_LCR		3	/*�����߿��ƼĴ���*/
#define SER_MCR		4	/*���ƽ�������ƼĴ���*/
#define SER_LSR		5	/*������״̬�Ĵ���*/
#define SER_MSR		6	/*���ƽ����״̬�Ĵ���*/

#define MCR_DTR		0x01	/*�����ն˾���DTR��Ч*/
#define MCR_RTS		0x02	/*������RTS��Ч*/
#define MCR_GP02	0x08	/*���2,�����ж�*/
#define LCR_DLAB	0x80	/*��Ƶ����������ȡλ*/
#define LSR_THRE	0x20	/*����������Ĵ�������*/  

/*д����*/
void WriteCom(UART_REQ *req)
{
	if (req->writer.addr)
	{
		outb(ComPort[req->com] + SER_DR, *(req->writer.CurAddr++));
		if (--(req->writer.cou) == 0)
		{
			DWORD data[MSG_DATA_LEN - 2];

			data[0] = NO_ERROR;
			data[1] = req->writer.cou;
			KUnmapProcAddr(req->writer.addr, data);
			req->writer.addr = NULL;
		}
	}
}

/*������*/
void ReadCom(UART_REQ *req)
{
	WORD BasePort;
	BYTE b;

	BasePort = ComPort[req->com];
	while (inb(BasePort + SER_LSR) & 0x01)
	{
		b = inb(ComPort[req->com] + SER_DR);
		if (req->reader.addr)	/*������̵߳Ŀռ�*/
		{
			*(req->reader.CurAddr++) = b;
			if (--(req->reader.cou) == 0)	/*����ɸ���*/
			{
				DWORD data[MSG_DATA_LEN - 2];

				data[0] = NO_ERROR;
				data[1] = req->reader.cou;
				KUnmapProcAddr(req->reader.addr, data);
				req->reader.addr = NULL;
			}
		}
		else	/*�������*/
		{
			lock(&req->quel);
			*req->head = b;
			if (++(req->head) >= &req->que[QUE_LEN])
				req->head = req->que;
			if (req->head == req->tail)	/*��������*/
				if (++(req->tail) >= &req->que[QUE_LEN])	/*������β����*/
					req->tail = req->que;
			ulock(&req->quel);
		}
	}
}

/*�����ж���Ӧ�߳�*/
void ComProc(UART_REQ *req)
{
	long res;		/*���ؽ��*/

	if ((res = KRegIrq(COM1_IRQ)) != NO_ERROR)	/*ע���ж������*/
		KExitThread(res);
	if ((res = KRegIrq(COM2_IRQ)) != NO_ERROR)	/*ע���ж������*/
		KExitThread(res);
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if ((data[0] & 0xFFFF0000) == MSG_ATTR_IRQ)	/*COM���ж�������Ϣ*/
		{
			UART_REQ *CurReq;

			CurReq = &req[data[1] == COM2_IRQ];
			switch (inb(ComPort[CurReq->com] + SER_IIR) & 0x07)
			{
			case 0:	/*MODEM�ж�*/
				inb(ComPort[CurReq->com] + SER_MSR);
				break;
			case 2:	/*�����ж�*/
				WriteCom(CurReq);
				break;
			case 4:	/*�����ж�*/
				ReadCom(CurReq);
				break;
			case 6:	/*���մ��ж�*/
				inb(ComPort[CurReq->com] + SER_LSR);
				break;
			}
		}
	}
	KUnregIrq(COM1_IRQ);
	KUnregIrq(COM2_IRQ);
	KExitThread(res);
}

/*���ƶ������Ѷ�������*/
static inline void CopyQue(UART_REQ *req)
{
	DWORD cou;	/*Ҫ���Ƶ�������*/

	lock(&req->quel);
	if (req->tail > req->head)	/*β��ͷǰ*/
	{
		cou = &req->que[QUE_LEN] - req->tail;
		if (cou > req->reader.cou)	/*�ѽ������ݴ�����������*/
			cou = req->reader.cou;
		memcpy8(req->reader.CurAddr, req->tail, cou);
		req->reader.CurAddr += cou;
		req->reader.cou -= cou;
		if ((req->tail += cou) >= &req->que[QUE_LEN])
			req->tail = req->que;
	}
	if (req->head > req->tail)	/*ͷ��βǰ,��������*/
	{
		cou = req->head - req->tail;
		if (cou > req->reader.cou)	/*�ѽ������ݴ�����������*/
			cou = req->reader.cou;
		memcpy8(req->reader.CurAddr, req->tail, cou);
		req->reader.CurAddr += cou;
		req->reader.cou -= cou;
		req->tail += cou;
	}
	ulock(&req->quel);
	if (req->reader.cou == 0)	/*����ɸ���*/
	{
		DWORD data[MSG_DATA_LEN - 2];

		data[0] = NO_ERROR;
		data[1] = req->reader.cou;
		KUnmapProcAddr(req->reader.addr, data);
		req->reader.addr = NULL;
	}
	else if (req->reader.clock == 0)	/*��ʱ*/
	{
		DWORD data[MSG_DATA_LEN - 2];

		data[0] = UART_ERR_NOTIME;
		data[1] = req->reader.cou;
		KUnmapProcAddr(req->reader.addr, data);
		req->reader.addr = NULL;
	}
}

/*�򿪴���*/
static inline long OpenCom(UART_REQ *req, DWORD com, DWORD baud, DWORD args)
{
	WORD BasePort;
	if (com >= PORT_LEN)
		return UART_ERR_NOPORT;
	BasePort = ComPort[com];
	baud = 115200 / baud;
	if (baud == 0)
		return UART_ERR_BAUD;
	cli();	/*���ô��ڲ���,�򿪴���*/
	outb(BasePort + SER_LCR, LCR_DLAB);
	outb(BasePort + SER_DLL, baud);
	outb(BasePort + SER_DLH, baud >> 8);
	outb(BasePort + SER_LCR, args & (~LCR_DLAB));
	outb(BasePort + SER_MCR, MCR_GP02);
	outb(BasePort + SER_IER, 0x0F);
	sti();
	req += com;
	lock(&req->quel);
	req->tail = req->head = req->que;	/*�����Ѵ洢������*/
	ulock(&req->quel);
	return NO_ERROR;
}

/*�رմ���*/
static inline long CloseCom(DWORD com)
{
	WORD BasePort;
	if (com >= PORT_LEN)
		return UART_ERR_NOPORT;
	BasePort = ComPort[com];
	cli();
	outb(BasePort + SER_MCR, 0);
	outb(BasePort + SER_IER, 0);	/*�رմ����ж�*/
	sti();
	return NO_ERROR;
}

int main()
{
	UART_REQ req[PORT_LEN];
	THREAD_ID ptid;
	long res;		/*���ؽ��*/

	if ((res = KRegKnlPort(SRV_UART_PORT)) != NO_ERROR)	/*ע�����˿ں�*/
		return res;
	memset32(req, 0, sizeof(req) / sizeof(DWORD));	/*��ʼ������*/
	for (res = 0; res < PORT_LEN; res++)
	{
		req[res].com = res;
		req[res].tail = req[res].head = req[res].que;
		outb(ComPort[res] + SER_MCR, 0);
		outb(ComPort[res] + SER_IER, 0);	/*�رմ����ж�*/
	}
	KCreateThread((void(*)(void*))ComProc, 0x1000, req, &ptid);	/*���������ж���Ӧ�߳�*/
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if ((data[0] & 0xFFFF0000) == MSG_ATTR_USER)	/*COM������Ϣ*/
		{
			switch (data[3])
			{
			case UART_API_OPENCOM:
				data[1] = OpenCom(req, data[1], data[2], data[4]);
				break;
			case UART_API_CLOSECOM:
				data[1] = CloseCom(data[1]);
				break;
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_MAP)	/*COM��д��Ϣ*/
		{
			UART_REQ *CurReq;

			if (data[4] >= PORT_LEN)
			{
				data[0] = UART_ERR_NOPORT;
				KUnmapProcAddr((void*)data[2], data);
				continue;
			}
			CurReq = &req[data[4]];
			if (data[0] & 1)	/*������*/
			{
				if (CurReq->reader.addr)
				{
					data[0] = UART_ERR_BUSY;
					KUnmapProcAddr((void*)data[2], data);
					continue;
				}
				CurReq->reader.CurAddr = CurReq->reader.addr = (BYTE*)data[2];
				CurReq->reader.cou = data[1];
				CurReq->reader.clock = data[5];
				CopyQue(CurReq);
			}
			else	/*д����*/
			{
				if (CurReq->writer.addr)
				{
					data[0] = UART_ERR_BUSY;
					KUnmapProcAddr((void*)data[2], data);
					continue;
				}
				CurReq->writer.CurAddr = CurReq->writer.addr = (BYTE*)data[2];
				CurReq->writer.cou = data[1];
				CurReq->writer.clock = data[5];
				WriteCom(CurReq);
			}
		}
	}
	KUnregKnlPort(SRV_UART_PORT);
	return res;
}

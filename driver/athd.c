/*	athd.c for ulios driver
	���ߣ�����
	���ܣ�LBAģʽӲ��������������жϿ���PIO��ʽ
	����޸����ڣ�2009-07-22
*/

#include "basesrv.h"

typedef struct _ATHD_REQ
{
	THREAD_ID ptid;
	void *addr;
	void *CurAddr;
	DWORD sec;
	BYTE cou;
	BYTE isWrite;
	BYTE drv;
	BYTE cmd;
	struct _ATHD_REQ *nxt;
}ATHD_REQ;	/*AT��������ڵ�*/
#define REQ_LEN		0x200

#define ATHD_IRQ	0xE	/*AT�����ж������*/

#define STATE_WAIT	0	/*�����ȴ�״̬*/
#define STATE_BUSY	1	/*���ڴ�������*/

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

typedef struct _HD_ARGS
{
	WORD cylinders;	/*������*/
	BYTE heads;		/*��ͷ��*/
	WORD null0;		/*��ʼ��Сд����������*/
	WORD wpcom;		/*��ʼдǰԤ���������*/
	BYTE ecc;		/*���ECC⧷�����*/
	BYTE ctrlbyte;	/*�����ֽ�*/
	BYTE outime;	/*��׼��ʱֵ*/
	BYTE fmtime;	/*��ʽ����ʱֵ*/
	BYTE checktime;	/*�����������ʱֵ*/
	WORD landzone;	/*��ͷ��½�����*/
	BYTE spt;		/*ÿ�ŵ�������*/
	BYTE res;		/*����*/
}__attribute__((packed)) HD_ARGS;	/*Ӳ�̲�����*/

#define HD_ARGS_ADDR	0x90600	/*Ӳ�̲��������ַ*/

/*��д��������*/
void RwSector(ATHD_REQ *req)
{
	while (inb(0x1F7) != 0x58);	/*�ȴ����������к�����������*/
	if (req->isWrite)
		WritePort(req->CurAddr, 0x1F0, ATHD_BPS / sizeof(WORD));	/*д�˿�����*/
	else
		ReadPort(req->CurAddr, 0x1F0, ATHD_BPS / sizeof(WORD));	/*���˿�����*/
	req->CurAddr += ATHD_BPS;
	req->cou--;
}

/*�����������*/
void OutCmd(ATHD_REQ *req)
{
	DWORD sec;

	sec = req->sec;
	while (inb(0x1F7) != 0x50);	/*�ȴ����������к�����������*/
	cli();
	outb(0x1F1, 0);			/*Ԥ���������*/
	outb(0x1F2, req->cou);	/*����������*/
	outb(0x1F3, sec);		/*�����ŵ�8λ*/
	outb(0x1F4, sec >> 8);	/*�����Ŵ�8λ*/
	outb(0x1F5, sec >> 16);	/*��������8λ*/
	outb(0x1F6, 0xE0 | (req->drv << 4) | ((sec >> 24) & 0x0F));	/*LBA��ʽ,��������,�����Ÿ�4λ*/
	outb(0x1F7, req->isWrite ? 0x30 : 0x20);	/*��дӲ������*/
	sti();
	if (req->isWrite)
		RwSector(req);
}

/*��������ṹ*/
static inline ATHD_REQ *AllocReq(ATHD_REQ *req, ATHD_REQ **FstReq)
{
	ATHD_REQ *CurReq;

	if (*FstReq >= &req[REQ_LEN])
		return NULL;
	CurReq = *FstReq;
	do
		(*FstReq)++;
	while (*FstReq < &req[REQ_LEN] && (*FstReq)->cou);
	return CurReq;
}

/*�ͷ�����ṹ*/
static inline void FreeReq(ATHD_REQ **FstReq, ATHD_REQ *CurReq)
{
	if (*FstReq > CurReq)
		*FstReq = CurReq;
}

/*��������ṹ*/
static inline void AddReq(ATHD_REQ **ReqList, ATHD_REQ **LstReq, ATHD_REQ *req)
{
	req->nxt = NULL;
	if (*ReqList == NULL)
		*ReqList = req;
	else
		(*LstReq)->nxt = req;
	*LstReq = req;
}

/*ɾ������ṹ*/
static inline void DelReq(ATHD_REQ **ReqList)
{
	*ReqList = (*ReqList)->nxt;
}

int main()
{
	ATHD_REQ req[REQ_LEN], *FstReq, *ReqList, *LstReq;	/*���������б�*/
	DWORD secou[2];	/*����Ӳ�̵�������*/
	DWORD state;	/*��ǰ״̬*/
	long res;		/*���ؽ��*/
	HD_ARGS *haddr;

	if ((res = KRegKnlPort(SRV_ATHD_PORT)) != NO_ERROR)	/*ע�����˿ں�*/
		return res;
	if ((res = KRegIrq(ATHD_IRQ)) != NO_ERROR)	/*ע���ж������*/
		return res;
	if ((res = KMapPhyAddr((void**)&haddr, HD_ARGS_ADDR, sizeof(HD_ARGS) * 2)) != NO_ERROR)	/*ӳ��Ӳ�̲���*/
		return res;
	secou[0] = haddr[0].cylinders * haddr[0].heads * haddr[0].spt;
	secou[1] = haddr[1].cylinders * haddr[1].heads * haddr[1].spt;
	if ((res = KFreeAddr(haddr)) != NO_ERROR)
		return res;
	memset32(req, 0, sizeof(req) / sizeof(DWORD));	/*��ʼ������*/
	FstReq = req;
	LstReq = ReqList = NULL;
	state = STATE_WAIT;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (data[0] == MSG_ATTR_IRQ && data[1] == ATHD_IRQ)	/*�����ж�������Ϣ*/
		{
			ATHD_REQ *CurReq;

			CurReq = ReqList;
			if (CurReq->cou)
				RwSector(CurReq);
			if (CurReq->cou == 0)	/*��д���*/
			{
				data[0] = NO_ERROR;
				KUnmapProcAddr(CurReq->addr, data);
				DelReq(&ReqList);
				FreeReq(&FstReq, CurReq);
				if (ReqList)	/*�����û��ʣ������*/
					OutCmd(ReqList);
				else
					state = STATE_WAIT;
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_MAP)	/*��������������Ϣ*/
		{
			ATHD_REQ *CurReq;

			if (secou[data[4] & 1] == 0)	/*������������*/
			{
				data[0] = ATHD_ERR_WRONG_DRV;
				KUnmapProcAddr((void*)data[2], data);
				continue;
			}
			if ((CurReq = AllocReq(req, &FstReq)) == NULL)	/*���������б�����*/
			{
				data[0] = ATHD_ERR_HAVENO_REQ;
				KUnmapProcAddr((void*)data[2], data);
				continue;
			}
			CurReq->ptid = ptid;
			CurReq->CurAddr = CurReq->addr = (void*)data[2];
			CurReq->sec = data[5];
			CurReq->cou = (data[1] + ATHD_BPS - 1) / ATHD_BPS;
			CurReq->cmd = CurReq->isWrite = (~data[0]) & 1;
			CurReq->drv = data[4] & 1;
			AddReq(&ReqList, &LstReq, CurReq);
			if (state == STATE_WAIT)
			{
				state = STATE_BUSY;
				OutCmd(ReqList);
			}
		}
	}
	KUnregIrq(ATHD_IRQ);
	KUnregKnlPort(SRV_ATHD_PORT);
	return res;
}

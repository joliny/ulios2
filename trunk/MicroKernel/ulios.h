/*	ulios.h for ulios
	���ߣ�����
	���ܣ�ulios�ں�һ�γ�ʼ������
	����޸����ڣ�2010-09-13
*/

#include "knldef.h"

/*�ں˱�����ʼ��*/
static inline void InitKnlVal()
{
	memset32(KnlValue, 0, KVAL_LEN * sizeof(BYTE) / sizeof(DWORD));	/*�����ɢ����*/
	memset32(kpt, INVALID, KPT_LEN * sizeof(THREAD_ID) / sizeof(DWORD));	/*��ʼ���ں˶˿�ע���*/
}

/*�ں��ڴ��ʼ��*/
static inline void InitKFMT()
{
	InitFbt(kmmt, FMT_LEN, kdat, KDAT_SIZ);
}

/*�����ڴ�����ʼ��*/
static inline long InitPMM()
{
	DWORD i;
	MEM_ARDS *CurArd;

	i = (((MemEnd - UPHYMEM_ADDR) + 0x0001FFFF) >> 17);	/*ȡ�ý�������ҳ�������*/
	if ((pmmap = (DWORD*)kmalloc(i * sizeof(DWORD))) == NULL)	/*�����û��ڴ�λͼ,��4�ֽ�Ϊ��λ*/
		return ERROR_HAVENO_KMEM;
	memset32(pmmap, 0xFFFFFFFF, i);	/*�ȱ��Ϊ����*/
	PmmLen = (i << 5);	/*�û��ڴ���ҳ��*/
	PmpID = INVALID;
	RemmSiz = 0;
	for (CurArd = ards; CurArd->addr != INVALID; CurArd++)
		if (CurArd->type == ARDS_TYPE_RAM && CurArd->addr + CurArd->siz > UPHYMEM_ADDR)	/*�ڴ�����Ч�Ұ����˽����ڴ�ҳ��*/
		{
			DWORD fst, cou, end, tcu;	/*ҳ����ʼ��,����,ѭ������ֵ,��ʱ����*/

			if (CurArd->addr < UPHYMEM_ADDR)	/*��ַת��Ϊ�����ڴ���Ե�ַ*/
			{
				fst = 0;
				cou = (CurArd->addr + CurArd->siz - UPHYMEM_ADDR) >> 12;
			}
			else
			{
				fst = (CurArd->addr + 0x00000FFF - UPHYMEM_ADDR) >> 12;	/*���ֽڵ�ַ�߶˵�ҳ����ʼ*/
				cou = CurArd->siz >> 12;
			}
			if (PmpID > fst)
				PmpID = fst;
			RemmSiz += (cou << 12);
			end = (fst + 0x0000001F) & 0xFFFFFFE0;
			tcu = end - fst;
			if (fst + cou < end)	/*32ҳ�߽��ڵ�С����*/
			{
				cou = (fst + cou) & 0x0000001F;
				pmmap[fst >> 5] &= ((0xFFFFFFFF >> tcu) | (0xFFFFFFFF << cou));
				continue;
			}
			pmmap[fst >> 5] &= (0xFFFFFFFF >> tcu);	/*32ҳ�߽翪ʼ�������*/
			fst = end;
			cou -= tcu;
			memset32(&pmmap[fst >> 5], 0, cou >> 5);	/*������*/
			fst += (cou & 0xFFFFFFE0);
			cou &= 0x0000001F;
			if (cou)	/*32ҳ�߽�����������*/
				pmmap[fst >> 5] &= (0xFFFFFFFF << cou);
		}
	return NO_ERROR;
}

/*��Ϣ�����ʼ��*/
static inline long InitMsg()
{
	if ((msgmt = (MESSAGE_DESC*)kmalloc(MSGMT_LEN * sizeof(MESSAGE_DESC))) == NULL)
		return ERROR_HAVENO_KMEM;
	memset32(msgmt, 0, MSGMT_LEN * sizeof(MESSAGE_DESC) / sizeof(DWORD));
	FstMsg = msgmt;
	return NO_ERROR;
}

/*��ַӳ������ʼ��*/
static inline long InitMap()
{
	if ((mapmt = (MAPBLK_DESC*)kmalloc(MAPMT_LEN * sizeof(MAPBLK_DESC))) == NULL)
		return ERROR_HAVENO_KMEM;
	memset32(mapmt, 0, MAPMT_LEN * sizeof(MAPBLK_DESC) / sizeof(DWORD));
	FstMap = mapmt;
	return NO_ERROR;
}

/*��ִ���������ʼ��*/
static inline void InitEXMT()
{
	memset32(exmt, 0, EXMT_LEN * sizeof(EXEC_DESC*) / sizeof(DWORD));
	EndExmd = FstExmd = exmt;
}

/*���̹�����ʼ��*/
static inline void InitPMT()
{
	memset32(pmt, 0, PMT_LEN * sizeof(PROCESS_DESC*) / sizeof(DWORD));
	EndPmd = FstPmd = pmt;
/*	CurPmd = NULL;
	PmdCou = 0;
	clock = 0;
	SleepList = NULL;
	LastI387 = NULL;
*/
}

/*��ʼ���ں˽���*/
static inline void InitKnlProc()
{
/*	memset32(&KnlTss, 0, sizeof(TSS) / sizeof(DWORD));
*/	KnlTss.cr3 = (DWORD)kpdt;
	KnlTss.io = sizeof(TSS);
	SetSegDesc(&gdt[UCODE_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_E | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);	/*��ʼ���û�GDT��*/
	SetSegDesc(&gdt[UDATA_SEL >> 3], 0, 0xFFFFF, DESC_ATTR_P | DESC_ATTR_DPL3 | DESC_ATTR_DT | STOSEG_ATTR_T_D_W | STOSEG_ATTR_T_A | SEG_ATTR_G | STOSEG_ATTR_D);
	SetSegDesc(&gdt[TSS_SEL >> 3], (DWORD)&KnlTss, sizeof(TSS) - 1, DESC_ATTR_P | SYSSEG_ATTR_T_TSS);
	__asm__("ltr %%ax":: "a"(TSS_SEL));
}

/*�쳣IDT����(������ַ�޷�����λ����,ֻ���üӼ���)*/
const SEG_GATE_DESC IsrIdtTable[] = {
	{(DWORD)AsmIsr00 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr01 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr02 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr03 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr04 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr05 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr06 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr07 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr08 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr09 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0A - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0B - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0C - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0D - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr0E - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR},
	{(DWORD)AsmIsr0F - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr10 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr11 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr12 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP},
	{(DWORD)AsmIsr13 - 0x00010000 + (KCODE_SEL << 16), 0x00010000 | DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_TRAP}
};

/*�жϴ����ʼ��*/
static inline void InitINTR()
{
	memcpy32(idt, IsrIdtTable, sizeof(IsrIdtTable) / sizeof(DWORD));	/*����20��ISR����������*/
	SetGate(&idt[INTN_APICALL], KCODE_SEL, (DWORD)AsmApiCall, DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP);	/*ϵͳ����*/
	memset32(IrqPort, INVALID, IRQ_LEN * sizeof(THREAD_ID) / sizeof(DWORD));	/*��ʼ��IRQ�˿�ע���*/
	/*��ʱ�Ӻʹ�Ƭ8259�ж�*/
	SetGate(&idt[0x20 + IRQN_TIMER], KCODE_SEL, (DWORD)AsmIrq0, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	SetGate(&idt[0x20 + IRQN_SLAVE8259], KCODE_SEL, (DWORD)AsmIrq2, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	outb(MASTER8259_PORT, IRQ_INIT_MASK);
}

/*��ʼ����������*/
static inline long InitBaseSrv()
{
	PHYBLK_DESC *CurSeg;
	THREAD_ID ptid;
	long res;

	for (CurSeg = &BaseSrv[1]; CurSeg->addr; CurSeg++)
		if ((res = CreateProc(EXEC_ARGV_BASESRV | EXEC_ARGV_DRIVER, CurSeg->addr, CurSeg->siz, &ptid)) != NO_ERROR)
			return res;
	return NO_ERROR;
}

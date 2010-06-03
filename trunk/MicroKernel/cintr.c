/*	cintr.c for ulios
	���ߣ�����
	���ܣ�C�ж�/�쳣/ϵͳ���ô���
	����޸����ڣ�2009-07-01
*/

#include "knldef.h"

/*�쳣IDT����(������ַ�޷�����λ����,ֻ���üӼ���)*/
SEG_GATE_DESC IsrIdtTable[] = {
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

/*C�쳣��*/
void (*IsrCallTable[])(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags) = {
	IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, FpuFaultProc,
	IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, PageFaultProc, IsrProc,
	IsrProc, IsrProc, IsrProc, IsrProc
};

/*�жϱ�*/
void (*AsmIrqCallTable[])() = {
	AsmIrq0, AsmIrq1, AsmIrq2, AsmIrq3, AsmIrq4, AsmIrq5, AsmIrq6, AsmIrq7,
	AsmIrq8, AsmIrq9, AsmIrqA, AsmIrqB, AsmIrqC, AsmIrqD, AsmIrqE, AsmIrqF
};

/*Cϵͳ���ñ�*/
void (*ApiCallTable[])(DWORD *argv) = {
	ApiGetPtid, ApiGiveUp, ApiSleep, ApiCreateThread, ApiExitThread, ApiKillThread, ApiCreateProcess, ApiExitProcess,
	ApiKillProcess, ApiRegKnlPort, ApiUnregKnlPort, ApiGetKpToThed, ApiRegIrq, ApiUnregIrq, ApiSendMsg, ApiRecvMsg,
	ApiMapPhyAddr, ApiMapUserAddr, ApiFreeAddr, ApiReadProcAddr, ApiWriteProcAddr, ApiUnmapProcAddr, ApiCnlmapProcAddr, ApiGetClock
};

/*�жϴ����ʼ��*/
void InitINTR()
{
	memcpy32(idt, IsrIdtTable, sizeof(IsrIdtTable) / sizeof(DWORD));	/*����20��ISR����������*/
	SetGate(&idt[INTN_APICALL], KCODE_SEL, (DWORD)AsmApiCall, DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP);	/*ϵͳ����*/
	memset32(IrqPort, INVALID, IRQ_LEN * sizeof(THREAD_ID) / sizeof(DWORD));	/*��ʼ��IRQ�˿�ע���*/
	/*��ʱ�Ӻʹ�Ƭ8259�ж�*/
	SetGate(&idt[0x20 + IRQN_TIMER], KCODE_SEL, (DWORD)AsmIrq0, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	SetGate(&idt[0x20 + IRQN_SLAVE8259], KCODE_SEL, (DWORD)AsmIrq2, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	outb(0x21, IRQ_INIT_MASK);
}

/*ע��IRQ�źŵ���Ӧ�߳�*/
long RegIrq(DWORD IrqN)
{
	PROCESS_DESC *CurProc;
	BYTE mask;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return ERROR_NOT_DRIVER;	/*����������ȨAPI*/
	if (IrqN >= IRQ_LEN)
		return ERROR_WRONG_IRQN;
	cli();
	if (idt[0x20 + IrqN].d1)
	{
		sti();
		return ERROR_IRQ_ISENABLED;
	}
	SetGate(&idt[0x20 + IrqN], KCODE_SEL, (DWORD)AsmIrqCallTable[IrqN], DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	IrqPort[IrqN] = CurProc->CurTmd->id;
	if (IrqN < 8)
	{
		mask = inb(0x21);	/*��Ƭ*/
		mask &= (~(1ul << IrqN));
		outb(0x21, mask);
	}
	else
	{
		mask = inb(0xA1);	/*��Ƭ*/
		mask &= (~(1ul << (IrqN & 7)));
		outb(0xA1, mask);
	}
	sti();
	return NO_ERROR;
}

/*ע��IRQ�źŵ���Ӧ�߳�*/
long UnregIrq(DWORD IrqN)
{
	PROCESS_DESC *CurProc;
	BYTE mask;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return ERROR_NOT_DRIVER;	/*����������ȨAPI*/
	if (IrqN >= IRQ_LEN)
		return ERROR_WRONG_IRQN;
	cli();
	if (idt[0x20 + IrqN].d1 == 0)
	{
		sti();
		return ERROR_IRQ_ISDISABLED;
	}
	if (IrqPort[IrqN].ProcID != CurProc->CurTmd->id.ProcID)
	{
		sti();
		return ERROR_IRQ_WRONG_CURPROC;
	}
	if (IrqN < 8)
	{
		mask = inb(0x21);	/*��Ƭ*/
		mask |= (1ul << IrqN);
		outb(0x21, mask);
	}
	else
	{
		mask = inb(0xA1);	/*��Ƭ*/
		mask |= (1ul << (IrqN & 7));
		outb(0xA1, mask);
	}
	*(DWORD*)(&IrqPort[IrqN]) = INVALID;
	idt[0x20 + IrqN].d1 = 0;
	sti();
	return NO_ERROR;
}

/*ע���̵߳�����IRQ�ź�*/
void UnregAllIrq()
{
	PROCESS_DESC *CurProc;
	THREAD_ID ptid;
	DWORD i;
	BYTE mask;

	CurProc = CurPmd;
	if (CurProc->attr & PROC_ATTR_APPS)
		return;	/*����������ȨAPI*/
	ptid = CurProc->CurTmd->id;
	cli();
	for (i = 0; i < IRQ_LEN; i++)
		if (*(DWORD*)(&IrqPort[i]) == *(DWORD*)(&ptid))
		{
			if (i < 8)
			{
				mask = inb(0x21);	/*��Ƭ*/
				mask |= (1ul << i);
				outb(0x21, mask);
			}
			else
			{
				mask = inb(0xA1);	/*��Ƭ*/
				mask |= (1ul << (i & 7));
				outb(0xA1, mask);
			}
			*(DWORD*)(&IrqPort[i]) = INVALID;
			idt[0x20 + i].d1 = 0;
		}
	sti();
}

/*���ɻָ��쳣�������*/
void IsrProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	MESSAGE_DESC *msg;

	if ((msg = AllocMsg()) != NULL)	/*֪ͨ���������������Ϣ*/
	{
		msg->ptid = kpt[REP_KPORT];
		msg->data[0] = MSG_ATTR_ISR;
		msg->data[1] = IsrN;
		msg->data[2] = ErrCode;
		msg->data[3] = eip;
		if (SendMsg(msg) != NO_ERROR)
			FreeMsg(msg);
	}
	ThedExit(ERROR_PROC_EXCEP);
}

/*����Э�������쳣�������*/
void FpuFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	THREAD_DESC *CurThed;
	I387 *CurI387;

	CurThed = CurPmd->CurTmd;
	CurI387 = CurThed->i387;
	CurThed->attr &= (~THED_ATTR_APPS);	/*����ϵͳ����̬*/
	if (CurI387 == NULL)	/*�߳��״�ִ��Э������ָ��*/
	{
		if ((CurI387 = (I387*)LockKmalloc(sizeof(I387))) == NULL)
			ThedExit(ERROR_HAVENO_KMEM);
	}
	cli();
	ClearTs();
	if (LastI387 != CurI387)	/*ʹ��Э���������̲߳���ʱ�����л�*/
	{
		if (LastI387)
			__asm__ ("fwait;fnsave %0"::"m"(LastI387));	/*����Э�������Ĵ���*/
		if (CurThed->i387)	/*Э�������Ѿ�����*/
			__asm__ ("frstor %0"::"m"(CurI387));	/*����Э�������Ĵ���*/
		else
		{
			__asm__ ("fninit");	/*��ʼ��Э������*/
			CurThed->i387 = CurI387;
		}
		LastI387 = CurI387;
	}
	sti();
	CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
}

/*�����ж��źŵ��ܵ�����*/
void IrqProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IrqN)
{
	THREAD_DESC *CurThed;

	/*�����жϴ��������ǰ�ж��Ѿ��ر�*/
	if (IrqN == 0)
	{
		clock++;
		if (SleepList && clock >= SleepList->WakeupClock)	/*��ʱ�������������Ѿ���ʱ���߳�*/
			wakeup(SleepList);
		else
		{
			schedul();
			CurThed = CurPmd ? CurPmd->CurTmd : NULL;
			if (CurThed && (CurThed->attr & (THED_ATTR_APPS | THED_ATTR_KILLED)) == (THED_ATTR_APPS | THED_ATTR_KILLED))	/*�߳���Ӧ��̬�±�ɱ��*/
			{
				CurThed->attr &= (~THED_ATTR_KILLED);
				sti();
				ThedExit(ERROR_THED_KILLED);
			}
		}
	}
	else	/*���жϴ�����̷�����Ϣ*/
	{
		MESSAGE_DESC *msg;

		sti();
		CurThed = CurPmd ? CurPmd->CurTmd : NULL;
		if (CurThed)
			CurThed->attr &= (~THED_ATTR_APPS);	/*����ϵͳ����̬*/
		if ((msg = AllocMsg()) == NULL)	/*�ڴ治��*/
		{
			if (CurThed)
				CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
			return;
		}
		msg->ptid = IrqPort[IrqN];
		msg->data[0] = MSG_ATTR_IRQ;
		msg->data[1] = IrqN;
		if (SendMsg(msg) != NO_ERROR)
			FreeMsg(msg);
		if (CurThed)
			CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
	}
}

/*ϵͳ���ýӿ�*/
void ApiCall(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, volatile DWORD eax)
{
	THREAD_DESC *CurThed;

	if (eax >= ((sizeof(ApiCallTable) / sizeof(void*)) << 16))
	{
		eax = ERROR_WRONG_APIN;
		return;
	}
	CurThed = CurPmd->CurTmd;
	CurThed->attr &= (~THED_ATTR_APPS);	/*����ϵͳ����̬*/
	ApiCallTable[eax >> 16](&edi);
	CurThed->attr |= THED_ATTR_APPS;	/*�뿪ϵͳ����̬*/
}

/*����ΪAPI�ӿں���ʵ��*/
#define EDI_ID	0
#define ESI_ID	1
#define EBP_ID	2
#define ESP_ID	3
#define EBX_ID	4
#define EDX_ID	5
#define ECX_ID	6
#define EAX_ID	7

/*ȡ���߳�ID*/
void ApiGetPtid(DWORD *argv)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd->CurTmd;
	argv[EBX_ID] = *(DWORD*)(&CurThed->id);
	argv[ECX_ID] = CurThed->par;
	argv[EDX_ID] = CurPmd->par;
	argv[EAX_ID] = NO_ERROR;
}

/*�������������*/
void ApiGiveUp(DWORD *argv)
{
	cli();
	schedul();
	sti();
	argv[EAX_ID] = NO_ERROR;
}

/*˯��*/
void ApiSleep(DWORD *argv)
{
	if (argv[EBX_ID])
		CliSleep(argv[EBX_ID]);
	argv[EAX_ID] = NO_ERROR;
}

/*�����߳�*/
void ApiCreateThread(DWORD *argv)
{
	argv[EAX_ID] = CreateThed(argv[EBX_ID], argv[ECX_ID], argv[EDX_ID], (THREAD_ID*)&argv[EBX_ID]);
}

/*�˳��߳�*/
void ApiExitThread(DWORD *argv)
{
	ThedExit(argv[EBX_ID]);
}

/*ɱ���߳�*/
void ApiKillThread(DWORD *argv)
{
	argv[EAX_ID] = KillThed(argv[EBX_ID]);
}

/*��������*/
void ApiCreateProcess(DWORD *argv)
{
	BYTE *addr;

	addr = (BYTE*)argv[ESI_ID];
	if (addr >= (BYTE*)UADDR_OFF && addr <= (BYTE*)(0 - PROC_ARGS_SIZE))
	{
		DWORD siz;

		for (siz = 0; *addr; addr++)
			if (++siz >= PROC_ARGS_SIZE)
			{
				argv[EAX_ID] = ERROR_WRONG_APPMSG;
				return;
			}
		addr = (BYTE*)argv[ESI_ID];
		CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*��ֹ�����û��ڴ�ʱ����ҳ�쳣,���½���ϵͳ����̬*/
	}
	else
		addr = NULL;
	argv[EAX_ID] = CreateProc(argv[EBX_ID] & (~EXEC_ARGV_BASESRV), argv[EDI_ID], (DWORD)addr, (THREAD_ID*)&argv[EBX_ID]);
}

/*�˳�����*/
void ApiExitProcess(DWORD *argv)
{
	DeleteProc(argv[EBX_ID]);
}

/*ɱ������*/
void ApiKillProcess(DWORD *argv)
{
	argv[EAX_ID] = KillProc(argv[EBX_ID]);
}

/*ע���ں˶˿ڶ�Ӧ�߳�*/
void ApiRegKnlPort(DWORD *argv)
{
	argv[EAX_ID] = RegKnlPort(argv[EBX_ID]);
}

/*ע���ں˶˿ڶ�Ӧ�߳�*/
void ApiUnregKnlPort(DWORD *argv)
{
	argv[EAX_ID] = UnregKnlPort(argv[EBX_ID]);
}

/*ȡ���ں˶˿ڶ�Ӧ�߳�*/
void ApiGetKpToThed(DWORD *argv)
{
	argv[EAX_ID] = GetKptThed(argv[EBX_ID], (THREAD_ID*)&argv[EBX_ID]);
}

/*ע��IRQ�źŵ���Ӧ�߳�*/
void ApiRegIrq(DWORD *argv)
{
	argv[EAX_ID] = RegIrq(argv[EBX_ID]);
}

/*ע��IRQ�źŵ���Ӧ�߳�*/
void ApiUnregIrq(DWORD *argv)
{
	argv[EAX_ID] = UnregIrq(argv[EBX_ID]);
}

/*������Ϣ*/
void ApiSendMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
	{
		argv[EAX_ID] = ERROR_WRONG_APPMSG;
		return;
	}
	memcpy32(data, addr, MSG_DATA_LEN);	/*�������ݵ��ں˿ռ�*/
	CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*��ֹ�����û��ڴ�ʱ����ҳ�쳣,���½���ϵͳ����̬*/
	if (data[0] < MSG_ATTR_USER)
	{
		argv[EAX_ID] = ERROR_WRONG_APPMSG;
		return;
	}
	if ((msg = AllocMsg()) == NULL)
	{
		argv[EAX_ID] = ERROR_HAVENO_MSGDESC;
		return;
	}
	msg->ptid = *((THREAD_ID*)&argv[EBX_ID]);
	memcpy32(msg->data, data, MSG_DATA_LEN);
	if ((argv[EAX_ID] = SendMsg(msg)) != NO_ERROR)
		FreeMsg(msg);
	if (argv[EAX_ID] == NO_ERROR && argv[ECX_ID])	/*�ȴ�������Ϣ*/
		if ((argv[EAX_ID] = WaitThedMsg(&msg, *((THREAD_ID*)&argv[EBX_ID]), argv[ECX_ID])) == NO_ERROR)
		{
			memcpy32(data, msg->data, MSG_DATA_LEN);
			FreeMsg(msg);
			memcpy32(addr, data, MSG_DATA_LEN);	/*�������ݵ��û��ռ�*/
		}
}

/*������Ϣ*/
void ApiRecvMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
		addr = NULL;
	if ((argv[EAX_ID] = RecvMsg(&msg, argv[EBX_ID])) == NO_ERROR)
	{
		argv[EBX_ID] = *((DWORD*)&msg->ptid);
		if (addr)
			memcpy32(data, msg->data, MSG_DATA_LEN);
		FreeMsg(msg);
		if (addr)
			memcpy32(addr, data, MSG_DATA_LEN);	/*�������ݵ��û��ռ�*/
	}
}

/*ӳ�������ַ*/
void ApiMapPhyAddr(DWORD *argv)
{
	argv[EAX_ID] = MapPhyAddr((void**)&argv[ESI_ID], argv[EBX_ID], argv[ECX_ID]);
}

/*ӳ���û���ַ*/
void ApiMapUserAddr(DWORD *argv)
{
	argv[EAX_ID] = MapUserAddr((void**)&argv[ESI_ID], argv[ECX_ID]);
}

/*�����û���ַ��*/
void ApiFreeAddr(DWORD *argv)
{
	argv[EAX_ID] = UnmapAddr((void*)argv[ESI_ID]);
}

/*ӳ����̵�ַ��ȡ*/
void ApiReadProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
		addr = NULL;
	if (addr)
	{
		memcpy32(data, addr, MSG_DATA_LEN - 3);	/*�������ݵ��ں˿ռ�*/
		CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*��ֹ�����û��ڴ�ʱ����ҳ�쳣,���½���ϵͳ����̬*/
	}
	if ((argv[EAX_ID] = MapProcAddr((void*)argv[EDI_ID], argv[ECX_ID], *((THREAD_ID*)&argv[EBX_ID]), TRUE, TRUE, data, argv[EDX_ID])) == NO_ERROR)
		if (addr)
			memcpy32(addr, data, MSG_DATA_LEN);	/*�������ݵ��û��ռ�*/
}

/*ӳ����̵�ַд��*/
void ApiWriteProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
		addr = NULL;
	if (addr)
	{
		memcpy32(data, addr, MSG_DATA_LEN - 3);	/*�������ݵ��ں˿ռ�*/
		CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*��ֹ�����û��ڴ�ʱ����ҳ�쳣,���½���ϵͳ����̬*/
	}
	if ((argv[EAX_ID] = MapProcAddr((void*)argv[EDI_ID], argv[ECX_ID], *((THREAD_ID*)&argv[EBX_ID]), FALSE, TRUE, data, argv[EDX_ID])) == NO_ERROR)
		if (addr)
			memcpy32(addr, data, MSG_DATA_LEN);	/*�������ݵ��û��ռ�*/
}

/*����ӳ����̵�ַ*/
void ApiUnmapProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN - 2];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
		addr = NULL;
	if (addr)
	{
		memcpy32(data, addr, MSG_DATA_LEN - 2);	/*�������ݵ��ں˿ռ�*/
		CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*��ֹ�����û��ڴ�ʱ����ҳ�쳣,���½���ϵͳ����̬*/
	}
	argv[EAX_ID] = UnmapProcAddr((void*)argv[EDI_ID], data);
}

/*ȡ��ӳ����̵�ַ*/
void ApiCnlmapProcAddr(DWORD *argv)
{
	void *addr;
	DWORD data[MSG_DATA_LEN - 2];

	addr = (void*)argv[ESI_ID];
	if (addr < UADDR_OFF || addr > (void*)(0 - sizeof(data)))
		addr = NULL;
	if (addr)
	{
		memcpy32(data, addr, MSG_DATA_LEN - 2);	/*�������ݵ��ں˿ռ�*/
		CurPmd->CurTmd->attr &= (~THED_ATTR_APPS);	/*��ֹ�����û��ڴ�ʱ����ҳ�쳣,���½���ϵͳ����̬*/
	}
	argv[EAX_ID] = CnlmapProcAddr((void*)argv[EDI_ID], data);
}

/*ȡ�ÿ���������ʱ��*/
void ApiGetClock(DWORD *argv)
{
	argv[EBX_ID] = clock;
	argv[EAX_ID] = NO_ERROR;
}

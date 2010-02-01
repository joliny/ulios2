/*	cintr.c for ulios
	作者：孙亮
	功能：C中断/异常/系统调用处理
	最后修改日期：2009-07-01
*/

#include "knldef.h"

/*异常IDT表副本(函数地址无法进行位运算,只好用加减了)*/
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

/*C异常表*/
void (*IsrCallTable[])(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags) = {
	IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc,
	IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, IsrProc, PageFaultProc, IsrProc,
	IsrProc, IsrProc, IsrProc, IsrProc
};

/*中断表*/
void (*AsmIrqCallTable[])() = {
	AsmIrq0, AsmIrq1, AsmIrq2, AsmIrq3, AsmIrq4, AsmIrq5, AsmIrq6, AsmIrq7,
	AsmIrq8, AsmIrq9, AsmIrqA, AsmIrqB, AsmIrqC, AsmIrqD, AsmIrqE, AsmIrqF
};

/*C系统调用表*/
void (*ApiCallTable[])(DWORD *argv) = {
	ApiPrintf, ApiGiveUp, ApiSleep, ApiCreateThread, ApiExitThread, ApiKillThread, ApiCreateProcess, ApiExitProcess,
	ApiKillProcess, ApiRegKnlPort, ApiUnregKnlPort, ApiGetKpToThed, ApiRegIrq, ApiUnregIrq, ApiSendMsg, ApiRecvMsg,
	ApiWaitMsg, ApiMapPhyAddr, ApiMapUserAddr, ApiFreeAddr
};

/*中断处理初始化*/
void InitINTR()
{
	memcpy32(idt, IsrIdtTable, sizeof(IsrIdtTable) / sizeof(DWORD));	/*复制20个ISR的门描述符*/
	SetGate(&idt[INTN_APICALL], KCODE_SEL, (DWORD)AsmApiCall, DESC_ATTR_P | DESC_ATTR_DPL3 | GATE_ATTR_T_TRAP);	/*系统调用*/
	memset32(IrqPort, 0xFFFFFFFF, IRQ_LEN * sizeof(THREAD_ID) / sizeof(DWORD));	/*初始化Irq端口注册表*/
	/*开时钟和从片8259中断*/
	SetGate(&idt[0x20 + IRQN_TIMER], KCODE_SEL, (DWORD)AsmIrq0, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	SetGate(&idt[0x20 + IRQN_SLAVE8259], KCODE_SEL, (DWORD)AsmIrq2, DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	outb(0x21, IRQ_INIT_MASK);
}

/*注册IRQ信号的响应线程*/
long RegIrq(DWORD IrqN)
{
	cli();
	if (idt[0x20 + IrqN].d1)
	{
		sti();
		return ERROR_IRQ_ISENABLED;
	}
	SetGate(&idt[0x20 + IrqN], KCODE_SEL, (DWORD)AsmIrqCallTable[IrqN], DESC_ATTR_P | DESC_ATTR_DPL0 | GATE_ATTR_T_INTR);
	IrqPort[IrqN] = CurPmd->CurTmd->id;
	if (IrqN < 8)
	{
		BYTE mask;

		mask = inb(0x21);	/*主片*/
		mask &= (~(1ul << IrqN));
		outb(0x21, mask);
	}
	else
	{
		BYTE mask;

		mask = inb(0xA1);	/*从片*/
		mask &= (~(1ul << (IrqN & 7)));
		outb(0xA1, mask);
	}
	sti();
	return NO_ERROR;
}

/*注销IRQ信号的响应线程*/
long UnregIrq(DWORD IrqN)
{
	cli();
	if (idt[0x20 + IrqN].d1 == 0)
	{
		sti();
		return ERROR_IRQ_ISDISABLED;
	}
	if (IrqPort[IrqN].ProcID != CurPmd->CurTmd->id.ProcID)
	{
		sti();
		return ERROR_IRQ_WRONG_CURPROC;
	}
	if (IrqN < 8)
	{
		BYTE mask;

		mask = inb(0x21);	/*主片*/
		mask |= (1ul << IrqN);
		outb(0x21, mask);
	}
	else
	{
		BYTE mask;

		mask = inb(0xA1);	/*从片*/
		mask |= (1ul << (IrqN & 7));
		outb(0xA1, mask);
	}
	*(DWORD*)(&IrqPort[IrqN]) = INVALID;
	idt[0x20 + IrqN].d1 = 0;
	sti();
	return NO_ERROR;
}

/*注销线程的所有IRQ信号*/
void UnregAllIrq()
{
	THREAD_ID ptid;
	DWORD i;

	ptid = CurPmd->CurTmd->id;
	cli();
	for (i = 0; i < IRQ_LEN; i++)
		if (*(DWORD*)(&IrqPort[i]) == *(DWORD*)(&ptid))
		{
			if (i < 8)
			{
				BYTE mask;

				mask = inb(0x21);	/*主片*/
				mask |= (1ul << i);
				outb(0x21, mask);
			}
			else
			{
				BYTE mask;

				mask = inb(0xA1);	/*从片*/
				mask |= (1ul << (i & 7));
				outb(0xA1, mask);
			}
			*(DWORD*)(&IrqPort[i]) = INVALID;
			idt[0x20 + i].d1 = 0;
		}
	sti();
}

/*所有异常信号的总调函数*/
void IsrProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags)
{
	DebugMsg("EAX=%X\tEBX=%X\tECX=%X\tEDX=%X\n", eax, ebx, ecx, edx);
	DebugMsg("EBP=%X\tESI=%X\tEDI=%X\tESP=%X\tEIP=%X\n", ebp, esi, edi, esp, eip);
	DebugMsg("CS=%X\tDS=%X\tES=%X\tFS=%X\tGS=%X\n", cs, ds, es, fs, gs);
	DebugMsg("EFLAGS=%X\tISR=%X\tERR=%X\n", eflags, IsrN, ErrCode);
	DeleteThed();
}

/*所有中断信号的总调函数*/
void IrqProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IrqN)
{
	THREAD_DESC *CurThed;

	CurThed = CurPmd ? CurPmd->CurTmd : NULL;
	/*进入中断处理程序以前中断已经关闭*/
	if (IrqN == 0)
	{
		clock++;
		if (SleepList && clock >= SleepList->WakeupClock)	/*延时阻塞链表中有已经超时的线程*/
			wakeup(SleepList);
		else
		{
			schedul();
			if (CurThed && (CurThed->attr & (THED_ATTR_APPS | THED_ATTR_KILLED)) == (THED_ATTR_APPS | THED_ATTR_KILLED))	/*线程在应用态下被杀死*/
			{
				CurThed->attr &= (~THED_ATTR_KILLED);
				sti();
				DeleteThed();
			}
		}
	}
	else	/*给中断处理进程发送消息*/
	{
		MESSAGE_DESC *msg;

		sti();
		if (CurThed)
			CurThed->attr &= (~THED_ATTR_APPS);	/*进入系统调用态*/
		if ((msg = AllocMsg()) == NULL)	/*内存不足*/
		{
			if (CurThed)
				CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
			return;
		}
		msg->ptid = IrqPort[IrqN];
		msg->data[0] = MSG_ATTR_IRQ | IrqN;
		if (SendMsg(msg) != NO_ERROR)
			FreeMsg(msg);
		if (CurThed)
			CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
	}
}

/*系统调用接口*/
void ApiCall(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, volatile DWORD eax)
{
	THREAD_DESC *CurThed;

	if (eax >= (APICALL_LEN << 16))
	{
		eax = ERROR_WRONG_APIN;
		return;
	}
	CurThed = CurPmd->CurTmd;
	CurThed->attr &= (~THED_ATTR_APPS);	/*进入系统调用态*/
	ApiCallTable[eax >> 16](&edi);
	CurThed->attr |= THED_ATTR_APPS;	/*离开系统调用态*/
}

/*以下为API接口函数实现*/
#define EDI_ID	0
#define ESI_ID	1
#define EBP_ID	2
#define ESP_ID	3
#define EBX_ID	4
#define EDX_ID	5
#define ECX_ID	6
#define EAX_ID	7

/*调试输出*/
void ApiPrintf(DWORD *argv)
{
	Print((const char*)argv[ESI_ID], argv[EBX_ID]);
	argv[EAX_ID] = NO_ERROR;
//	argv[EBX_ID] = *(DWORD*)(&CurPmd->CurTmd->id);
}

/*主动放弃处理机*/
void ApiGiveUp(DWORD *argv)
{
	cli();
	schedul();
	sti();
	argv[EAX_ID] = NO_ERROR;
}

/*睡眠*/
void ApiSleep(DWORD *argv)
{
	SleepCs(argv[EBX_ID]);
	argv[EAX_ID] = NO_ERROR;
}

/*创建线程*/
void ApiCreateThread(DWORD *argv)
{
	DWORD args[2];

	args[0] = argv[EBX_ID];
	args[1] = argv[ECX_ID];
	argv[EAX_ID] = CreateThed(args, (THREAD_ID*)&argv[EBX_ID]);
}

/*退出线程*/
void ApiExitThread(DWORD *argv)
{
	DeleteThed();
}

/*杀死线程*/
void ApiKillThread(DWORD *argv)
{
	if (argv[EBX_ID] >= TMT_LEN)
		argv[EAX_ID] = ERROR_WRONG_THEDID;
	else
		argv[EAX_ID] = KillThed(argv[EBX_ID]);
}

/*创建进程*/
void ApiCreateProcess(DWORD *argv)
{
	argv[EAX_ID] = CreateProc(argv[EBX_ID] & (~EXEC_ARGS_BASESRV), &argv[ECX_ID], (THREAD_ID*)&argv[EBX_ID]);
}

/*退出进程*/
void ApiExitProcess(DWORD *argv)
{
	DeleteProc();
}

/*杀死进程*/
void ApiKillProcess(DWORD *argv)
{
	if (argv[EBX_ID] >= PMT_LEN)
		argv[EAX_ID] = ERROR_WRONG_PROCID;
	else
		argv[EAX_ID] = KillProc(argv[EBX_ID]);
}

/*注册内核端口对应线程*/
void ApiRegKnlPort(DWORD *argv)
{
	argv[EAX_ID] = RegKnlPort(argv[EBX_ID]);
}

/*注销内核端口对应线程*/
void ApiUnregKnlPort(DWORD *argv)
{
	argv[EAX_ID] = UnregKnlPort(argv[EBX_ID]);
}

/*取得内核端口对应线程*/
void ApiGetKpToThed(DWORD *argv)
{
	argv[EAX_ID] = GetKpToThed(argv[EBX_ID], (THREAD_ID*)&argv[EBX_ID]);
}

/*注册IRQ信号的响应线程*/
void ApiRegIrq(DWORD *argv)
{
	if (CurPmd->attr & PROC_ATTR_APPS)
		argv[EAX_ID] = ERROR_NOT_DRIVER;
	else if (argv[EBX_ID] >= IRQ_LEN)
		argv[EAX_ID] = ERROR_WRONG_IRQN;
	else	/*驱动进程特权API*/
		argv[EAX_ID] = RegIrq(argv[EBX_ID]);
}

/*注销IRQ信号的响应线程*/
void ApiUnregIrq(DWORD *argv)
{
	if (CurPmd->attr & PROC_ATTR_APPS)
		argv[EAX_ID] = ERROR_NOT_DRIVER;
	else if (argv[EBX_ID] >= IRQ_LEN)
		argv[EAX_ID] = ERROR_WRONG_IRQN;
	else	/*驱动进程特权API*/
		argv[EAX_ID] = UnregIrq(argv[EBX_ID]);
}

/*发送消息*/
void ApiSendMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;

	if (((THREAD_ID*)&argv[EBX_ID])->ProcID >= PMT_LEN)
		argv[EAX_ID] = ERROR_WRONG_PROCID;
	else if (((THREAD_ID*)&argv[EBX_ID])->ThedID >= TMT_LEN)
		argv[EAX_ID] = ERROR_WRONG_THEDID;
	else if (argv[ECX_ID] < MSG_ATTR_PROC)
		argv[EAX_ID] = ERROR_WRONG_APPMSG;
	else if ((msg = AllocMsg()) == NULL)
		argv[EAX_ID] = ERROR_HAVENO_MSGDESC;
	else
	{
		msg->ptid = *((THREAD_ID*)&argv[EBX_ID]);
		msg->data[0] = argv[ECX_ID];
		msg->data[1] = argv[EDX_ID];
		msg->data[2] = argv[ESI_ID];
		msg->data[3] = argv[EDI_ID];
		if ((argv[EAX_ID] = SendMsg(msg)) != NO_ERROR)
			FreeMsg(msg);
	}
}

/*接收消息*/
void ApiRecvMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;

	if ((argv[EAX_ID] = RecvMsg(&msg)) == NO_ERROR)
	{
		argv[EBX_ID] = *((DWORD*)&msg->ptid);
		argv[ECX_ID] = msg->data[0];
		argv[EDX_ID] = msg->data[1];
		argv[ESI_ID] = msg->data[2];
		argv[EDI_ID] = msg->data[3];
		FreeMsg(msg);
	}
}

/*阻塞并等待消息*/
void ApiWaitMsg(DWORD *argv)
{
	MESSAGE_DESC *msg;

	if ((argv[EAX_ID] = WaitMsg(&msg, argv[EBX_ID])) == NO_ERROR)
	{
		argv[EBX_ID] = *((DWORD*)&msg->ptid);
		argv[ECX_ID] = msg->data[0];
		argv[EDX_ID] = msg->data[1];
		argv[ESI_ID] = msg->data[2];
		argv[EDI_ID] = msg->data[3];
		FreeMsg(msg);
	}
}

/*映射物理地址*/
void ApiMapPhyAddr(DWORD *argv)
{
	if (CurPmd->attr & PROC_ATTR_APPS)
		argv[EAX_ID] = ERROR_NOT_DRIVER;
	else	/*驱动进程特权API*/
		argv[EAX_ID] = MapPhyAddr((void**)&argv[EBX_ID], argv[EBX_ID], argv[ECX_ID]);
}

/*映射用户地址*/
void ApiMapUserAddr(DWORD *argv)
{
	argv[EAX_ID] = MapUserAddr((void**)&argv[EBX_ID], argv[EBX_ID]);
}

/*回收用户地址块*/
void ApiFreeAddr(DWORD *argv)
{
	argv[EAX_ID] = LockFreeUBlk(CurPmd, (void*)argv[EBX_ID]);
}

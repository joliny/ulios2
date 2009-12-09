/*	cintr.c for ulios
	作者：孙亮
	功能：C中断/异常/系统调用处理
	最后修改日期：2009-07-01
*/

#ifndef _CINTR_H_
#define _CINTR_H_

#include "ulidef.h"

#define IRQN_TIMER		0		/*时钟中断信号*/
#define IRQN_SLAVE8259	2		/*从片8259信号*/
#define IRQ_INIT_MASK	0xFA	/*开启时钟和从片8259中断的掩码*/

#define INTN_APICALL	0xF0	/*系统调用号*/
#define APICALL_LEN		0x14	/*系统调用表长度，也就是系统调用数量*/

/*中断异常系统调用处理函数*/
extern void AsmIsr00();
extern void AsmIsr01();
extern void AsmIsr02();
extern void AsmIsr03();
extern void AsmIsr04();
extern void AsmIsr05();
extern void AsmIsr06();
extern void AsmIsr07();
extern void AsmIsr08();
extern void AsmIsr09();
extern void AsmIsr0A();
extern void AsmIsr0B();
extern void AsmIsr0C();
extern void AsmIsr0D();
extern void AsmIsr0E();
extern void AsmIsr0F();
extern void AsmIsr10();
extern void AsmIsr11();
extern void AsmIsr12();
extern void AsmIsr13();

extern void AsmIrq0();
extern void AsmIrq1();
extern void AsmIrq2();
extern void AsmIrq3();
extern void AsmIrq4();
extern void AsmIrq5();
extern void AsmIrq6();
extern void AsmIrq7();
extern void AsmIrq8();
extern void AsmIrq9();
extern void AsmIrqA();
extern void AsmIrqB();
extern void AsmIrqC();
extern void AsmIrqD();
extern void AsmIrqE();
extern void AsmIrqF();

extern void AsmApiCall();

/*中断处理初始化*/
void InitINTR();

/*注册IRQ信号的响应线程*/
long RegIrq(DWORD IrqN);

/*注销IRQ信号的响应线程*/
long UnregIrq(DWORD IrqN);

/*注销线程的所有IRQ信号*/
void UnregAllIrq();

/*所有异常信号的总调函数*/
void IsrProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

/*所有中断信号的总调函数*/
void IrqProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IrqN);

/*系统调用接口*/
void ApiCall(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax);

/*以下为API接口函数*/

/*调试输出*/
void ApiPrintf(DWORD *argv);

/*主动放弃处理机*/
void ApiGiveUp(DWORD *argv);

/*睡眠*/
void ApiSleep(DWORD *argv);

/*创建线程*/
void ApiCreateThread(DWORD *argv);

/*退出线程*/
void ApiExitThread(DWORD *argv);

/*杀死线程*/
void ApiKillThread(DWORD *argv);

/*创建进程*/
void ApiCreateProcess(DWORD *argv);

/*退出进程*/
void ApiExitProcess(DWORD *argv);

/*杀死进程*/
void ApiKillProcess(DWORD *argv);

/*注册内核端口对应线程*/
void ApiRegKnlPort(DWORD *argv);

/*注销内核端口对应线程*/
void ApiUnregKnlPort(DWORD *argv);

/*取得内核端口对应线程*/
void ApiGetKpToThed(DWORD *argv);

/*注册IRQ信号的响应线程*/
void ApiRegIrq(DWORD *argv);

/*注销IRQ信号的响应线程*/
void ApiUnregIrq(DWORD *argv);

/*发送消息*/
void ApiSendMsg(DWORD *argv);

/*接收消息*/
void ApiRecvMsg(DWORD *argv);

/*阻塞并等待消息*/
void ApiWaitMsg(DWORD *argv);

/*映射物理地址*/
void ApiMapPhyAddr(DWORD *argv);

/*映射用户地址*/
void ApiMapUserAddr(DWORD *argv);

/*回收用户地址块*/
void ApiFreeAddr(DWORD *argv);

#endif

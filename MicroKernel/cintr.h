/*	cintr.c for ulios
	���ߣ�����
	���ܣ�C�ж�/�쳣/ϵͳ���ô���
	����޸����ڣ�2009-07-01
*/

#ifndef _CINTR_H_
#define _CINTR_H_

#include "ulidef.h"

#define IRQN_TIMER		0		/*ʱ���ж��ź�*/
#define IRQN_SLAVE8259	2		/*��Ƭ8259�ź�*/
#define IRQ_INIT_MASK	0xFA	/*����ʱ�Ӻʹ�Ƭ8259�жϵ�����*/

#define INTN_APICALL	0xF0	/*ϵͳ���ú�*/
#define APICALL_LEN		0x14	/*ϵͳ���ñ����ȣ�Ҳ����ϵͳ��������*/

/*�ж��쳣ϵͳ���ô�������*/
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

/*�жϴ�����ʼ��*/
void InitINTR();

/*ע��IRQ�źŵ���Ӧ�߳�*/
long RegIrq(DWORD IrqN);

/*ע��IRQ�źŵ���Ӧ�߳�*/
long UnregIrq(DWORD IrqN);

/*ע���̵߳�����IRQ�ź�*/
void UnregAllIrq();

/*�����쳣�źŵ��ܵ�����*/
void IsrProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

/*�����ж��źŵ��ܵ�����*/
void IrqProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IrqN);

/*ϵͳ���ýӿ�*/
void ApiCall(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, volatile DWORD eax);

/*����ΪAPI�ӿں���*/

/*�������*/
void ApiPrintf(DWORD *argv);

/*��������������*/
void ApiGiveUp(DWORD *argv);

/*˯��*/
void ApiSleep(DWORD *argv);

/*�����߳�*/
void ApiCreateThread(DWORD *argv);

/*�˳��߳�*/
void ApiExitThread(DWORD *argv);

/*ɱ���߳�*/
void ApiKillThread(DWORD *argv);

/*��������*/
void ApiCreateProcess(DWORD *argv);

/*�˳�����*/
void ApiExitProcess(DWORD *argv);

/*ɱ������*/
void ApiKillProcess(DWORD *argv);

/*ע���ں˶˿ڶ�Ӧ�߳�*/
void ApiRegKnlPort(DWORD *argv);

/*ע���ں˶˿ڶ�Ӧ�߳�*/
void ApiUnregKnlPort(DWORD *argv);

/*ȡ���ں˶˿ڶ�Ӧ�߳�*/
void ApiGetKpToThed(DWORD *argv);

/*ע��IRQ�źŵ���Ӧ�߳�*/
void ApiRegIrq(DWORD *argv);

/*ע��IRQ�źŵ���Ӧ�߳�*/
void ApiUnregIrq(DWORD *argv);

/*������Ϣ*/
void ApiSendMsg(DWORD *argv);

/*������Ϣ*/
void ApiRecvMsg(DWORD *argv);

/*�������ȴ���Ϣ*/
void ApiWaitMsg(DWORD *argv);

/*ӳ��������ַ*/
void ApiMapPhyAddr(DWORD *argv);

/*ӳ���û���ַ*/
void ApiMapUserAddr(DWORD *argv);

/*�����û���ַ��*/
void ApiFreeAddr(DWORD *argv);

#endif
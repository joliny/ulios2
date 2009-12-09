/*	page.h for ulios
	作者：孙亮
	功能：分页内存管理定义
	最后修改日期：2009-05-29
*/

#ifndef _PAGE_H_
#define _PAGE_H_

#include "ulidef.h"

#define PAGE_SIZE	0x00001000

/*初始化物理内存管理*/
long InitPMM();

/*分配物理页,返回物理地址,无法分配返回0*/
DWORD AllocPage();

/*回收物理页,参数:物理地址*/
void FreePage(DWORD pgaddr);

/*映射物理地址*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz);

/*映射用户地址*/
long MapUserAddr(void **addr, DWORD siz);

/*解除映射地址*/
void UnmapAddr(void *addr, DWORD siz, DWORD attr);

/*映射地址块给别的进程,并发送开始消息*/
long MapProcAddr(void *ProcAddr, DWORD siz, THREAD_ID ptid, BOOL isReadonly);

/*解除映射进程共享地址,并发送完成消息*/
long UnmapProcAddr(void *addr, DWORD siz, THREAD_ID ptid);

/*页故障处理程序*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

#endif

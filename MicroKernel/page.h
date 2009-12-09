/*	page.h for ulios
	���ߣ�����
	���ܣ���ҳ�ڴ������
	����޸����ڣ�2009-05-29
*/

#ifndef _PAGE_H_
#define _PAGE_H_

#include "ulidef.h"

#define PAGE_SIZE	0x00001000

/*��ʼ�������ڴ����*/
long InitPMM();

/*��������ҳ,���������ַ,�޷����䷵��0*/
DWORD AllocPage();

/*��������ҳ,����:�����ַ*/
void FreePage(DWORD pgaddr);

/*ӳ�������ַ*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz);

/*ӳ���û���ַ*/
long MapUserAddr(void **addr, DWORD siz);

/*���ӳ���ַ*/
void UnmapAddr(void *addr, DWORD siz, DWORD attr);

/*ӳ���ַ�����Ľ���,�����Ϳ�ʼ��Ϣ*/
long MapProcAddr(void *ProcAddr, DWORD siz, THREAD_ID ptid, BOOL isReadonly);

/*���ӳ����̹����ַ,�����������Ϣ*/
long UnmapProcAddr(void *addr, DWORD siz, THREAD_ID ptid);

/*ҳ���ϴ������*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

#endif

/*	page.h for ulios
	���ߣ�����
	���ܣ���ҳ�ڴ������
	����޸����ڣ�2009-05-29
*/

#ifndef _PAGE_H_
#define _PAGE_H_

#include "ulidef.h"

#define MAPMT_LEN			0x1000	/*��ַӳ��������*/
typedef struct _MAPBLK_DESC
{
	void *addr;					/*ӳ�������ʼ��ַ*/
	void *addr2;				/*��ӳ�������ʼ��ַ*/
	DWORD siz;					/*�ֽ���*/
	THREAD_ID ptid;				/*ӳ���߳�*/
	THREAD_ID ptid2;			/*��ӳ���߳�*/
	struct _MAPBLK_DESC *nxt;	/*ӳ����̺�һ��*/
	struct _MAPBLK_DESC *nxt2;	/*��ӳ����̺�һ��*/
}MAPBLK_DESC;	/*��ַӳ������������*/

#define PAGE_SIZE	0x00001000

/*��������ҳ,���������ַ,�޷����䷵��0*/
DWORD AllocPage();

/*��������ҳ,����:�����ַ*/
void FreePage(DWORD pgaddr);

/*�����ַӳ��ṹ*/
MAPBLK_DESC *AllocMap();

/*�ͷŵ�ַӳ��ṹ*/
void FreeMap(MAPBLK_DESC *map);

/*���������ҳ��ַ*/
long FillConAddr(PAGE_DESC *FstPg, PAGE_DESC *EndPg, DWORD PhyAddr, DWORD attr);

/*���ҳ����*/
long FillPage(EXEC_DESC *exec, void *addr, DWORD ErrCode);

/*���ҳ����*/
void ClearPage(PAGE_DESC *FstPg, PAGE_DESC *EndPg, BOOL isFree);

/*���ҳ����(���������)*/
void ClearPageNoPt0(PAGE_DESC *FstPg, PAGE_DESC *EndPg);

/*ӳ�������ַ*/
long MapPhyAddr(void **addr, DWORD PhyAddr, DWORD siz);

/*ӳ���û���ַ*/
long MapUserAddr(void **addr, DWORD siz);

/*���ӳ���ַ*/
long UnmapAddr(void *addr);

/*ӳ���ַ�����Ľ���,�����Ϳ�ʼ��Ϣ*/
long MapProcAddr(void *addr, DWORD siz, THREAD_ID *ptid, BOOL isWrite, BOOL isChkExec, DWORD *argv, DWORD cs);

/*���ӳ����̹����ַ,�����������Ϣ*/
long UnmapProcAddr(void *addr, const DWORD *argv);

/*ȡ��ӳ����̹����ַ,������ȡ����Ϣ*/
long CnlmapProcAddr(void *addr, const DWORD *argv);

/*������̵�ӳ�����*/
void FreeAllMap();

/*ҳ���ϴ������*/
void PageFaultProc(DWORD edi, DWORD esi, DWORD ebp, DWORD esp, DWORD ebx, DWORD edx, DWORD ecx, DWORD eax, WORD gs, WORD fs, WORD es, WORD ds, DWORD IsrN, DWORD ErrCode, DWORD eip, WORD cs, DWORD eflags);

#endif

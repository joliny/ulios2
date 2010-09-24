/*	global.c for ulios
	���ߣ�����
	���ܣ�ȫ�ֱ��������ú�������
	����޸����ڣ�2009-05-26
*/

#include "knldef.h"

/**********�ں����ݱ�**********/

/*����ģʽ��ر�*/
SEG_GATE_DESC gdt[GDT_LEN];	/*ȫ����������2KB*/
SEG_GATE_DESC idt[IDT_LEN];	/*�ж���������2KB*/
PAGE_DESC kpdt[PDT_LEN];	/*�ں�ҳĿ¼��4KB*/
PAGE_DESC pddt[PDT_LEN];	/*ҳĿ¼����Ŀ¼��4KB*/
PAGE_DESC pddt0[PDT_LEN];	/*ҳĿ¼��������Ŀ¼��4KB*/
/*�ں˹�����*/
FREE_BLK_DESC kmmt[FMT_LEN];/*�ں��������ݹ�����12KB(0���)*/
PROCESS_DESC* pmt[PMT_LEN];	/*���̹�����4KB*/
EXEC_DESC* exmt[EXMT_LEN];	/*��ִ���������4KB*/
THREAD_ID kpt[KPT_LEN];		/*�ں˶˿�ע���4KB*/

/**********�ں���ɢ����**********/

BYTE KnlValue[KVAL_LEN];	/*�ں���ɢ������4KB*/

/**********ʵģʽ���õı���**********/

PHYBLK_DESC BaseSrv[16];	/*8 * 16�ֽڻ����������α�*/
DWORD MemEnd;			/*�ڴ�����*/
MEM_ARDS ards[1];		/*20 * N�ֽ��ڴ�ṹ��*/

/**********�ں˴��������**********/

BYTE gramem[0x20000];	/*ͼ���Դ�128K*/
BYTE txtmem[0x8000];	/*�ı��Դ�32K*/
BYTE kdat[KDAT_SIZ];	/*�ں���������*/

/**********��ҳ������ر�**********/

/*ռ��24M�����ַ�ռ�*/
PAGE_DESC pdt[PT_LEN];		/*���н���ҳĿ¼��4MB*/
PAGE_DESC pdt0[PT_LEN];		/*���н��̸���ҳĿ¼��4MB*/
PAGE_DESC pt[PT_LEN];		/*��ǰ����ҳ��4MB*/
PAGE_DESC pt0[PT_LEN];		/*��ǰ���̸���ҳ��4MB*/
PAGE_DESC pt2[PT_LEN];		/*��ϵ����ҳ��4MB*/
BYTE pg0[PG_LEN];			/*��ǰҳ����4MB*/

/*ע���ں˶˿ڶ�Ӧ�߳�*/
long RegKnlPort(DWORD PortN)
{
	if (PortN >= KPT_LEN)
		return ERROR_WRONG_KPTN;
	cli();
	if (*(DWORD*)(&kpt[PortN]) != INVALID)
	{
		sti();
		return ERROR_KPT_ISENABLED;
	}
	kpt[PortN] = CurPmd->CurTmd->id;
	sti();
	return NO_ERROR;
}

/*ע���ں˶˿ڶ�Ӧ�߳�*/
long UnregKnlPort(DWORD PortN)
{
	if (PortN >= KPT_LEN)
		return ERROR_WRONG_KPTN;
	cli();
	if (*(DWORD*)(&kpt[PortN]) == INVALID)
	{
		sti();
		return ERROR_KPT_ISDISABLED;
	}
	if (kpt[PortN].ProcID != CurPmd->CurTmd->id.ProcID)
	{
		sti();
		return ERROR_KPT_WRONG_CURPROC;
	}
	*(DWORD*)(&kpt[PortN]) = INVALID;
	sti();
	return NO_ERROR;
}

/*ȡ���ں˶˿ڶ�Ӧ�߳�*/
long GetKptThed(DWORD PortN, THREAD_ID *ptid)
{
	if (PortN >= KPT_LEN)
		return ERROR_WRONG_KPTN;
	cli();
	if (*(DWORD*)(&kpt[PortN]) == INVALID)
	{
		sti();
		return ERROR_KPT_ISDISABLED;
	}
	*ptid = kpt[PortN];
	sti();
	return NO_ERROR;
}

/*ע���̵߳������ں˶˿�*/
long UnregAllKnlPort()
{
	THREAD_ID ptid;
	DWORD i;

	ptid = CurPmd->CurTmd->id;
	cli();
	for (i = 0; i < KPT_LEN; i++)
		if (*(DWORD*)(&kpt[i]) == *(DWORD*)(&ptid))
			*(DWORD*)(&kpt[i]) = INVALID;
	sti();
	return NO_ERROR;
}
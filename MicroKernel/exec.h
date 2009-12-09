/*	exec.h for ulios
	���ߣ�����
	���ܣ���ִ������������̳߳�ʼ������
	����޸����ڣ�2009-09-01
*/

#ifndef _EXEC_H_
#define _EXEC_H_

#include "ulidef.h"

#define EXEC_ARGS_SIZ		512			/*�½������ļ�·������*/

#define EXEC_ARGS_BASESRV	0x00000001	/*0:�ļ�����1:��������*/
#define EXEC_ARGS_DRIVER	0x00000002	/*0:Ӧ�ý���1:��������*/

typedef struct _EXEC_DESC
{
	WORD id, file;	/*��ִ����ID,�ļ�������*/
	void *CodeOff;	/*����ο�ʼ��ַ*/
	void *CodeEnd;	/*����ν�����ַ*/
	DWORD CodeSeek;	/*������ļ�ƫ��*/
	void *DataOff;	/*���ݶο�ʼ��ַ*/
	void *DataEnd;	/*���ݶν�����ַ*/
	DWORD DataSeek;	/*���ݶ��ļ�ƫ��*/
	void *BssEnd;	/*�����������ַ*/
	void *entry;	/*��ڵ�*/
	DWORD cou;		/*ʹ�ü���*/
	volatile DWORD Page_l;	/*��ҳ������*/
}EXEC_DESC;	/*��ִ����ṹ*/

/*��ʼ����ִ��������*/
void InitEXMT();

/*����տ�ִ����ID*/
long AllocExid(EXEC_DESC *exec);

/*�ͷſտ�ִ����ID*/
void FreeExid(EXEC_DESC **exmd);

/*�߳����*/
void ThedStart();

/*�������*/
void ProcStart();

#endif

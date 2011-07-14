/*	exec.h for ulios
	���ߣ�����
	���ܣ���ִ������������̳߳�ʼ������
	����޸����ڣ�2009-09-01
*/

#ifndef _EXEC_H_
#define _EXEC_H_

#include "ulidef.h"

#define EXEC_ARGV_BASESRV	0x00000001	/*0:�ļ�����1:��������*/
#define EXEC_ARGV_DRIVER	0x00000002	/*0:Ӧ�ý���1:��������*/
#define PROC_EXEC_SIZE		0x400		/*����·��������ֽ���*/
#define PROC_ARGS_SIZE		0x200		/*���̲�������ֽ���*/
#define REP_KPORT			0			/*����ϵͳ�����̶߳˿�*/
#define FS_KPORT			1			/*�ļ�ϵͳ�̶߳˿�*/
#define FS_API_GETEXEC		0			/*�����ļ�ϵͳȡ�ÿ�ִ���ļ���Ϣ���ܺ�*/
#define FS_API_READPAGE		1			/*�����ļ�ϵͳ��ȡ��ִ���ļ�ҳ���ܺ�*/
#define FS_OUT_TIME			6000		/*�ļ�ϵͳ��ʱʱ��*/

typedef struct _EXEC_DESC
{
	WORD cou;		/*ʹ�ü���*/
	volatile WORD Page_l;	/*��ҳ������*/
	void *CodeOff;	/*����ο�ʼ��ַ*/
	void *CodeEnd;	/*����ν�����ַ*/
	DWORD CodeSeek;	/*������ļ�ƫ��*/
	void *DataOff;	/*���ݶο�ʼ��ַ*/
	void *DataEnd;	/*���ݶν�����ַ*/
	DWORD DataSeek;	/*���ݶ��ļ�ƫ��*/
	void *entry;	/*��ڵ�*/
}EXEC_DESC;	/*��ִ����ṹ*/

/*�߳����*/
void ThedStart();

/*�������*/
void ProcStart();

/*ɾ���߳���Դ���˳�*/
void ThedExit(DWORD ExitCode);

#endif

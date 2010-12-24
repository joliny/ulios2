/*	bootdata.h for ulios
	���ߣ�����
	���ܣ�ʵģʽ�³�ʼ�����ݽṹ����
	����޸����ڣ�2009-06-17
*/

#ifndef	_BOOTDATA_H_
#define	_BOOTDATA_H_

#include "ulidef.h"

typedef struct _PHYBLK_DESC
{
	DWORD addr;	/*��ʼ�����ַ*/
	DWORD siz;	/*�ֽ���*/
}PHYBLK_DESC;	/*�����ַ��������*/

#define ARDS_TYPE_RAM		1	/*��������洢��*/
#define ARDS_TYPE_RESERVED	2	/*����*/
#define ARDS_TYPE_ACPI		3	/*ACPIר��*/
#define ARDS_TYPE_NVS		4	/*���ӷ��洢��(ROM/FLASH)*/

typedef struct _MEM_ARDS
{
	DWORD addr;	/*��ʼ�����ַ*/
	DWORD addrhi;
	DWORD siz;	/*�ֽ���*/
	DWORD sizhi;
	DWORD type;	/*����*/
}MEM_ARDS;	/*�ڴ�ṹ��*/

/**********��������**********/

extern PHYBLK_DESC BaseSrv[];	/*8 * 16�ֽڻ����������α�*/
extern DWORD MemEnd;		/*�ڴ�����*/
extern MEM_ARDS ards[];		/*20 * 19�ֽ��ڴ�ṹ��*/

#endif

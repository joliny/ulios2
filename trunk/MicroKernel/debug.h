/*	debug.h for ulios
	���ߣ�����
	���ܣ��ں˵�����ض���
	����޸����ڣ�2009-05-29
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "ulidef.h"

#define DebugMsg	Print		/*������Ϣ*/

/*����ַ�*/
void PutChar(char c);

/*�����ֵ*/
void PutNum(DWORD n, DWORD r);

/*����ַ���*/
void PutS(const char *str);

/*��ʽ�����*/
void Print(const char *fmtstr, ...);

#endif

/*	malloc.h for ulios
	���ߣ�����
	���ܣ��û��ڴ�ѹ�����
	����޸����ڣ�2009-05-29
*/

#ifndef	_MALLOC_H_
#define	_MALLOC_H_

#include "../MkApi/ulimkapi.h"

/*��ʼ���ѹ����*/
long InitMallocTab(DWORD siz);

/*�ڴ����*/
void *malloc(DWORD siz);

/*�ڴ����*/
void free(void *addr);

/*�ڴ��ط���*/
void *realloc(void *addr, DWORD siz);

#endif

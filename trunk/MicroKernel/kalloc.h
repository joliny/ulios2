/*	kalloc.h for ulios
	���ߣ�����
	���ܣ��ں˶�̬�������
	����޸����ڣ�2009-05-29
*/

#ifndef	_KALLOC_H_
#define	_KALLOC_H_

#include "ulidef.h"

typedef struct _FREE_BLK_DESC
{
	void *addr;					/*��ʼ��ַ*/
	DWORD siz;					/*�ֽ���*/
	struct _FREE_BLK_DESC *nxt;	/*��һ��*/
}FREE_BLK_DESC;	/*���ɿ�������*/

/*��ʼ�����ɿ�����*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz);

/*���ɿ����*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz);

/*���ɿ����*/
void free(FREE_BLK_DESC *fbt, void *addr, DWORD siz);

#endif

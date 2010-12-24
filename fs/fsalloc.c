/*	fsalloc.c for ulios file system
	���ߣ�����
	���ܣ��ļ�ϵͳ��̬�ڴ����
	����޸����ڣ�2009-05-28
*/

#include "fs.h"

FREE_BLK_DESC fmt[FMT_LEN];
DWORD fmtl;	/*��̬�ڴ������*/

/*���ɿ����*/
void *malloc(DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	lock(&fmtl);
	if (siz == 0 || siz > fmt->siz)	/*û���㹻�ռ�*/
	{
		ulock(&fmtl);
		return NULL;
	}
	for (CurFblk = (PreFblk = fmt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (CurFblk->siz >= siz)	/*�״�ƥ�䷨*/
		{
			void *addr;
			fmt->siz -= siz;
			if ((CurFblk->siz -= siz) == 0)	/*�����ѿ�*/
			{
				PreFblk->nxt = CurFblk->nxt;	/*ȥ������*/
				CurFblk->nxt = (FREE_BLK_DESC*)fmt->addr;	/*�ͷű���*/
				fmt->addr = (void*)CurFblk;
			}
			addr = CurFblk->addr + CurFblk->siz;
			ulock(&fmtl);
			return addr;
		}
	ulock(&fmtl);
	return NULL;
}

/*���ɿ����*/
void free(void *addr, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk, *TmpFblk;

	lock(&fmtl);
	fmt->siz += siz;
	for (CurFblk = (PreFblk = fmt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (addr < CurFblk->addr)
			break;	/*�����ͷ�λ��*/
	if (PreFblk != fmt)
		if (CurFblk)
			if (PreFblk->addr + PreFblk->siz == addr)	/*�Ƿ���ǰ�տ����*/
				if (addr + siz == CurFblk->addr)	/*�Ƿ����տ����*/
					goto link;
				else
					goto addpre;
			else
				if (addr + siz == CurFblk->addr)	/*�Ƿ����տ����*/
					goto addnxt;
				else
					goto creat;
		else
			if (PreFblk->addr + PreFblk->siz == addr)	/*�Ƿ���ǰ�տ����*/
				goto addpre;
			else
				goto creat;
	else
		if (CurFblk && addr + siz == CurFblk->addr)	/*�Ƿ����տ����*/
			goto addnxt;
		else
			goto creat;
creat:	/*�½�������*/
	TmpFblk = (FREE_BLK_DESC*)fmt->addr;
	if (TmpFblk == NULL)	/*�޿ձ���,�޷��ͷ�*/
		return;
	fmt->addr = (void*)TmpFblk->nxt;	/*�������*/
	TmpFblk->addr = addr;
	TmpFblk->siz = siz;
	TmpFblk->nxt = PreFblk->nxt;
	PreFblk->nxt = TmpFblk;
	ulock(&fmtl);
	return;
addpre:	/*���뵽ǰ��*/
	PreFblk->siz += siz;
	ulock(&fmtl);
	return;
addnxt:	/*���뵽����*/
	CurFblk->addr = addr;
	CurFblk->siz += siz;
	ulock(&fmtl);
	return;
link:	/*����ǰ��������*/
	PreFblk->siz += (siz + CurFblk->siz);
	PreFblk->nxt = CurFblk->nxt;
	CurFblk->nxt = (FREE_BLK_DESC*)fmt->addr;	/*�ͷű���*/
	fmt->addr = (void*)CurFblk;
	ulock(&fmtl);
	return;
}

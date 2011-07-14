/*	kalloc.c for ulios
	���ߣ�����
	���ܣ��ں˶�̬�����
	����޸����ڣ�2009-05-28
*/

#include "knldef.h"

/*��ʼ�����ɿ�����*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz)
{
	FREE_BLK_DESC *fbd;

	fbt->addr = &fbt[2];	/*0���1������2�Ժ�Ŀ���*/
	fbt->siz = siz;
	fbt->nxt = &fbt[1];
	fbt[1].addr = addr;
	fbt[1].siz = siz;
	fbt[1].nxt = NULL;
	for (fbd = &fbt[2]; fbd < &fbt[FbtLen - 1]; fbd++)
		fbd->nxt = fbd + 1;
	fbd->nxt = NULL;
}

/*���ɿ����*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	if (siz > fbt->siz)	/*û���㹻�ռ�*/
		return NULL;
	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (CurFblk->siz >= siz)	/*�״�ƥ�䷨*/
		{
			fbt->siz -= siz;
			if ((CurFblk->siz -= siz) == 0)	/*�����ѿ�*/
			{
				PreFblk->nxt = CurFblk->nxt;	/*ȥ������*/
				CurFblk->nxt = (FREE_BLK_DESC*)fbt->addr;	/*�ͷű���*/
				fbt->addr = (void*)CurFblk;
			}
			return CurFblk->addr + CurFblk->siz;
		}
	return NULL;
}

/*���ɿ����*/
void free(FREE_BLK_DESC *fbt, void *addr, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk, *TmpFblk;

	fbt->siz += siz;
	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (addr < CurFblk->addr)
			break;	/*�����ͷ�λ��*/
	if (PreFblk != fbt)
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
	TmpFblk = (FREE_BLK_DESC*)fbt->addr;
	if (TmpFblk == NULL)	/*�޿ձ���,�޷��ͷ�*/
		return;
	fbt->addr = (void*)TmpFblk->nxt;	/*�������*/
	TmpFblk->addr = addr;
	TmpFblk->siz = siz;
	TmpFblk->nxt = PreFblk->nxt;
	PreFblk->nxt = TmpFblk;
	return;
addpre:	/*���뵽ǰ��*/
	PreFblk->siz += siz;
	return;
addnxt:	/*���뵽����*/
	CurFblk->addr = addr;
	CurFblk->siz += siz;
	return;
link:	/*����ǰ��������*/
	PreFblk->siz += (siz + CurFblk->siz);
	PreFblk->nxt = CurFblk->nxt;
	CurFblk->nxt = (FREE_BLK_DESC*)fbt->addr;	/*�ͷű���*/
	fbt->addr = (void*)CurFblk;
	return;
}

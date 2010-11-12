/*	malloc.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û����涯̬�ڴ����
	����޸����ڣ�2009-05-28
*/

#include "gui.h"

FREE_BLK_DESC fmt[FMT_LEN];
DWORD fmtl;	/*��̬�ڴ������*/
FREE_BLK_DESC vmt[VMT_LEN];
DWORD vmtl;	/*�����ڴ������*/

/*��ʼ�����ɿ�����*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz)
{
	fbt->addr = &fbt[2];	/*0���1������2�Ժ�Ŀ���*/
	fbt->siz = siz;
	fbt->nxt = &fbt[1];
	fbt[1].addr = addr;
	fbt[1].siz = siz;
	fbt[1].nxt = NULL;
	memset32(&fbt[2], 0, (FbtLen - 2) * sizeof(FREE_BLK_DESC) / sizeof(DWORD));
}

/*���ɿ����*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	if (siz == 0 || siz > fbt->siz)	/*û���㹻�ռ�*/
		return NULL;
	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (CurFblk->siz >= siz)	/*�״�ƥ�䷨*/
		{
			fbt->siz -= siz;
			if ((CurFblk->siz -= siz) == 0)	/*�����ѿ�*/
			{
				PreFblk->nxt = CurFblk->nxt;	/*ȥ������*/
				if (fbt->addr > (void*)CurFblk)
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
	TmpFblk->addr = addr;
	TmpFblk->siz = siz;
	TmpFblk->nxt = PreFblk->nxt;
	PreFblk->nxt = TmpFblk;
	do
		TmpFblk++;
	while (TmpFblk->siz);	/*	while (TmpFblk < &fbt[FBT_LEN] && TmpFblk->siz);*/
	fbt->addr = (void*)TmpFblk;
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
	CurFblk->siz = 0;
	if (fbt->addr > (void*)CurFblk)
		fbt->addr = (void*)CurFblk;
	return;
}

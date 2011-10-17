/*	malloc.c for ulios
	���ߣ�����
	���ܣ��û��ڴ�ѹ���ʵ��
	����޸����ڣ�2009-05-28
*/

#include "malloc.h"

typedef struct _FREE_BLK_DESC
{
	void *addr;					/*��ʼ��ַ*/
	DWORD siz;					/*�ֽ���*/
	struct _FREE_BLK_DESC *nxt;	/*��һ��*/
}FREE_BLK_DESC;	/*���ɿ�������*/

#define FBT_LEN		256

FREE_BLK_DESC fbt[FBT_LEN];

/*��ʼ���ѹ����*/
long InitMallocTab(DWORD siz)
{
	FREE_BLK_DESC *fbd;
	void *addr;
	long res;

	if ((res = KMapUserAddr(&addr, siz)) != NO_ERROR)
		return res;
	fbt->addr = &fbt[2];	/*0���1������2�Ժ�Ŀ���*/
	fbt->siz = siz;
	fbt->nxt = &fbt[1];
	fbt[1].addr = addr;
	fbt[1].siz = siz;
	fbt[1].nxt = NULL;
	for (fbd = &fbt[2]; fbd < &fbt[sizeof(fbt) / sizeof(FREE_BLK_DESC) - 1]; fbd++)
		fbd->nxt = fbd + 1;
	fbd->nxt = NULL;
	return NO_ERROR;
}

/*���ɿ����*/
static void *AllocBlk(DWORD siz)
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
static void FreeBlk(void *addr, DWORD siz)
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

/*���ɿ����ӷ���*/
static BOOL LinkBlk(void *addr, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (addr <= CurFblk->addr)	/*��������λ��*/
		{
			if (addr == CurFblk->addr && CurFblk->siz >= siz)	/*ƥ��ɹ�*/
			{
				fbt->siz -= siz;
				if ((CurFblk->siz -= siz) == 0)	/*�����ѿ�*/
				{
					PreFblk->nxt = CurFblk->nxt;	/*ȥ������*/
					CurFblk->nxt = (FREE_BLK_DESC*)fbt->addr;	/*�ͷű���*/
					fbt->addr = (void*)CurFblk;
				}
				CurFblk->addr += siz;
				return TRUE;	/*���ӳɹ�*/
			}
			return FALSE;
		}
	return FALSE;
}

/*�ڴ����*/
void *malloc(DWORD siz)
{
	void *addr;

	if ((long)siz <= 0)
		return NULL;
	siz = (siz + sizeof(DWORD) - 1) & 0xFFFFFFFC;	/*32λ����*/
	if ((addr = AllocBlk(siz + sizeof(DWORD))) == NULL)
		return NULL;
	*(DWORD*)addr = siz;	/*���ó�����*/
	return addr + sizeof(DWORD);
}

/*�ڴ����*/
void free(void *addr)
{
	if (addr == NULL)
		return;
	addr -= sizeof(DWORD);	/*ȡ�ó����ֵ�ַ*/
	FreeBlk(addr, *(DWORD*)addr + sizeof(DWORD));
}

/*�ڴ��ط���*/
void *realloc(void *addr, DWORD siz)
{
	DWORD oldsiz;

	if (addr == NULL)
		return malloc(siz);
	if (siz == 0)
	{
		free(addr);
		return NULL;
	}
	oldsiz = *(DWORD*)(addr - sizeof(DWORD));	/*ȡ�ó�����*/
	siz = (siz + sizeof(DWORD) - 1) & 0xFFFFFFFC;	/*32λ����*/
	if (siz < oldsiz)	/*��С*/
	{
		FreeBlk(addr + siz, oldsiz - siz);
		*(DWORD*)(addr - sizeof(DWORD)) = siz;
		return addr;
	}
	else if (siz > oldsiz)	/*����*/
	{
		if (siz - oldsiz > fbt->siz)	/*û���㹻�ռ�*/
			return NULL;
		if (LinkBlk(addr + oldsiz, siz - oldsiz))	/*���ӵ�ԭ�����*/
		{
			*(DWORD*)(addr - sizeof(DWORD)) = siz;
			return addr;
		}
		else	/*���·���*/
		{
			free(addr);
			return malloc(siz);
		}
	}
	return addr;	/*�޸ı�,����ԭ��ַ*/
}

/*	malloc.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面动态内存管理
	最后修改日期：2009-05-28
*/

#include "gui.h"

FREE_BLK_DESC fmt[FMT_LEN];
DWORD fmtl;	/*动态内存管理锁*/
FREE_BLK_DESC vmt[VMT_LEN];
DWORD vmtl;	/*可视内存管理锁*/

/*初始化自由块管理表*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz)
{
	fbt->addr = &fbt[2];	/*0项不用1项已用2以后的空闲*/
	fbt->siz = siz;
	fbt->nxt = &fbt[1];
	fbt[1].addr = addr;
	fbt[1].siz = siz;
	fbt[1].nxt = NULL;
	memset32(&fbt[2], 0, (FbtLen - 2) * sizeof(FREE_BLK_DESC) / sizeof(DWORD));
}

/*自由块分配*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk;

	if (siz == 0 || siz > fbt->siz)	/*没有足够空间*/
		return NULL;
	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (CurFblk->siz >= siz)	/*首次匹配法*/
		{
			fbt->siz -= siz;
			if ((CurFblk->siz -= siz) == 0)	/*表项已空*/
			{
				PreFblk->nxt = CurFblk->nxt;	/*去除表项*/
				if (fbt->addr > (void*)CurFblk)
					fbt->addr = (void*)CurFblk;
			}
			return CurFblk->addr + CurFblk->siz;
		}
	return NULL;
}

/*自由块回收*/
void free(FREE_BLK_DESC *fbt, void *addr, DWORD siz)
{
	FREE_BLK_DESC *CurFblk, *PreFblk, *TmpFblk;

	fbt->siz += siz;
	for (CurFblk = (PreFblk = fbt)->nxt; CurFblk; CurFblk = (PreFblk = CurFblk)->nxt)
		if (addr < CurFblk->addr)
			break;	/*搜索释放位置*/
	if (PreFblk != fbt)
		if (CurFblk)
			if (PreFblk->addr + PreFblk->siz == addr)	/*是否与前空块相接*/
				if (addr + siz == CurFblk->addr)	/*是否与后空块相接*/
					goto link;
				else
					goto addpre;
			else
				if (addr + siz == CurFblk->addr)	/*是否与后空块相接*/
					goto addnxt;
				else
					goto creat;
		else
			if (PreFblk->addr + PreFblk->siz == addr)	/*是否与前空块相接*/
				goto addpre;
			else
				goto creat;
	else
		if (CurFblk && addr + siz == CurFblk->addr)	/*是否与后空块相接*/
			goto addnxt;
		else
			goto creat;
creat:	/*新建描述符*/
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
addpre:	/*加入到前面*/
	PreFblk->siz += siz;
	return;
addnxt:	/*加入到后面*/
	CurFblk->addr = addr;
	CurFblk->siz += siz;
	return;
link:	/*连接前后描述符*/
	PreFblk->siz += (siz + CurFblk->siz);
	PreFblk->nxt = CurFblk->nxt;
	CurFblk->siz = 0;
	if (fbt->addr > (void*)CurFblk)
		fbt->addr = (void*)CurFblk;
	return;
}

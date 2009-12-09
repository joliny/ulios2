/*	ulifs.c for ulios
	作者：孙亮
	功能：ULIFS文件系统实现
	最后修改日期：2009-06-06
*/

#include "../include/kernel.h"
#include "../include/error.h"

/*ulios分区结构(单位:扇区):
|引导|保留|坏簇|使用|数据|剩余
|记录|扇区|位图|位图|簇区|扇区
*/
typedef struct
{
	DWORD	INS_JMP;/*跳转指令*/
	BYTE	OEM[8];	/*OEM ID(tian&uli2k_X)*/
	DWORD	fsid;	/*文件系统标志"ULTN"四个字符,文件系统代码:0x7C*/
	WORD	ver;	/*版本号0,以下为0版本的BPB*/
	WORD	bps;	/*每扇区字节数*/
	WORD	spc;	/*每簇扇区数*/
	WORD	res;	/*保留扇区数,包括引导记录*/
	DWORD	secoff;	/*分区在磁盘上的起始扇区偏移*/
	DWORD	seccou;	/*分区占总扇区数,包括剩余扇区*/
	WORD	spbm;	/*每个位图占扇区数*/
	WORD	cluoff;	/*分区内首簇开始扇区偏移*/
	DWORD	clucou;	/*数据簇数目*/
	BYTE	knlpath[88];/*内核路径*/
	BYTE	code[382];	/*引导代码*/
	WORD	aa55;	/*引导标志*/
}ULIFS_BPB;	/*ulifs BIOS Parameter Block*/

typedef struct
{
	DWORD spc;	/*每簇扇区数*/
	DWORD res;	/*保留扇区数(位图之前的扇区)*/
	DWORD ubm;	/*使用位图开始扇区*/
	DWORD spbm;	/*每个位图占扇区数*/
	DWORD CluID;/*首簇扇区号*/
	DWORD CuCou;/*簇数量*/
	DWORD FstCu;/*第一个空簇号*/
	DWORD RemCu;/*剩余簇数量*/
	DWORD ubm_l;/*使用位图锁*/
}ULIFS;	/*运行时ulifs信息结构*/

/*ulifs文件系统的核心结构:块索引节点(非常简单^_^)
fst是被指向的连续簇块的首簇号,从0开始计数,
cou是被索引的相连数据簇的个数,但它的值可以是0,
是0表示这个节点指向下一个索引节点簇,而不是数据簇*/
typedef struct
{
	DWORD fst;	/*首簇*/
	DWORD cou;	/*数量*/
}BLKID;	/*块索引节点*/

/*ulifs文件系统目录项结构(单位:字节):
注:目录中没有.和..目录项
根目录0号目录项为根目录的目录项,名称为分区卷标,以'/'开头,但不包括'/'*/
#define ULIFS_FILE_NAME_SIZE	80
#define ULIFS_FILE_ATTR_RDONLY	0x00000001	/*只读*/
#define ULIFS_FILE_ATTR_HIDDEN	0x00000002	/*隐藏*/
#define ULIFS_FILE_ATTR_SYSTEM	0x00000004	/*系统*/
#define ULIFS_FILE_ATTR_LABEL	0x00000008	/*卷标*/
#define ULIFS_FILE_ATTR_DIREC	0x00000010	/*目录(只读)*/
#define ULIFS_FILE_ATTR_ARCH	0x00000020	/*归档*/
#define ULIFS_FILE_ATTR_EXEC	0x00000040	/*可执行*/
#define ULIFS_FILE_ATTR_UNMDFY	0x80000000	/*属性不可修改*/

typedef struct	/*目录项结构*/
{
	BYTE name[ULIFS_FILE_NAME_SIZE];/*文件名*/
	DWORD CreateTime;	/*创建时间1970-01-01经过的秒数*/
	DWORD ModifyTime;	/*修改时间*/
	DWORD AccessTime;	/*访问时间*/
	DWORD attr;			/*属性*/
	QWORD size;			/*文件字节数,目录文件的字节数有效*/
	BLKID idx[3];		/*最后有3个自带的索引块,所以一个文件有可能用不到索引簇*/
}ULIFS_DIR;	/*目录项结构*/

#define ULIFS_MAX_DIR	0x10000		/*目录中目录项数量极限*/

/*读写分区扇区的宏*/
#define RwPart(ptd, isWrite, sec, cou, buf) \
RwBuf((ptd)->DevID, (isWrite), (sec), (cou), (buf))

/*读写簇的宏(分区号,读写,簇号,个数,缓冲区)*/
#define RwClu(ptd, isWrite, clu, cou, buf) \
RwBuf((ptd)->DevID, (isWrite), ((ULIFS*)((ptd)->data))->CluID + ((ULIFS*)((ptd)->data))->spc * (clu), ((ULIFS*)((ptd)->data))->spc * (cou), (buf))

/*挂载ULIFS分区*/
long MntUlifs(DWORD ptid)
{
	DWORD i;
	PART_DESC *CurPd;
	ULIFS *fs;
	BYTE *buf;
	long res;

	if ((fs = (ULIFS *)kmalloc(sizeof(ULIFS))) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	if ((buf = (BYTE *)kmalloc(512)) == NULL)
	{
		kfree(fs, sizeof(ULIFS));
		return ERROR_FS_HAVENO_MEMORY;
	}
	CurPd = part[ptid];
	res = RwPart(CurPd, FALSE, CurPd->SecID, 1, buf);	/*读引导扇区*/
	if (res != NO_ERROR)
	{
		kfree(buf, 512);
		kfree(fs, sizeof(ULIFS));
		return res;
	}
	if (((ULIFS_BPB*)buf)->aa55 != 0xAA55 || ((ULIFS_BPB*)buf)->fsid != 0x4E544C55 || ((ULIFS_BPB*)buf)->bps != 512)	/*检查引导标志和分区标志,暂不支持不是512字节的扇区*/
	{
		kfree(buf, 512);
		kfree(fs, sizeof(ULIFS));
		return ERROR_FS_PART_UNRECOGN;
	}
	fs->spc = ((ULIFS_BPB*)buf)->spc;
	fs->res = ((ULIFS_BPB*)buf)->res;
	fs->ubm = ((ULIFS_BPB*)buf)->secoff + fs->res + ((ULIFS_BPB*)buf)->spbm;
	fs->spbm = ((ULIFS_BPB*)buf)->spbm;
	fs->CluID = ((ULIFS_BPB*)buf)->secoff + ((ULIFS_BPB*)buf)->cluoff;
	fs->CuCou = ((ULIFS_BPB*)buf)->clucou;
	fs->RemCu = fs->FstCu = 0;
	ULOCK(fs->ubm_l);
	for (i = 0;; i += sizeof(DWORD) * 8)	/*查找空簇*/
	{
		DWORD bit;

		if ((i & 0xFFF) == 0)
		{
			res = RwPart(CurPd, FALSE, fs->ubm + (i >> 12), 1, buf);	/*读取使用位图扇区*/
			if (res != NO_ERROR)
			{
				kfree(buf, 512);
				kfree(fs, sizeof(ULIFS));
				return res;
			}
		}
		if ((bit = ((DWORD *)buf)[(i & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*有空簇*/
		{
			DWORD j;

			for (j = 0; j < sizeof(DWORD) * 8; j++)
				if (!(bit & (1lu << j)))	/*空簇*/
				{
					if (i + j >= fs->CuCou)
					{
						RwPart(CurPd, FALSE, fs->CluID, 1, buf);	/*取得卷标*/
						strcpy(CurPd->attr.label, &buf[1]);
						kfree(buf, 512);
						CurPd->attr.size = fs->CuCou * fs->spc * 512;
						CurPd->attr.remain = fs->RemCu * fs->spc * 512;
						CurPd->data = fs;
						return NO_ERROR;
					}
					if (fs->FstCu == 0)
						fs->FstCu = i + j;	/*取得首空簇号*/
					fs->RemCu++;	/*累计空簇数*/
				}
		}
	}
}

/*卸载ULIFS分区*/
void UmntUlifs(PART_DESC *ptd)
{
	kfree(ptd->data, sizeof(ULIFS));
	ptd->data = NULL;
}

/*设置分区信息*/
long SetPartUlifs(PART_DESC *ptd, PART_ATTR *pa)
{
	ULIFS_DIR *tmpdir;
	long res;

	if ((tmpdir = (ULIFS_DIR *)kmalloc(512)) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	res = RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->CluID, 1, tmpdir);	/*读取数据簇中的第一个扇区*/
	if (res != NO_ERROR)
	{
		kfree(tmpdir, 512);
		return res;
	}
	strcpy(&tmpdir->name[1], pa->label);
	res = RwPart(ptd, TRUE, ((ULIFS *)ptd->data)->CluID, 1, tmpdir);
	if (res != NO_ERROR)
	{
		kfree(tmpdir, 512);
		return res;
	}
	kfree(tmpdir, 512);
	return NO_ERROR;
}

/*格式化分区*/
long FmtPartUlifs(PART_DESC *ptd)
{
}

/*读写文件,数据不允许超出文件尾*/
long RwFileUlifs(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, BYTE *buf)
{
	DWORD idxi, bpc;/*索引节点索引,簇字节数*/
	QWORD dati, end;/*当前处理字节位置,处理字节结尾位置*/
	DWORD fst, cou;	/*临时索引节点*/
	PART_DESC *ptd;	/*所在分区结构指针*/
	BLKID *ip;	/*索引簇指针*/
	BYTE *dp;	/*数据簇指针*/
	long res;

	ptd = part[fd->PartID];
	bpc = ((ULIFS *)(ptd->data))->spc << 9;
	if ((ip = (BLKID *)kmalloc(bpc)) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	if ((dp = (BYTE *)kmalloc(bpc)) == NULL)
	{
		kfree(ip, bpc);
		return ERROR_FS_HAVENO_MEMORY;
	}
	idxi = (bpc >> 3) - 3;
	memcpy32(&ip[idxi], ((ULIFS_DIR *)fd->data)->idx, sizeof(BLKID) * 3 / sizeof(DWORD));	/*复制目录项中的索引*/
	dati = 0;
	end = seek + siz;
	for (;;)
	{
		for (;;)	/*索引簇*/
		{
			cou = ip[idxi].cou;
			if (cou == 0)
				break;
			fst = ip[idxi].fst;
			cou += fst;
			while (fst < cou)	/*索引节点*/
			{
				if (dati >= seek)	/*开头以后*/
				{
					if (dati + bpc > end)	/*末尾,完成*/
					{
						res = RwClu(ptd, FALSE, fst, 1, dp);
						if (res != NO_ERROR)
						{
							kfree(dp, bpc);
							kfree(ip, bpc);
							return res;
						}
						if (isWrite)
						{
							memcpy(dp, buf, (DWORD)(end % bpc));
							res = RwClu(ptd, TRUE, fst, 1, dp);
							if (res != NO_ERROR)
							{
								kfree(dp, bpc);
								kfree(ip, bpc);
								return res;
							}
						}
						else
							memcpy(buf, dp, (DWORD)(end % bpc));
						kfree(dp, bpc);
						kfree(ip, bpc);
						return NO_ERROR;
					}
					else	/*中间*/
					{
						DWORD cou1 = cou - fst;	/*连续簇数*/
						DWORD cou2 = (end - dati) / bpc;	/*剩余操作簇数*/
						if (cou1 > cou2)
							cou1 = cou2;
						cou2 = cou1 * bpc;
						res = RwClu(ptd, isWrite, fst, cou1, buf);
						if (res != NO_ERROR)
						{
							kfree(dp, bpc);
							kfree(ip, bpc);
							return res;
						}
						buf += cou2;
						fst += cou1;
						dati += cou2;
					}
				}
				else if (dati + bpc > seek)	/*将要开头*/
				{
					res = RwClu(ptd, FALSE, fst, 1, dp);
					if (res != NO_ERROR)
					{
						kfree(dp, bpc);
						kfree(ip, bpc);
						return res;
					}
					if (dati + bpc > end)	/*开头即末尾,完成*/
					{
						if (isWrite)
						{
							memcpy(dp + seek % bpc, buf, siz);
							res = RwClu(ptd, TRUE, fst, 1, dp);
							if (res != NO_ERROR)
							{
								kfree(dp, bpc);
								kfree(ip, bpc);
								return res;
							}
						}
						else
							memcpy(buf, dp + seek % bpc, siz);
						kfree(dp, bpc);
						kfree(ip, bpc);
						return NO_ERROR;
					}
					else	/*开头*/
					{
						if (isWrite)
						{
							memcpy(dp + seek % bpc, buf, bpc - (DWORD)(seek % bpc));
							res = RwClu(ptd, TRUE, fst, 1, dp);
							if (res != NO_ERROR)
							{
								kfree(dp, bpc);
								kfree(ip, bpc);
								return res;
							}
						}
						else
							memcpy(buf, dp + seek % bpc, bpc - (DWORD)(seek % bpc));
						buf += (bpc - seek % bpc);
						fst++;
						dati += bpc;
					}
				}
			}
			idxi++;
		}
		res = RwClu(ptd, FALSE, ip[idxi].fst, 1, ip);	/*读取下一个索引簇*/
		if (res != NO_ERROR)
		{
			kfree(dp, bpc);
			kfree(ip, bpc);
			return res;
		}
		idxi = 0;
	}
}

/*分配簇,不允许*cou为0,不允许无剩余空间,返回时*cou为实际申请簇数*/
long AllocClu(PART_DESC *ptd, DWORD *fst, DWORD *cou)
{
	DWORD i, end, *bm;	/*循环,结束位置,位图*/
	long res;

	if ((bm = (DWORD *)kmalloc(512)) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	(*fst) = i = ((ULIFS *)ptd->data)->FstCu;
	end = i + (*cou);
	res = RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->ubm + (i >> 12), 1, bm);
	if (res != NO_ERROR)
	{
		kfree(bm, 512);
		return res;
	}
	for (;;)
	{
		if ((i & 0x1F) == 0 && end - i >= sizeof(DWORD) * 8 && bm[(i & 0xFFF) >> 5] == 0)	/*分配一个双字描述的簇*/
		{
			bm[(i & 0xFFF) >> 5] = 0xFFFFFFFF;
			i += sizeof(DWORD) * 8;
		}
		else	/*分配一位描述的簇*/
		{
			bm[(i & 0xFFF) >> 5] |= (1lu << (i & 0x1F));
			i++;
		}
		if (i >= end || (bm[(i & 0xFFF) >> 5] & (1lu << (i & 0x1F))))
			break;	/*完成或遇到非空簇*/
		if ((i & 0xFFF) == 0)	/*切换当前索引簇*/
		{
			RwPart(ptd, TRUE, ((ULIFS *)ptd->data)->ubm + ((i - 1) >> 12), 1, bm);
			RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->ubm + (i >> 12), 1, bm);
		}
	}
	RwPart(ptd, TRUE, ((ULIFS *)ptd->data)->ubm + ((i - 1) >> 12), 1, bm);
	(*cou) = i - (*fst);
	((ULIFS *)ptd->data)->RemCu -= (*cou);
	for (;; i += 32)	/*查找空簇*/
	{
		DWORD bit;

		if ((i & 0xFFF) == 0)
			RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->ubm + (i >> 12), 1, bm);	/*读取新扇区*/
		if ((bit = bm[(i & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*有空簇*/
		{
			DWORD j;

			for (j = 0;; j++)
				if (!(bit & (1lu << j)))	/*空簇*/
				{
					((ULIFS *)ptd->data)->FstCu = i + j;	/*取得首空簇号*/
					kfree(bm, 512);
					return NO_ERROR;
				}
		}
	}
}

/*回收簇,不允许cou为0*/
long FreeClu(PART_DESC *ptd, DWORD fst, DWORD cou)
{
	DWORD i, end, *bm;	/*循环,结束位置,位图*/
	long res;

	if ((bm = (DWORD *)kmalloc(512)) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	i = fst;
	end = i + cou;
	res = RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->ubm + (i >> 12), 1, bm);
	if (res != NO_ERROR)
	{
		kfree(bm, 512);
		return res;
	}
	for (;;)
	{
		if ((i & 0x1F) == 0 && end - i >= sizeof(DWORD) * 8)	/*释放一个双字描述的簇*/
		{
			bm[(i & 0xFFF) >> 5] = 0;
			i += sizeof(DWORD) * 8;
		}
		else	/*释放一位描述的簇*/
		{
			bm[(i & 0xFFF) >> 5] &= (~(1lu << (i & 0x1F)));
			i++;
		}
		if (i >= end)
			break;	/*完成*/
		if ((i & 0xFFF) == 0)	/*切换当前索引簇*/
		{
			RwPart(ptd, TRUE, ((ULIFS *)ptd->data)->ubm + ((i - 1) >> 12), 1, bm);
			RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->ubm + (i >> 12), 1, bm);
		}
	}
	RwPart(ptd, TRUE, ((ULIFS *)ptd->data)->ubm + ((i - 1) >> 12), 1, bm);
	((ULIFS *)ptd->data)->RemCu += cou;
	if (fst < ((ULIFS *)ptd->data)->FstCu)
		((ULIFS *)ptd->data)->FstCu = fst;
	kfree(bm, 512);
	return NO_ERROR;
}

/*设置文件长度*/
long SetSizeUlifs(FILE_DESC *fd, QWORD siz)
{
	DWORD curidx = 0, cou;	/*当前索引位置,每节点簇数量*/
	DWORD idxi, bpc;/*索引节点索引,簇字节数*/
	DWORD cun0, cun, clui;/*原簇数量,修改后簇数量,数据簇索引*/
	PART_DESC *ptd;	/*所在分区结构指针*/
	BLKID *ip;	/*索引簇指针*/
	long res;

	ptd = part[fd->PartID];
	bpc = ((ULIFS *)(ptd->data))->spc << 9;
	cun0 = (((ULIFS_DIR *)fd->data)->size + bpc - 1) / bpc;
	cun = (siz + bpc - 1) / bpc;
	if (cun == cun0)	/*不需要修改簇*/
	{
		fd->attr.size = ((ULIFS_DIR *)fd->data)->size = siz;
		return RwFileUlifs(filmt[fd->par], TRUE, (QWORD)fd->avl * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data);
	}
	LOCK(((ULIFS *)(ptd->data))->ubm_l);
	if (cun > cun0 && cun - cun0 > ((ULIFS *)(ptd->data))->RemCu)
	{
		ULOCK(((ULIFS *)(ptd->data))->ubm_l);
		return ERROR_FS_HAVENO_SPACE;	/*超出剩余簇数量*/
	}
	if ((ip = (BLKID *)kmalloc(bpc)) == NULL)
	{
		ULOCK(((ULIFS *)(ptd->data))->ubm_l);
		return ERROR_FS_HAVENO_MEMORY;
	}
	idxi = (bpc >> 3) - 3;
	memcpy32(&ip[idxi], ((ULIFS_DIR *)fd->data)->idx, sizeof(BLKID) * 3 / sizeof(DWORD));	/*复制目录项中的索引*/
	clui = 0;
	if (cun < cun0)	/*减小文件*/
	{
		DWORD preidx;	/*前索引簇位置*/

		for (;;)	/*查找释放位置*/
		{
			for (;;)	/*索引簇*/
			{
				cou = ip[idxi].cou;
				if (cou == 0)
					break;
				clui += cou;
				if (clui > cun)	/*开始释放簇*/
				{
					if (clui - cou < cun)	/*临界节点*/
					{
						res = FreeClu(ptd, ip[idxi].fst + cou + cun - clui, clui - cun);	/*释放当前索引标记的簇,取得释放掉的簇数量*/
						if (res != NO_ERROR)
						{
							kfree(ip, bpc);
							ULOCK(((ULIFS *)(ptd->data))->ubm_l);
							return res;
						}
						ip[idxi].cou -= (clui - cun);
					}
					else
					{
						res = FreeClu(ptd, ip[idxi].fst, cou);
						if (res != NO_ERROR)
						{
							kfree(ip, bpc);
							ULOCK(((ULIFS *)(ptd->data))->ubm_l);
							return res;
						}
						ip[idxi].cou = 0;
					}
					if (clui >= cun0)	/*释放完成*/
						break;
				}
				idxi++;
			}
			if (clui > cun)	/*开始释放簇*/
			{
				if (curidx == 0)	/*没有当前索引簇,修改目录项*/
					memcpy32(((ULIFS_DIR *)fd->data)->idx, &ip[(bpc >> 3) - 3], sizeof(BLKID) * 3 / sizeof(DWORD));
				else if (ip[0].cou == 0)	/*当前索引簇全空,删除当前索引索引簇*/
				{
					res = FreeClu(ptd, curidx, 1);
					if (res != NO_ERROR)
					{
						kfree(ip, bpc);
						ULOCK(((ULIFS *)(ptd->data))->ubm_l);
						return res;
					}
				}
				else if (ip[1].cou == 0)	/*当前索引簇只剩一个节点,节点写入前索引项*/
				{
					res = FreeClu(ptd, curidx, 1);
					if (res != NO_ERROR)
					{
						kfree(ip, bpc);
						ULOCK(((ULIFS *)(ptd->data))->ubm_l);
						return res;
					}
					if (preidx == 0)	/*没有前索引簇*/
						((ULIFS_DIR *)fd->data)->idx[2] = ip[0];
					else
					{
						BLKID tmpbid = ip[0];	/*临时索引结点*/

						res = RwClu(ptd, FALSE, preidx, 1, ip);
						if (res != NO_ERROR)
						{
							kfree(ip, bpc);
							ULOCK(((ULIFS *)(ptd->data))->ubm_l);
							return res;
						}
						ip[(bpc >> 3) - 1] = tmpbid;
						res = RwClu(ptd, TRUE, preidx, 1, ip);
						if (res != NO_ERROR)
						{
							kfree(ip, bpc);
							ULOCK(((ULIFS *)(ptd->data))->ubm_l);
							return res;
						}
					}
				}
				else	/*写入当前索引*/
				{
					RwClu(ptd, TRUE, curidx, 1, ip);
					if (res != NO_ERROR)
					{
						kfree(ip, bpc);
						ULOCK(((ULIFS *)(ptd->data))->ubm_l);
						return res;
					}
				}
				if (clui >= cun0)
				{
					kfree(ip, bpc);
					ULOCK(((ULIFS *)(ptd->data))->ubm_l);
					fd->attr.size = ((ULIFS_DIR *)fd->data)->size = siz;
					return RwFileUlifs(filmt[fd->par], TRUE, (QWORD)fd->avl * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data);
				}
			}
			preidx = curidx;
			curidx = ip[idxi].fst;
			res = RwClu(ptd, FALSE, curidx, 1, ip);	/*读取下一个索引簇*/
			if (res != NO_ERROR)
			{
				kfree(ip, bpc);
				ULOCK(((ULIFS *)(ptd->data))->ubm_l);
				return res;
			}
			idxi = 0;
		}
	}
	else	/*增大文件*/
	{
		for (;;)
		{
			for (;;)	/*索引簇*/
			{
				if (clui >= cun0)	/*到文件尾,开始申请*/
					goto StartAlloc;
				cou = ip[idxi].cou;
				if (cou == 0)
					break;
				clui += cou;
				idxi++;
			}
			curidx = ip[idxi].fst;
			RwClu(ptd, FALSE, curidx, 1, ip);	/*读取下一个索引簇*/
			if (res != NO_ERROR)
			{
				kfree(ip, bpc);
				ULOCK(((ULIFS *)(ptd->data))->ubm_l);
				return res;
			}
			idxi = 0;
		}
StartAlloc:
		cou = cun - cun0;
		idxi--;
		for (;;)
		{
			DWORD tfst, tcou;

			tcou = cou;
			res = AllocClu(ptd, &tfst, &tcou);
			if (res != NO_ERROR)
			{
				kfree(ip, bpc);
				ULOCK(((ULIFS *)(ptd->data))->ubm_l);
				return res;
			}
			cou -= tcou;
			if (tfst == ip[idxi].fst + ip[idxi].cou)	/*在当前簇后面,加入当前簇节点*/
				ip[idxi].cou += tcou;
			else if (++idxi < (bpc >> 3))	/*加入下一个簇节点*/
			{
				ip[idxi].fst = tfst;
				ip[idxi].cou = tcou;
			}
			else	/*加入下一个索引簇*/
			{
				BLKID tmpbid = ip[--idxi];

				clui = 1;	/*clui从此改变用途,成为临时cou*/
				res = AllocClu(ptd, &ip[idxi].fst, &clui);
				if (res != NO_ERROR)
				{
					kfree(ip, bpc);
					ULOCK(((ULIFS *)(ptd->data))->ubm_l);
					return res;
				}
				ip[idxi].cou = 0;
				if (curidx == 0)
					memcpy32(((ULIFS_DIR *)fd->data)->idx, &ip[(bpc >> 3) - 3], sizeof(BLKID) * 3 / sizeof(DWORD));
				else
				{
					res = RwClu(ptd, TRUE, curidx, 1, ip);
					if (res != NO_ERROR)
					{
						kfree(ip, bpc);
						ULOCK(((ULIFS *)(ptd->data))->ubm_l);
						return res;
					}
				}
				curidx = ip[idxi].fst;
				ip[0] = tmpbid;
				ip[1].fst = tfst;
				ip[1].cou = tcou;
				idxi = 1;
			}
			if (cou == 0)	/*申请完成*/
			{
				if (curidx == 0)
					memcpy32(((ULIFS_DIR *)fd->data)->idx, &ip[(bpc >> 3) - 3], sizeof(BLKID) * 3 / sizeof(DWORD));
				else
				{
					RwClu(ptd, TRUE, curidx, 1, ip);
					if (res != NO_ERROR)
					{
						kfree(ip, bpc);
						ULOCK(((ULIFS *)(ptd->data))->ubm_l);
						return res;
					}
				}
				fd->attr.size = ((ULIFS_DIR *)fd->data)->size = siz;
				return RwFileUlifs(filmt[fd->par], TRUE, (QWORD)fd->avl * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data);
			}
		}
	}
}

/*比较路径字符串与文件名是否匹配*/
BOOL CmpFileUlifs(FILE_DESC *fd, BYTE *path)
{
	BYTE *namep = ((ULIFS_DIR *)fd->data)->name;

	while (*namep)
		if (*namep++ != *path++)
			return FALSE;	/*文件名不匹配*/
	if (*path != '/' && *path)
		return FALSE;
	return TRUE;
}

/*data信息拷贝到attr*/
void DataToAttr(FILE_ATTR *fa, ULIFS_DIR *ud)
{
	strcpy(fa->name, ud->name);
	fa->CreateTime = ud->CreateTime;
	fa->ModifyTime = ud->ModifyTime;
	fa->AccessTime = ud->AccessTime;
	fa->attr = ud->attr;
	fa->size = ud->size;
}

/*attr信息拷贝到data*/
void AttrToData(ULIFS_DIR *ud, FILE_ATTR *fa)
{
	strcpy(ud->name, fa->name);
	ud->CreateTime = fa->CreateTime;
	ud->ModifyTime = fa->ModifyTime;
	ud->AccessTime = fa->AccessTime;
	ud->attr = fa->attr;
	ud->size = fa->size;
}

/*搜索并设置文件项*/
long SchFileUlifs(FILE_DESC *fd, BYTE *path)
{
	ULIFS_DIR *ud;
	long res;

	if ((ud = (void *)kmalloc(sizeof(ULIFS_DIR))) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	if (fd->par == 0)	/*取得根目录项*/
	{
		ULIFS_DIR *tmpdir;

		if ((tmpdir = (ULIFS_DIR *)kmalloc(512)) == NULL)
		{
			kfree(ud, sizeof(ULIFS_DIR));
			return ERROR_FS_HAVENO_MEMORY;
		}
		res = RwPart(part[fd->PartID], FALSE, ((ULIFS *)part[fd->PartID]->data)->CluID, 1, tmpdir);	/*读取数据簇中的第一个扇区*/
		if (res != NO_ERROR)
		{
			kfree(tmpdir, 512);
			kfree(ud, sizeof(ULIFS_DIR));
			return res;
		}
		*ud = *tmpdir;
		kfree(tmpdir, 512);
		DataToAttr(&fd->attr, ud);
		fd->avl = 0;
		fd->data = ud;
		return NO_ERROR;
	}
	else	/*取得其他目录项*/
	{
		FILE_DESC *ParFd;
		QWORD seek, siz;

		ParFd = filmt[fd->par];
		siz = ((ULIFS_DIR *)ParFd->data)->size;
		for (seek = 0; seek < siz; seek += sizeof(ULIFS_DIR))
		{
			res = RwFileUlifs(ParFd, FALSE, seek, sizeof(ULIFS_DIR), (BYTE *)ud);
			if (res != NO_ERROR)
			{
				kfree(ud, sizeof(ULIFS_DIR));
				return res;
			}
			if (CmpFileUlifs(fd, path))
			{
				DataToAttr(&fd->attr, ud);
				fd->avl = seek / sizeof(ULIFS_DIR);
				fd->data = ud;
				return NO_ERROR;
			}
		}
		kfree(ud, sizeof(ULIFS_DIR));
		return ERROR_FS_HAVENO_FILE;
	}
}

/*创建并设置文件项*/
long NewFileUlifs(FILE_DESC *fd, BYTE *path)
{
	QWORD seek, siz;
	FILE_DESC *ParFd;
	ULIFS_DIR *ud;
	BYTE *pathp;
	long res;

	for (pathp = path; *pathp; pathp++)	/*文件名超长或含有非法字符*/
		if (pathp - path >= ULIFS_FILE_NAME_SIZE || *pathp == '/')
			return ERROR_FS_PATH_FORMAT;
	res = SchFileUlifs(fd, path);
	if (res == NO_ERROR)	/*文件项已经存在了*/
	{
		kfree(fd->data, sizeof(ULIFS_DIR));	/*释放SchFileUlifs申请的数据*/
		fd->data = NULL;
		return ERROR_FS_FILE_EXIST;
	}
	if (res != ERROR_FS_HAVENO_FILE)	/*出错了*/
		return res;
	if ((ud = (void *)kmalloc(sizeof(ULIFS_DIR))) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	/*搜索空位*/
	ParFd = filmt[fd->par];
	siz = ((ULIFS_DIR *)ParFd->data)->size;
	for (seek = 0; seek < siz; seek += sizeof(ULIFS_DIR))
	{
		res = RwFileUlifs(ParFd, FALSE, seek, sizeof(ULIFS_DIR), (BYTE *)ud);
		if (res != NO_ERROR)
		{
			kfree(ud, sizeof(ULIFS_DIR));
			return res;
		}
		if (ud->name[0] == 0)
			goto CreateDir;
	}
	if (seek >= sizeof(ULIFS_DIR) * ULIFS_MAX_DIR)
		return ERROR_FS_FILE_LIMIT;
	res = SetSizeUlifs(ParFd, seek + sizeof(ULIFS_DIR));
	if (res != NO_ERROR)
	{
		kfree(ud, sizeof(ULIFS_DIR));
		return res;
	}
CreateDir:	/*创建*/
	strcpy(fd->attr.name, path);
	AttrToData(ud, &fd->attr);
	res = RwFileUlifs(ParFd, TRUE, seek, sizeof(ULIFS_DIR), fd->data);
	if (res != NO_ERROR)
	{
		kfree(ud, sizeof(ULIFS_DIR));
		return res;
	}
	fd->avl = seek / sizeof(ULIFS_DIR);
	fd->data = ud;
	return NO_ERROR;
}

/*删除文件项*/
long DelFileUlifs(FILE_DESC *fd)
{
	QWORD seek, siz;
	FILE_DESC *ParFd;
	long res;

	seek = (QWORD)fd->avl * sizeof(ULIFS_DIR);
	ParFd = filmt[fd->par];
	siz = ((ULIFS_DIR *)ParFd->data)->size;
	if (seek + sizeof(ULIFS_DIR) == siz)	/*已是最后一项*/
	{
		if (seek == 0)	/*只有一项*/
		{
			res = SetSizeUlifs(ParFd, 0);
			if (res != NO_ERROR)
				return res;
		}
		else
		{
			do
			{
				res = RwFileUlifs(ParFd, FALSE, seek -= sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data);
				if (res != NO_ERROR)
					return res;
			}
			while (((ULIFS_DIR *)fd->data)->name[0] == 0);
			res = SetSizeUlifs(ParFd, seek + sizeof(ULIFS_DIR));
			if (res != NO_ERROR)
				return res;
		}
	}
	else	/*直接标记为删除*/
	{
		((ULIFS_DIR *)fd->data)->name[0] = 0;
		res = RwFileUlifs(ParFd, TRUE, seek, sizeof(ULIFS_DIR), fd->data);
		if (res != NO_ERROR)
			return res;
	}
	kfree(fd->data, sizeof(ULIFS_DIR));
	fd->data = NULL;
	return NO_ERROR;
}

/*设置文件项信息*/
long SetFileUlifs(FILE_DESC *fd, FILE_ATTR *fa)
{
	if (fd->par)
	{
		if (fa->name[0])
			strcpy(fd->attr.name, fa->name);
	}
	else
		strcpy(&fd->attr.name[1], part[fd->PartID]->attr.label);
	if (fa->CreateTime != INVALID)
		fd->attr.CreateTime = fa->CreateTime;
	if (fa->ModifyTime != INVALID)
		fd->attr.ModifyTime = fa->ModifyTime;
	if (fa->AccessTime != INVALID)
		fd->attr.AccessTime = fa->AccessTime;
	if (fa->attr != INVALID)
		fd->attr.attr = fa->attr;
	AttrToData(fd->data, &fd->attr);
	return RwFileUlifs(filmt[fd->par], TRUE, (QWORD)fd->avl * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data);
}

/*获取目录列表*/
long ReadDirUlifs(FILE_DESC *fd, QWORD *seek, FILE_ATTR *fa)
{
	long res;

	while ((*seek) < ((ULIFS_DIR *)fd->data)->size)
	{
		ULIFS_DIR tmpdir;

		res = RwFileUlifs(fd, FALSE, (*seek), sizeof(ULIFS_DIR), (BYTE *)&tmpdir);
		if (res != NO_ERROR)
			return res;
		(*seek) += sizeof(ULIFS_DIR);
		if (tmpdir.name[0] && tmpdir.name[0] != '/')	/*有效的文件名,除去根目录*/
		{
			DataToAttr(fa, &tmpdir);
			return NO_ERROR;
		}
	}
	(*seek) = 0;
	return NO_ERROR;
}

/*释放文件描述符中的私有数据*/
void FreeDataUlifs(FILE_DESC *fd)
{
	kfree(fd->data, sizeof(ULIFS_DIR));
	fd->data = NULL;
}

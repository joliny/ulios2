/*	ulifs.c for ulios
	���ߣ�����
	���ܣ�ULIFS�ļ�ϵͳʵ��
	����޸����ڣ�2009-06-06
*/

#include "../include/kernel.h"
#include "../include/error.h"

/*ulios�����ṹ(��λ:����):
|����|����|����|ʹ��|����|ʣ��
|��¼|����|λͼ|λͼ|����|����
*/
typedef struct
{
	DWORD	INS_JMP;/*��תָ��*/
	BYTE	OEM[8];	/*OEM ID(tian&uli2k_X)*/
	DWORD	fsid;	/*�ļ�ϵͳ��־"ULTN"�ĸ��ַ�,�ļ�ϵͳ����:0x7C*/
	WORD	ver;	/*�汾��0,����Ϊ0�汾��BPB*/
	WORD	bps;	/*ÿ�����ֽ���*/
	WORD	spc;	/*ÿ��������*/
	WORD	res;	/*����������,����������¼*/
	DWORD	secoff;	/*�����ڴ����ϵ���ʼ����ƫ��*/
	DWORD	seccou;	/*����ռ��������,����ʣ������*/
	WORD	spbm;	/*ÿ��λͼռ������*/
	WORD	cluoff;	/*�������״ؿ�ʼ����ƫ��*/
	DWORD	clucou;	/*���ݴ���Ŀ*/
	BYTE	knlpath[88];/*�ں�·��*/
	BYTE	code[382];	/*��������*/
	WORD	aa55;	/*������־*/
}ULIFS_BPB;	/*ulifs BIOS Parameter Block*/

typedef struct
{
	DWORD spc;	/*ÿ��������*/
	DWORD res;	/*����������(λͼ֮ǰ������)*/
	DWORD ubm;	/*ʹ��λͼ��ʼ����*/
	DWORD spbm;	/*ÿ��λͼռ������*/
	DWORD CluID;/*�״�������*/
	DWORD CuCou;/*������*/
	DWORD FstCu;/*��һ���մغ�*/
	DWORD RemCu;/*ʣ�������*/
	DWORD ubm_l;/*ʹ��λͼ��*/
}ULIFS;	/*����ʱulifs��Ϣ�ṹ*/

/*ulifs�ļ�ϵͳ�ĺ��Ľṹ:�������ڵ�(�ǳ���^_^)
fst�Ǳ�ָ��������ؿ���״غ�,��0��ʼ����,
cou�Ǳ��������������ݴصĸ���,������ֵ������0,
��0��ʾ����ڵ�ָ����һ�������ڵ��,���������ݴ�*/
typedef struct
{
	DWORD fst;	/*�״�*/
	DWORD cou;	/*����*/
}BLKID;	/*�������ڵ�*/

/*ulifs�ļ�ϵͳĿ¼��ṹ(��λ:�ֽ�):
ע:Ŀ¼��û��.��..Ŀ¼��
��Ŀ¼0��Ŀ¼��Ϊ��Ŀ¼��Ŀ¼��,����Ϊ�������,��'/'��ͷ,��������'/'*/
#define ULIFS_FILE_NAME_SIZE	80
#define ULIFS_FILE_ATTR_RDONLY	0x00000001	/*ֻ��*/
#define ULIFS_FILE_ATTR_HIDDEN	0x00000002	/*����*/
#define ULIFS_FILE_ATTR_SYSTEM	0x00000004	/*ϵͳ*/
#define ULIFS_FILE_ATTR_LABEL	0x00000008	/*���*/
#define ULIFS_FILE_ATTR_DIREC	0x00000010	/*Ŀ¼(ֻ��)*/
#define ULIFS_FILE_ATTR_ARCH	0x00000020	/*�鵵*/
#define ULIFS_FILE_ATTR_EXEC	0x00000040	/*��ִ��*/
#define ULIFS_FILE_ATTR_UNMDFY	0x80000000	/*���Բ����޸�*/

typedef struct	/*Ŀ¼��ṹ*/
{
	BYTE name[ULIFS_FILE_NAME_SIZE];/*�ļ���*/
	DWORD CreateTime;	/*����ʱ��1970-01-01����������*/
	DWORD ModifyTime;	/*�޸�ʱ��*/
	DWORD AccessTime;	/*����ʱ��*/
	DWORD attr;			/*����*/
	QWORD size;			/*�ļ��ֽ���,Ŀ¼�ļ����ֽ�����Ч*/
	BLKID idx[3];		/*�����3���Դ���������,����һ���ļ��п����ò���������*/
}ULIFS_DIR;	/*Ŀ¼��ṹ*/

#define ULIFS_MAX_DIR	0x10000		/*Ŀ¼��Ŀ¼����������*/

/*��д���������ĺ�*/
#define RwPart(ptd, isWrite, sec, cou, buf) \
RwBuf((ptd)->DevID, (isWrite), (sec), (cou), (buf))

/*��д�صĺ�(������,��д,�غ�,����,������)*/
#define RwClu(ptd, isWrite, clu, cou, buf) \
RwBuf((ptd)->DevID, (isWrite), ((ULIFS*)((ptd)->data))->CluID + ((ULIFS*)((ptd)->data))->spc * (clu), ((ULIFS*)((ptd)->data))->spc * (cou), (buf))

/*����ULIFS����*/
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
	res = RwPart(CurPd, FALSE, CurPd->SecID, 1, buf);	/*����������*/
	if (res != NO_ERROR)
	{
		kfree(buf, 512);
		kfree(fs, sizeof(ULIFS));
		return res;
	}
	if (((ULIFS_BPB*)buf)->aa55 != 0xAA55 || ((ULIFS_BPB*)buf)->fsid != 0x4E544C55 || ((ULIFS_BPB*)buf)->bps != 512)	/*���������־�ͷ�����־,�ݲ�֧�ֲ���512�ֽڵ�����*/
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
	for (i = 0;; i += sizeof(DWORD) * 8)	/*���ҿմ�*/
	{
		DWORD bit;

		if ((i & 0xFFF) == 0)
		{
			res = RwPart(CurPd, FALSE, fs->ubm + (i >> 12), 1, buf);	/*��ȡʹ��λͼ����*/
			if (res != NO_ERROR)
			{
				kfree(buf, 512);
				kfree(fs, sizeof(ULIFS));
				return res;
			}
		}
		if ((bit = ((DWORD *)buf)[(i & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*�пմ�*/
		{
			DWORD j;

			for (j = 0; j < sizeof(DWORD) * 8; j++)
				if (!(bit & (1lu << j)))	/*�մ�*/
				{
					if (i + j >= fs->CuCou)
					{
						RwPart(CurPd, FALSE, fs->CluID, 1, buf);	/*ȡ�þ��*/
						strcpy(CurPd->attr.label, &buf[1]);
						kfree(buf, 512);
						CurPd->attr.size = fs->CuCou * fs->spc * 512;
						CurPd->attr.remain = fs->RemCu * fs->spc * 512;
						CurPd->data = fs;
						return NO_ERROR;
					}
					if (fs->FstCu == 0)
						fs->FstCu = i + j;	/*ȡ���׿մغ�*/
					fs->RemCu++;	/*�ۼƿմ���*/
				}
		}
	}
}

/*ж��ULIFS����*/
void UmntUlifs(PART_DESC *ptd)
{
	kfree(ptd->data, sizeof(ULIFS));
	ptd->data = NULL;
}

/*���÷�����Ϣ*/
long SetPartUlifs(PART_DESC *ptd, PART_ATTR *pa)
{
	ULIFS_DIR *tmpdir;
	long res;

	if ((tmpdir = (ULIFS_DIR *)kmalloc(512)) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	res = RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->CluID, 1, tmpdir);	/*��ȡ���ݴ��еĵ�һ������*/
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

/*��ʽ������*/
long FmtPartUlifs(PART_DESC *ptd)
{
}

/*��д�ļ�,���ݲ��������ļ�β*/
long RwFileUlifs(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, BYTE *buf)
{
	DWORD idxi, bpc;/*�����ڵ�����,���ֽ���*/
	QWORD dati, end;/*��ǰ�����ֽ�λ��,�����ֽڽ�βλ��*/
	DWORD fst, cou;	/*��ʱ�����ڵ�*/
	PART_DESC *ptd;	/*���ڷ����ṹָ��*/
	BLKID *ip;	/*������ָ��*/
	BYTE *dp;	/*���ݴ�ָ��*/
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
	memcpy32(&ip[idxi], ((ULIFS_DIR *)fd->data)->idx, sizeof(BLKID) * 3 / sizeof(DWORD));	/*����Ŀ¼���е�����*/
	dati = 0;
	end = seek + siz;
	for (;;)
	{
		for (;;)	/*������*/
		{
			cou = ip[idxi].cou;
			if (cou == 0)
				break;
			fst = ip[idxi].fst;
			cou += fst;
			while (fst < cou)	/*�����ڵ�*/
			{
				if (dati >= seek)	/*��ͷ�Ժ�*/
				{
					if (dati + bpc > end)	/*ĩβ,���*/
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
					else	/*�м�*/
					{
						DWORD cou1 = cou - fst;	/*��������*/
						DWORD cou2 = (end - dati) / bpc;	/*ʣ���������*/
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
				else if (dati + bpc > seek)	/*��Ҫ��ͷ*/
				{
					res = RwClu(ptd, FALSE, fst, 1, dp);
					if (res != NO_ERROR)
					{
						kfree(dp, bpc);
						kfree(ip, bpc);
						return res;
					}
					if (dati + bpc > end)	/*��ͷ��ĩβ,���*/
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
					else	/*��ͷ*/
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
		res = RwClu(ptd, FALSE, ip[idxi].fst, 1, ip);	/*��ȡ��һ��������*/
		if (res != NO_ERROR)
		{
			kfree(dp, bpc);
			kfree(ip, bpc);
			return res;
		}
		idxi = 0;
	}
}

/*�����,������*couΪ0,��������ʣ��ռ�,����ʱ*couΪʵ���������*/
long AllocClu(PART_DESC *ptd, DWORD *fst, DWORD *cou)
{
	DWORD i, end, *bm;	/*ѭ��,����λ��,λͼ*/
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
		if ((i & 0x1F) == 0 && end - i >= sizeof(DWORD) * 8 && bm[(i & 0xFFF) >> 5] == 0)	/*����һ��˫�������Ĵ�*/
		{
			bm[(i & 0xFFF) >> 5] = 0xFFFFFFFF;
			i += sizeof(DWORD) * 8;
		}
		else	/*����һλ�����Ĵ�*/
		{
			bm[(i & 0xFFF) >> 5] |= (1lu << (i & 0x1F));
			i++;
		}
		if (i >= end || (bm[(i & 0xFFF) >> 5] & (1lu << (i & 0x1F))))
			break;	/*��ɻ������ǿմ�*/
		if ((i & 0xFFF) == 0)	/*�л���ǰ������*/
		{
			RwPart(ptd, TRUE, ((ULIFS *)ptd->data)->ubm + ((i - 1) >> 12), 1, bm);
			RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->ubm + (i >> 12), 1, bm);
		}
	}
	RwPart(ptd, TRUE, ((ULIFS *)ptd->data)->ubm + ((i - 1) >> 12), 1, bm);
	(*cou) = i - (*fst);
	((ULIFS *)ptd->data)->RemCu -= (*cou);
	for (;; i += 32)	/*���ҿմ�*/
	{
		DWORD bit;

		if ((i & 0xFFF) == 0)
			RwPart(ptd, FALSE, ((ULIFS *)ptd->data)->ubm + (i >> 12), 1, bm);	/*��ȡ������*/
		if ((bit = bm[(i & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*�пմ�*/
		{
			DWORD j;

			for (j = 0;; j++)
				if (!(bit & (1lu << j)))	/*�մ�*/
				{
					((ULIFS *)ptd->data)->FstCu = i + j;	/*ȡ���׿մغ�*/
					kfree(bm, 512);
					return NO_ERROR;
				}
		}
	}
}

/*���մ�,������couΪ0*/
long FreeClu(PART_DESC *ptd, DWORD fst, DWORD cou)
{
	DWORD i, end, *bm;	/*ѭ��,����λ��,λͼ*/
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
		if ((i & 0x1F) == 0 && end - i >= sizeof(DWORD) * 8)	/*�ͷ�һ��˫�������Ĵ�*/
		{
			bm[(i & 0xFFF) >> 5] = 0;
			i += sizeof(DWORD) * 8;
		}
		else	/*�ͷ�һλ�����Ĵ�*/
		{
			bm[(i & 0xFFF) >> 5] &= (~(1lu << (i & 0x1F)));
			i++;
		}
		if (i >= end)
			break;	/*���*/
		if ((i & 0xFFF) == 0)	/*�л���ǰ������*/
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

/*�����ļ�����*/
long SetSizeUlifs(FILE_DESC *fd, QWORD siz)
{
	DWORD curidx = 0, cou;	/*��ǰ����λ��,ÿ�ڵ������*/
	DWORD idxi, bpc;/*�����ڵ�����,���ֽ���*/
	DWORD cun0, cun, clui;/*ԭ������,�޸ĺ������,���ݴ�����*/
	PART_DESC *ptd;	/*���ڷ����ṹָ��*/
	BLKID *ip;	/*������ָ��*/
	long res;

	ptd = part[fd->PartID];
	bpc = ((ULIFS *)(ptd->data))->spc << 9;
	cun0 = (((ULIFS_DIR *)fd->data)->size + bpc - 1) / bpc;
	cun = (siz + bpc - 1) / bpc;
	if (cun == cun0)	/*����Ҫ�޸Ĵ�*/
	{
		fd->attr.size = ((ULIFS_DIR *)fd->data)->size = siz;
		return RwFileUlifs(filmt[fd->par], TRUE, (QWORD)fd->avl * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data);
	}
	LOCK(((ULIFS *)(ptd->data))->ubm_l);
	if (cun > cun0 && cun - cun0 > ((ULIFS *)(ptd->data))->RemCu)
	{
		ULOCK(((ULIFS *)(ptd->data))->ubm_l);
		return ERROR_FS_HAVENO_SPACE;	/*����ʣ�������*/
	}
	if ((ip = (BLKID *)kmalloc(bpc)) == NULL)
	{
		ULOCK(((ULIFS *)(ptd->data))->ubm_l);
		return ERROR_FS_HAVENO_MEMORY;
	}
	idxi = (bpc >> 3) - 3;
	memcpy32(&ip[idxi], ((ULIFS_DIR *)fd->data)->idx, sizeof(BLKID) * 3 / sizeof(DWORD));	/*����Ŀ¼���е�����*/
	clui = 0;
	if (cun < cun0)	/*��С�ļ�*/
	{
		DWORD preidx;	/*ǰ������λ��*/

		for (;;)	/*�����ͷ�λ��*/
		{
			for (;;)	/*������*/
			{
				cou = ip[idxi].cou;
				if (cou == 0)
					break;
				clui += cou;
				if (clui > cun)	/*��ʼ�ͷŴ�*/
				{
					if (clui - cou < cun)	/*�ٽ�ڵ�*/
					{
						res = FreeClu(ptd, ip[idxi].fst + cou + cun - clui, clui - cun);	/*�ͷŵ�ǰ������ǵĴ�,ȡ���ͷŵ��Ĵ�����*/
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
					if (clui >= cun0)	/*�ͷ����*/
						break;
				}
				idxi++;
			}
			if (clui > cun)	/*��ʼ�ͷŴ�*/
			{
				if (curidx == 0)	/*û�е�ǰ������,�޸�Ŀ¼��*/
					memcpy32(((ULIFS_DIR *)fd->data)->idx, &ip[(bpc >> 3) - 3], sizeof(BLKID) * 3 / sizeof(DWORD));
				else if (ip[0].cou == 0)	/*��ǰ������ȫ��,ɾ����ǰ����������*/
				{
					res = FreeClu(ptd, curidx, 1);
					if (res != NO_ERROR)
					{
						kfree(ip, bpc);
						ULOCK(((ULIFS *)(ptd->data))->ubm_l);
						return res;
					}
				}
				else if (ip[1].cou == 0)	/*��ǰ������ֻʣһ���ڵ�,�ڵ�д��ǰ������*/
				{
					res = FreeClu(ptd, curidx, 1);
					if (res != NO_ERROR)
					{
						kfree(ip, bpc);
						ULOCK(((ULIFS *)(ptd->data))->ubm_l);
						return res;
					}
					if (preidx == 0)	/*û��ǰ������*/
						((ULIFS_DIR *)fd->data)->idx[2] = ip[0];
					else
					{
						BLKID tmpbid = ip[0];	/*��ʱ�������*/

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
				else	/*д�뵱ǰ����*/
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
			res = RwClu(ptd, FALSE, curidx, 1, ip);	/*��ȡ��һ��������*/
			if (res != NO_ERROR)
			{
				kfree(ip, bpc);
				ULOCK(((ULIFS *)(ptd->data))->ubm_l);
				return res;
			}
			idxi = 0;
		}
	}
	else	/*�����ļ�*/
	{
		for (;;)
		{
			for (;;)	/*������*/
			{
				if (clui >= cun0)	/*���ļ�β,��ʼ����*/
					goto StartAlloc;
				cou = ip[idxi].cou;
				if (cou == 0)
					break;
				clui += cou;
				idxi++;
			}
			curidx = ip[idxi].fst;
			RwClu(ptd, FALSE, curidx, 1, ip);	/*��ȡ��һ��������*/
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
			if (tfst == ip[idxi].fst + ip[idxi].cou)	/*�ڵ�ǰ�غ���,���뵱ǰ�ؽڵ�*/
				ip[idxi].cou += tcou;
			else if (++idxi < (bpc >> 3))	/*������һ���ؽڵ�*/
			{
				ip[idxi].fst = tfst;
				ip[idxi].cou = tcou;
			}
			else	/*������һ��������*/
			{
				BLKID tmpbid = ip[--idxi];

				clui = 1;	/*clui�Ӵ˸ı���;,��Ϊ��ʱcou*/
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
			if (cou == 0)	/*�������*/
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

/*�Ƚ�·���ַ������ļ����Ƿ�ƥ��*/
BOOL CmpFileUlifs(FILE_DESC *fd, BYTE *path)
{
	BYTE *namep = ((ULIFS_DIR *)fd->data)->name;

	while (*namep)
		if (*namep++ != *path++)
			return FALSE;	/*�ļ�����ƥ��*/
	if (*path != '/' && *path)
		return FALSE;
	return TRUE;
}

/*data��Ϣ������attr*/
void DataToAttr(FILE_ATTR *fa, ULIFS_DIR *ud)
{
	strcpy(fa->name, ud->name);
	fa->CreateTime = ud->CreateTime;
	fa->ModifyTime = ud->ModifyTime;
	fa->AccessTime = ud->AccessTime;
	fa->attr = ud->attr;
	fa->size = ud->size;
}

/*attr��Ϣ������data*/
void AttrToData(ULIFS_DIR *ud, FILE_ATTR *fa)
{
	strcpy(ud->name, fa->name);
	ud->CreateTime = fa->CreateTime;
	ud->ModifyTime = fa->ModifyTime;
	ud->AccessTime = fa->AccessTime;
	ud->attr = fa->attr;
	ud->size = fa->size;
}

/*�����������ļ���*/
long SchFileUlifs(FILE_DESC *fd, BYTE *path)
{
	ULIFS_DIR *ud;
	long res;

	if ((ud = (void *)kmalloc(sizeof(ULIFS_DIR))) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	if (fd->par == 0)	/*ȡ�ø�Ŀ¼��*/
	{
		ULIFS_DIR *tmpdir;

		if ((tmpdir = (ULIFS_DIR *)kmalloc(512)) == NULL)
		{
			kfree(ud, sizeof(ULIFS_DIR));
			return ERROR_FS_HAVENO_MEMORY;
		}
		res = RwPart(part[fd->PartID], FALSE, ((ULIFS *)part[fd->PartID]->data)->CluID, 1, tmpdir);	/*��ȡ���ݴ��еĵ�һ������*/
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
	else	/*ȡ������Ŀ¼��*/
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

/*�����������ļ���*/
long NewFileUlifs(FILE_DESC *fd, BYTE *path)
{
	QWORD seek, siz;
	FILE_DESC *ParFd;
	ULIFS_DIR *ud;
	BYTE *pathp;
	long res;

	for (pathp = path; *pathp; pathp++)	/*�ļ����������зǷ��ַ�*/
		if (pathp - path >= ULIFS_FILE_NAME_SIZE || *pathp == '/')
			return ERROR_FS_PATH_FORMAT;
	res = SchFileUlifs(fd, path);
	if (res == NO_ERROR)	/*�ļ����Ѿ�������*/
	{
		kfree(fd->data, sizeof(ULIFS_DIR));	/*�ͷ�SchFileUlifs���������*/
		fd->data = NULL;
		return ERROR_FS_FILE_EXIST;
	}
	if (res != ERROR_FS_HAVENO_FILE)	/*������*/
		return res;
	if ((ud = (void *)kmalloc(sizeof(ULIFS_DIR))) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	/*������λ*/
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
CreateDir:	/*����*/
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

/*ɾ���ļ���*/
long DelFileUlifs(FILE_DESC *fd)
{
	QWORD seek, siz;
	FILE_DESC *ParFd;
	long res;

	seek = (QWORD)fd->avl * sizeof(ULIFS_DIR);
	ParFd = filmt[fd->par];
	siz = ((ULIFS_DIR *)ParFd->data)->size;
	if (seek + sizeof(ULIFS_DIR) == siz)	/*�������һ��*/
	{
		if (seek == 0)	/*ֻ��һ��*/
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
	else	/*ֱ�ӱ��Ϊɾ��*/
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

/*�����ļ�����Ϣ*/
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

/*��ȡĿ¼�б�*/
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
		if (tmpdir.name[0] && tmpdir.name[0] != '/')	/*��Ч���ļ���,��ȥ��Ŀ¼*/
		{
			DataToAttr(fa, &tmpdir);
			return NO_ERROR;
		}
	}
	(*seek) = 0;
	return NO_ERROR;
}

/*�ͷ��ļ��������е�˽������*/
void FreeDataUlifs(FILE_DESC *fd)
{
	kfree(fd->data, sizeof(ULIFS_DIR));
	fd->data = NULL;
}

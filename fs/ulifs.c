/*	ulifs.c for ulios file system
	���ߣ�����
	���ܣ�ULIFS�ļ�ϵͳʵ��
	����޸����ڣ�2009-06-06
*/

#include "fs.h"

/*ulios�����ṹ:
|����|����|����|ʣ��
|��¼|����|����|����
*/
typedef struct _ULIFS_BPB
{
	DWORD	INS_JMP;/*��תָ��*/
	BYTE	OEM[8];	/*OEM ID(tian&uli2k_X)*/
	DWORD	fsid;	/*�ļ�ϵͳ��־"ULTN",�ļ�ϵͳ����:0x7C*/
	WORD	ver;	/*�汾��0,����Ϊ0�汾��BPB*/
	WORD	bps;	/*ÿ�����ֽ���*/
	WORD	spc;	/*ÿ��������*/
	WORD	res;	/*����������,����������¼*/
	DWORD	secoff;	/*�����ڴ����ϵ���ʼ����ƫ��*/
	DWORD	seccou;	/*����ռ��������,����ʣ������*/
	WORD	spbm;	/*ÿ��λͼռ������*/
	WORD	cluoff;	/*�������״ؿ�ʼ����ƫ��*/
	DWORD	clucou;	/*���ݴ���Ŀ*/
	char	BootPath[88];/*�����б��ļ�·��*/
	BYTE	code[382];	/*��������*/
	WORD	aa55;	/*������־*/
}ULIFS_BPB;	/*ulifs BIOS Parameter Block*/

typedef struct _ULIFS
{
	DWORD	spc;	/*ÿ��������*/
	DWORD	res;	/*����������(λͼ֮ǰ������)*/
	DWORD	ubm;	/*ʹ��λͼ��ʼ����*/
	DWORD	spbm;	/*ÿ��λͼռ������*/
	DWORD	CluID;	/*�״�������*/
	DWORD	CuCou;	/*������*/
	DWORD	FstCu;	/*��һ���մغ�*/
	DWORD	RemCu;	/*ʣ�������*/
	DWORD	ubml;	/*ʹ��λͼ��*/
}ULIFS;	/*����ʱulifs��Ϣ�ṹ*/

/*ulifs�ļ�ϵͳ�ĺ��Ľṹ:�������ڵ�(�ǳ���^_^)
fst�Ǳ�ָ��������ؿ���״غ�,��0��ʼ����,
cou�Ǳ��������������ݴصĸ���,������ֵ������0,
��0��ʾ����ڵ�ָ����һ�������ڵ��,���������ݴ�*/
typedef struct _BLKID
{
	DWORD	fst;	/*�״�*/
	DWORD	cou;	/*����*/
}BLKID;	/*�������ڵ�*/

/*ulifs�ļ�ϵͳĿ¼��ṹ:
ע:Ŀ¼��û��.��..Ŀ¼��
��Ŀ¼0��Ŀ¼��Ϊ��Ŀ¼��Ŀ¼��,����Ϊ�������,��'/'��ͷ,��������'/'*/
#define ULIFS_FILE_NAME_SIZE	80
#define ULIFS_FILE_ATTR_RDONLY	0x00000001	/*ֻ��*/
#define ULIFS_FILE_ATTR_HIDDEN	0x00000002	/*����*/
#define ULIFS_FILE_ATTR_SYSTEM	0x00000004	/*ϵͳ*/
#define ULIFS_FILE_ATTR_LABEL	0x00000008	/*���(ֻ��)*/
#define ULIFS_FILE_ATTR_DIREC	0x00000010	/*Ŀ¼(ֻ��)*/
#define ULIFS_FILE_ATTR_ARCH	0x00000020	/*�鵵*/
#define ULIFS_FILE_ATTR_EXEC	0x00000040	/*��ִ��*/
#define ULIFS_FILE_ATTR_DIRTY	0x40000000	/*���ݲ�һ��*/
#define ULIFS_FILE_ATTR_UNMDFY	0x80000000	/*���Բ����޸�*/
#define ULIFS_FILE_IDX_LEN		3

typedef struct _ULIFS_DIR	/*Ŀ¼��ṹ*/
{
	char name[ULIFS_FILE_NAME_SIZE];/*�ļ���*/
	DWORD CreateTime;	/*����ʱ��1970-01-01����������*/
	DWORD ModifyTime;	/*�޸�ʱ��*/
	DWORD AccessTime;	/*����ʱ��*/
	DWORD attr;			/*����*/
	QWORD size;			/*�ļ��ֽ���,Ŀ¼�ļ����ֽ�����Ч*/
	BLKID idx[ULIFS_FILE_IDX_LEN];	/*�Դ�������*/
}ULIFS_DIR;	/*Ŀ¼��ṹ*/

#define ULIFS_ID		0x7C		/*ULIFS�ļ�ϵͳID*/
#define ULIFS_FLAG		0x4E544C55	/*ULTN�ļ�ϵͳ��ʶ*/
#define ULIFS_BPS		0x200		/*�����ÿ�����ֽ���*/
#define ULIFS_MAX_SPC	0x80		/*ÿ������������*/
#define ULIFS_MAX_DIR	0x10000		/*Ŀ¼��Ŀ¼����������*/

static inline long RwClu(PART_DESC *part, BOOL isWrite, DWORD clu, DWORD cou, void *buf)	/*��д������*/
{
	ULIFS *data = (ULIFS*)part->data;
	return RwPart(part, isWrite, data->CluID + data->spc * clu, data->spc * cou, buf);
}

/*����ULIFS����*/
long UlifsMntPart(PART_DESC *pd)
{
	ULIFS *fs;
	DWORD i;
	long res;
	BYTE buf[ULIFS_BPS];

	if (pd->FsID != 0x7C)
		return FS_ERR_WRONG_ARGS;
	if ((res = RwPart(pd, FALSE, 0, 1, buf)) != NO_ERROR)	/*����������*/
		return res;
	if (((ULIFS_BPB*)buf)->aa55 != 0xAA55 || ((ULIFS_BPB*)buf)->fsid != ULIFS_FLAG || ((ULIFS_BPB*)buf)->bps != ULIFS_BPS)	/*���������־�ͷ�����־,�ݲ�֧�ֲ���512�ֽڵ�����*/
		return FS_ERR_WRONG_ARGS;
	if ((fs = (ULIFS*)malloc(sizeof(ULIFS))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	fs->spc = ((ULIFS_BPB*)buf)->spc;
	fs->res = ((ULIFS_BPB*)buf)->res;
	fs->ubm = fs->res + ((ULIFS_BPB*)buf)->spbm;
	fs->spbm = ((ULIFS_BPB*)buf)->spbm;
	fs->CluID = ((ULIFS_BPB*)buf)->cluoff;
	fs->CuCou = ((ULIFS_BPB*)buf)->clucou;
	fs->RemCu = fs->FstCu = 0;
	fs->ubml = FALSE;
	for (i = 0;; i += sizeof(DWORD) * 8)	/*���ҿմ�*/
	{
		DWORD bit;

		if ((i & 0xFFF) == 0)
		{
			res = RwPart(pd, FALSE, fs->ubm + (i >> 12), 1, buf);	/*��ȡʹ��λͼ����*/
			if (res != NO_ERROR)
			{
				free(fs, sizeof(ULIFS));
				return res;
			}
		}
		if ((bit = ((DWORD*)buf)[(i & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*�пմ�*/
		{
			DWORD j;

			for (j = 0; j < sizeof(DWORD) * 8; j++)
				if (!(bit & (1ul << j)))	/*�մ�*/
				{
					if (i + j >= fs->CuCou)
					{
						RwPart(pd, FALSE, fs->CluID, 1, buf);	/*ȡ�þ��*/
						strncpy(pd->part.label, (const char*)&buf[1], LABEL_SIZE - 1);
						pd->part.label[LABEL_SIZE - 1] = 0;
						pd->part.size = (QWORD)fs->CuCou * fs->spc * ULIFS_BPS;
						pd->part.remain = (QWORD)fs->RemCu * fs->spc * ULIFS_BPS;
						pd->part.attr = 0;
						pd->data = fs;
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
void UlifsUmntPart(PART_DESC *pd)
{
	free(pd->data, sizeof(ULIFS));
	pd->data = NULL;
	pd->FsID = ULIFS_ID;
}

/*���÷�����Ϣ*/
long UlifsSetPart(PART_DESC *pd, PART_INFO *pi)
{
	long res;
	ULIFS *data;
	ULIFS_DIR dir[ULIFS_BPS / sizeof(ULIFS_DIR)];

	data = (ULIFS*)pd->data;
	if ((res = RwPart(pd, FALSE, data->CluID, 1, dir)) != NO_ERROR)	/*��ȡ���ݴ��еĵ�һ������*/
		return res;
	memcpy32(pd->part.label, pi->label, LABEL_SIZE / sizeof(DWORD));
	strncpy(&dir->name[1], pi->label, ULIFS_FILE_NAME_SIZE - 1);
	if ((res = RwPart(pd, TRUE, data->CluID, 1, dir)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*��д�ļ�,���ݲ��������ļ�β*/
long UlifsRwFile(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *curc)
{
	QWORD dati, end;	/*��ǰ�ֽ�λ��,�ֽڽ�βλ��*/
	PART_DESC *CurPart;	/*���ڷ����ṹָ��*/
	DWORD bpc;	/*ÿ���ֽ���*/
	BLKID *idxp;	/*�����ڵ�����*/
	long res;
	BLKID blk, idx[ULIFS_BPS * ULIFS_MAX_SPC / sizeof(BLKID)];	/*������*/
	BYTE dat[ULIFS_BPS * ULIFS_MAX_SPC];	/*���ݴ�*/

	dati = 0;
	end = seek + siz;
	CurPart = fd->part;
	bpc = ((ULIFS*)(CurPart->data))->spc * ULIFS_BPS;
	if ((DWORD)seek % bpc + siz <= bpc && curc && *curc)	/*��ȡ��������һ������*/
	{
		if ((res = RwClu(CurPart, FALSE, *curc, 1, dat)) != NO_ERROR)
			return res;
		if (isWrite)
		{
			memcpy8(dat + (DWORD)seek % bpc, buf, siz);
			if ((res = RwClu(CurPart, TRUE, *curc, 1, dat)) != NO_ERROR)
				return res;
		}
		else
			memcpy8(buf, dat + (DWORD)seek % bpc, siz);
		if ((DWORD)end % bpc == 0)
			*curc = 0;
		return NO_ERROR;
	}
	idxp = &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN];
	memcpy32(idxp, ((ULIFS_DIR*)fd->data)->idx, sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));	/*����Ŀ¼���е�����*/
	for (;;)	/*��������ѭ��*/
	{
		DWORD tmpcou;

		blk = *idxp;
		if (blk.cou == 0)
		{
			if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*��ȡ��һ��������*/
				return res;
			idxp = idx;
			continue;
		}
		if (dati + blk.cou * bpc <= seek)	/*δ��ʼ*/
		{
			dati += blk.cou * bpc;	/*������һ��*/
			idxp++;
			continue;
		}
		if (dati < seek)	/*��ǰ�齫��ʼ*/
		{
			tmpcou = (DWORD)((seek - dati) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*tmpcou = (seek - dati) / bpc;*/
			blk.fst += tmpcou;
			blk.cou -= tmpcou;
			tmpcou *= bpc;
			dati += tmpcou;	/*��������ָ��*/
		}
		if (dati < seek)	/*��ǰ�ؽ���ʼ*/
		{
			if (dati + bpc >= end)	/*��ʼ�����*/
				tmpcou = siz;
			else
				tmpcou = bpc - (DWORD)seek % bpc;
			if ((res = RwClu(CurPart, FALSE, blk.fst, 1, dat)) != NO_ERROR)
				return res;
			if (isWrite)
			{
				memcpy8(dat + (DWORD)seek % bpc, buf, tmpcou);
				if ((res = RwClu(CurPart, TRUE, blk.fst, 1, dat)) != NO_ERROR)
					return res;
			}
			else
				memcpy8(buf, dat + (DWORD)seek % bpc, tmpcou);
			if (dati + bpc >= end)	/*��ʼ�����*/
			{
				if (curc)
					*curc = (DWORD)end % bpc ? blk.fst : 0;
				return NO_ERROR;
			}
			else
			{
				blk.fst++;
				blk.cou--;
				dati += bpc;
				buf += tmpcou;
			}
		}
		if (dati + bpc <= end)	/*�����ؿɴ���*/
		{
			tmpcou = (DWORD)((end - dati) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*tmpcou = (end - dati) / bpc;*/
			if (tmpcou > blk.cou)
				tmpcou = blk.cou;
			if ((res = RwClu(CurPart, isWrite, blk.fst, tmpcou, buf)) != NO_ERROR)
				return res;
			blk.fst += tmpcou;
			blk.cou -= tmpcou;
			tmpcou *= bpc;
			dati += tmpcou;
			buf += tmpcou;
			if (dati >= end)	/*�պ����*/
			{
				if (curc)
					*curc = 0;
				return NO_ERROR;
			}
			if (blk.cou == 0)
			{
				idxp++;
				continue;
			}
		}
		if (dati + bpc >= end)	/*��Ҫ���*/
		{
			if ((res = RwClu(CurPart, FALSE, blk.fst, 1, dat)) != NO_ERROR)
				return res;
			if (isWrite)
			{
				memcpy8(dat, buf, (DWORD)end % bpc);
				if ((res = RwClu(CurPart, TRUE, blk.fst, 1, dat)) != NO_ERROR)
					return res;
			}
			else
				memcpy8(buf, dat, (DWORD)end % bpc);
			if (curc)
				*curc = blk.fst;
			return NO_ERROR;
		}
	}
}

/*�����*/
static long AllocClu(PART_DESC *pd, BLKID *blk)
{
	ULIFS *data;
	DWORD fst, end;	/*ѭ��,����λ��*/
	DWORD bm[ULIFS_BPS / sizeof(DWORD)];	/*λͼ*/

	data = (ULIFS*)pd->data;
	lock(&data->ubml);
	blk->fst = fst = data->FstCu;
	if (blk->cou)
		end = fst + blk->cou;
	else
		end = fst + 1;
	RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
	for (;;)
	{
		if ((fst & 0x1F) == 0 && end - fst >= sizeof(DWORD) * 8 && bm[(fst & 0xFFF) >> 5] == 0)	/*����һ��˫�������Ĵ�*/
		{
			bm[(fst & 0xFFF) >> 5] = 0xFFFFFFFF;
			fst += sizeof(DWORD) * 8;
		}
		else	/*����һλ�����Ĵ�*/
		{
			bm[(fst & 0xFFF) >> 5] |= (1ul << (fst & 0x1F));
			fst++;
		}
		if (fst >= end || (bm[(fst & 0xFFF) >> 5] & (1ul << (fst & 0x1F))))
			break;	/*��ɻ������ǿմ�*/
		if ((fst & 0xFFF) == 0)	/*�л���ǰ������*/
		{
			RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
			RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
		}
	}
	RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
	if (blk->cou)
	{
		blk->cou = fst - blk->fst;
		data->RemCu -= blk->cou;
	}
	else
		data->RemCu--;
	for (;; fst += 32)	/*���ҿմ�*/
	{
		DWORD bit;

		if ((fst & 0xFFF) == 0)
			RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);	/*��ȡ������*/
		if ((bit = bm[(fst & 0xFFF) >> 5]) != 0xFFFFFFFF)	/*�пմ�*/
		{
			DWORD j;

			for (j = 0;; j++)
				if (!(bit & (1ul << j)))	/*�մ�*/
				{
					data->FstCu = (fst & 0xFFFFFFE0) + j;	/*ȡ���׿մغ�*/
					ulock(&data->ubml);
					return NO_ERROR;
				}
		}
	}
}

/*���մ�*/
static long FreeClu(PART_DESC *pd, BLKID *blk)
{
	ULIFS *data;
	DWORD fst, end;	/*ѭ��,����λ��,λͼ*/
	DWORD bm[ULIFS_BPS / sizeof(DWORD)];	/*λͼ*/

	data = (ULIFS*)pd->data;
	lock(&data->ubml);
	fst = blk->fst;
	if (blk->cou)
		end = fst + blk->cou;
	else
		end = fst + 1;
	RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
	for (;;)
	{
		if ((fst & 0x1F) == 0 && end - fst >= sizeof(DWORD) * 8)	/*�ͷ�һ��˫�������Ĵ�*/
		{
			bm[(fst & 0xFFF) >> 5] = 0;
			fst += sizeof(DWORD) * 8;
		}
		else	/*�ͷ�һλ�����Ĵ�*/
		{
			bm[(fst & 0xFFF) >> 5] &= (~(1ul << (fst & 0x1F)));
			fst++;
		}
		if (fst >= end)
			break;	/*���*/
		if ((fst & 0xFFF) == 0)	/*�л���ǰ������*/
		{
			RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
			RwPart(pd, FALSE, data->ubm + (fst >> 12), 1, bm);
		}
	}
	RwPart(pd, TRUE, data->ubm + ((fst - 1) >> 12), 1, bm);
	if (blk->cou)
		data->RemCu += blk->cou;
	else
		data->RemCu++;
	if (blk->fst < data->FstCu)
		data->FstCu = blk->fst;
	ulock(&data->ubml);
	return NO_ERROR;
}

/*�����ļ�����*/
long UlifsSetSize(FILE_DESC *fd, QWORD siz)
{
	ULIFS_DIR *data;
	FILE_DESC *par;
	DWORD cun0, cun, clui;/*ԭ������,�޸ĺ������,���ݴ�����*/
	PART_DESC *CurPart;	/*���ڷ����ṹָ��*/
	DWORD bpc;	/*ÿ���ֽ���*/
	BLKID *idxp;	/*�����ڵ�����*/
	long res;
	BLKID idx[ULIFS_BPS * ULIFS_MAX_SPC / sizeof(BLKID)];	/*������*/

	data = (ULIFS_DIR*)fd->data;
	par = fd->par ? fd->par : fd;
	CurPart = fd->part;
	bpc = ((ULIFS*)CurPart->data)->spc * ULIFS_BPS;
	cun0 = (DWORD)((data->size + bpc - 1) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*cun0 = (data->size + bpc - 1) / bpc;*/
	cun = (DWORD)((siz + bpc - 1) / ULIFS_BPS) / (bpc / ULIFS_BPS);	/*cun = (siz + bpc - 1) / bpc;*/
	if (cun == cun0)	/*����Ҫ�޸Ĵ�*/
	{
		fd->file.size = data->size = siz;
		return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
	}
	if (cun > cun0 && cun - cun0 > ((ULIFS*)CurPart->data)->RemCu)
		return FS_ERR_HAVENO_SPACE;	/*����ʣ�������*/
	idxp = &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN];
	memcpy32(idxp, data->idx, sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));	/*����Ŀ¼���е�����*/
	clui = 0;
	if (cun < cun0)	/*��С�ļ�*/
	{
		DWORD PreIdx, CurIdx;	/*ǰ������,��ǰ������λ��*/
		BLKID blk;	/*�����*/
		CurIdx = PreIdx = 0;
		for (;;)	/*��������ѭ��,�����ͷ�λ��*/
		{
			blk = *idxp;
			if (blk.cou == 0)
			{
				if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*��ȡ��һ��������*/
					return res;
				PreIdx = CurIdx;
				CurIdx = blk.fst;
				idxp = idx;
				continue;
			}
			clui += blk.cou;
			if (clui > cun)	/*��ʼ�ͷŴ�*/
				break;
			idxp++;
		}
		blk.cou = clui - cun;
		blk.fst += idxp->cou - blk.cou;
		if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)	/*�ͷŵ�ǰ������ǵĴ�*/
			return res;
		idxp->cou -= blk.cou;
		blk.fst = CurIdx;
		blk.cou = 1;
		if (CurIdx == 0)	/*û�е�ǰ������,�޸�Ŀ¼��*/
			memcpy32(data->idx, &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN], sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));
		else if (idx[0].cou == 0)	/*��ǰ������ȫ��,ɾ����ǰ������*/
		{
			if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
				return res;
		}
		else if (idx[1].cou == 0)	/*��ǰ������ֻʣһ���ڵ�,�ڵ�д��ǰ������*/
		{
			if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
				return res;
			if (PreIdx == 0)	/*û��ǰ������*/
				data->idx[ULIFS_FILE_IDX_LEN - 1] = idx[0];
			else
			{
				BLKID tmpidx[ULIFS_BPS * ULIFS_MAX_SPC / sizeof(BLKID)];	/*��ʱ������*/
				if ((res = RwClu(CurPart, FALSE, PreIdx, 1, tmpidx)) != NO_ERROR)
					return res;
				tmpidx[bpc / sizeof(BLKID) - 1] = idx[0];
				if ((res = RwClu(CurPart, TRUE, PreIdx, 1, tmpidx)) != NO_ERROR)
					return res;
			}
		}
		else	/*д�뵱ǰ����*/
		{
			if ((res = RwClu(CurPart, TRUE, CurIdx, 1, idx)) != NO_ERROR)
				return res;
		}
		if (clui >= cun0)
		{
			fd->file.size = data->size = siz;
			CurPart->part.remain += (cun0 - cun) * bpc;
			return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
		}
		idxp++;
		for (;;)	/*��������ѭ��,�����ͷ�*/
		{
			blk = *idxp;
			if (blk.cou == 0)
			{
				if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*��ȡ��һ��������*/
					return res;
				idxp = idx;
				if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
					return res;
				continue;
			}
			if ((res = FreeClu(CurPart, &blk)) != NO_ERROR)
				return res;
			clui += blk.cou;
			if (clui >= cun0)	/*�ͷ����*/
			{
				fd->file.size = data->size = siz;
				CurPart->part.remain += (cun0 - cun) * bpc;
				return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
			}
			idxp++;
		}
	}
	else	/*�����ļ�*/
	{
		DWORD CurIdx;	/*ǰ������,��ǰ������λ��*/
		BLKID blk;	/*��ǰ�����*/
		CurIdx = 0;
		blk.cou = blk.fst = 0;
		if (cun0)
		{
			for (;;)	/*��������ѭ��,��������λ��*/
			{
				blk = *idxp;
				if (blk.cou == 0)
				{
					if ((res = RwClu(CurPart, FALSE, blk.fst, 1, idx)) != NO_ERROR)	/*��ȡ��һ��������*/
						return res;
					CurIdx = blk.fst;
					idxp = idx;
					continue;
				}
				clui += blk.cou;
				if (clui >= cun0)	/*���ļ�β*/
					break;
				idxp++;
			}
		}
		for (;;)
		{
			BLKID tblk;	/*��ʱ�����*/
			tblk.fst = blk.fst + blk.cou;
			tblk.cou = cun - clui;
			if ((res = AllocClu(CurPart, &tblk)) != NO_ERROR)
				return res;
			if (clui == 0)	/*�ļ���ʼ,���õ�ǰ�ؽڵ�*/
			{
				blk = tblk;
				*idxp = tblk;
			}
			else if (tblk.fst == blk.fst + blk.cou)	/*�ڵ�ǰ�ؿ����,���뵱ǰ�ؽڵ�*/
			{
				blk.cou += tblk.cou;
				idxp->cou += tblk.cou;
			}
			else if (idxp < &idx[bpc / sizeof(BLKID) - 1])	/*������һ���ؽڵ�*/
			{
				blk = tblk;
				*(++idxp) = tblk;
			}
			else	/*������һ��������*/
			{
				BLKID iblk;	/*����������BLKID*/
				iblk.cou = 0;
				if ((res = AllocClu(CurPart, &iblk)) != NO_ERROR)
					return res;
				*idxp = iblk;
				if (CurIdx == 0)
					memcpy32(data->idx, &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN], sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));
				else
				{
					if ((res = RwClu(CurPart, TRUE, CurIdx, 1, idx)) != NO_ERROR)
						return res;
				}
				idx[0] = blk;
				idx[1] = tblk;
				blk = tblk;
				idxp = &idx[1];
				CurIdx = iblk.fst;
			}
			if ((clui += tblk.cou) >= cun)	/*�������*/
			{
				if (CurIdx == 0)
					memcpy32(data->idx, &idx[(bpc / sizeof(BLKID)) - ULIFS_FILE_IDX_LEN], sizeof(BLKID) * ULIFS_FILE_IDX_LEN / sizeof(DWORD));
				else
				{
					if ((res = RwClu(CurPart, TRUE, CurIdx, 1, idx)) != NO_ERROR)
						return res;
				}
				fd->file.size = data->size = siz;
				CurPart->part.remain -= (cun - cun0) * bpc;
				return UlifsRwFile(par, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL);
			}
		}
	}
}

/*�Ƚ�·���ַ������ļ����Ƿ�ƥ��*/
BOOL UlifsCmpFile(FILE_DESC *fd, const char *path)
{
	char *namep = ((ULIFS_DIR*)fd->data)->name;
	while (*namep)
		if (*namep++ != *path++)
			return FALSE;	/*�ļ�����ƥ��*/
	if (*path != '/' && *path)
		return FALSE;
	return TRUE;
}

/*data��Ϣ������info*/
static void DataToInfo(FILE_INFO *fi, ULIFS_DIR *ud)
{
	strncpy(fi->name, ud->name, FILE_NAME_SIZE);
	fi->CreateTime = ud->CreateTime;
	fi->ModifyTime = ud->ModifyTime;
	fi->AccessTime = ud->AccessTime;
	fi->attr = ud->attr;
	fi->size = ud->size;
}

/*info��Ϣ������data*/
static void InfoToData(ULIFS_DIR *ud, FILE_INFO *fi)
{
	strncpy(ud->name, fi->name, ULIFS_FILE_NAME_SIZE - 1);
	ud->name[ULIFS_FILE_NAME_SIZE - 1] = 0;
	ud->CreateTime = fi->CreateTime;
	ud->ModifyTime = fi->ModifyTime;
	ud->AccessTime = fi->AccessTime;
	ud->attr = fi->attr;
	ud->size = fi->size;
}

/*�����������ļ���*/
long UlifsSchFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	ULIFS_DIR *dir;
	long res;

	if ((dir = (ULIFS_DIR*)malloc(sizeof(ULIFS_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	if ((par = fd->par) == NULL)	/*ȡ�ø�Ŀ¼��*/
	{
		ULIFS_DIR tmpdir[ULIFS_BPS / sizeof(ULIFS_DIR)];
		if ((res = RwPart(fd->part, FALSE, ((ULIFS*)fd->part->data)->CluID, 1, tmpdir)) != NO_ERROR)	/*��ȡ���ݴ��еĵ�һ������*/
		{
			free(dir, sizeof(ULIFS_DIR));
			return res;
		}
		*dir = tmpdir[0];
		DataToInfo(&fd->file, dir);
		fd->idx = 0;
		fd->data = dir;
		return NO_ERROR;
	}
	else	/*ȡ������Ŀ¼��*/
	{
		DWORD curc;
		QWORD seek, siz;

		fd->data = dir;
		for (curc = 0, seek = 0, siz = ((ULIFS_DIR*)par->data)->size; seek < siz; seek += sizeof(ULIFS_DIR))
		{
			if ((res = UlifsRwFile(par, FALSE, seek, sizeof(ULIFS_DIR), dir, &curc)) != NO_ERROR)
			{
				fd->data = NULL;
				free(dir, sizeof(ULIFS_DIR));
				return res;
			}
			if (UlifsCmpFile(fd, path))
			{
				DataToInfo(&fd->file, dir);
				fd->idx = seek / sizeof(ULIFS_DIR);
				return NO_ERROR;
			}
		}
		fd->data = NULL;
		free(dir, sizeof(ULIFS_DIR));
		return FS_ERR_PATH_NOT_FOUND;
	}
}

/*����ļ�����ȷ��*/
static long CheckName(const char *name)
{
	const char *namep;
	namep = name;
	while (*namep)	/*�ļ�������*/
	{
		if (namep - name >= ULIFS_FILE_NAME_SIZE)
			return FS_ERR_NAME_TOOLONG;
		if (*namep < 0x20)
			return FS_ERR_NAME_FORMAT;
		namep++;
	}
	return NO_ERROR;
}

/*�����������ļ���*/
long UlifsNewFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	ULIFS_DIR *dir;
	DWORD curc;
	QWORD seek, siz;
	long res;

	if ((res = CheckName(path)) != NO_ERROR)	/*����ļ���*/
		return res;
	if ((res = UlifsSchFile(fd, path)) != FS_ERR_PATH_NOT_FOUND)	/*�ļ����Ѿ�������*/
	{
		if (res == NO_ERROR)
		{
			free(fd->data, sizeof(ULIFS_DIR));	/*�ͷ�SchFile���������*/
			fd->data = NULL;
			return FS_ERR_PATH_EXISTS;
		}
		return res;
	}
	if ((dir = (ULIFS_DIR*)malloc(sizeof(ULIFS_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	/*������λ*/
	par = fd->par;
	for (curc = 0, seek = 0, siz = ((ULIFS_DIR*)par->data)->size; seek < siz; seek += sizeof(ULIFS_DIR))
	{
		if ((res = UlifsRwFile(par, FALSE, seek, sizeof(ULIFS_DIR), dir, &curc)) != NO_ERROR)
		{
			free(dir, sizeof(ULIFS_DIR));
			return res;
		}
		if (dir->name[0] == 0)
			goto crtdir;
	}
	if (seek >= sizeof(ULIFS_DIR) * ULIFS_MAX_DIR)
	{
		free(dir, sizeof(ULIFS_DIR));
		return FS_ERR_SIZE_LIMIT;
	}
	if ((res = UlifsSetSize(par, seek + sizeof(ULIFS_DIR))) != NO_ERROR)
	{
		free(dir, sizeof(ULIFS_DIR));
		return res;
	}
crtdir:	/*����*/
	strcpy(fd->file.name, path);
	fd->file.size = 0;
	InfoToData(dir, &fd->file);
	if ((res = UlifsRwFile(par, TRUE, seek, sizeof(ULIFS_DIR), dir, &curc)) != NO_ERROR)
	{
		free(dir, sizeof(ULIFS_DIR));
		return res;
	}
	fd->idx = seek / sizeof(ULIFS_DIR);
	fd->data = dir;
	return NO_ERROR;
}

/*ɾ���ļ���*/
long UlifsDelFile(FILE_DESC *fd)
{
	ULIFS_DIR *data;
	FILE_DESC *par;
	QWORD seek, siz;
	long res;

	data = (ULIFS_DIR*)fd->data;
	if (data->size)
	{
		if (data->attr & ULIFS_FILE_ATTR_DIREC)	/*���յ�Ŀ¼����ɾ��*/
			return FS_ERR_DIR_NOT_EMPTY;
		else if ((res = UlifsSetSize(fd, 0)) != NO_ERROR)	/*����ļ�ռ�õĿռ�*/
			return res;
	}
	par = fd->par;
	seek = (QWORD)fd->idx * sizeof(ULIFS_DIR);
	siz = ((ULIFS_DIR*)par->data)->size;
	if (seek + sizeof(ULIFS_DIR) >= siz)	/*�������һ��*/
	{
		while (seek)
		{
			if ((res = UlifsRwFile(par, FALSE, seek - sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), data, NULL)) != NO_ERROR)
				return res;
			if (data->name[0])
				break;
			seek -= sizeof(ULIFS_DIR);
		}
		if ((res = UlifsSetSize(par, seek)) != NO_ERROR)
			return res;
	}
	else	/*ֱ�ӱ��Ϊɾ��*/
	{
		data->name[0] = 0;
		if ((res = UlifsRwFile(par, TRUE, seek, sizeof(ULIFS_DIR), data, NULL)) != NO_ERROR)
			return res;
	}
	free(data, sizeof(ULIFS_DIR));
	fd->data = NULL;
	return NO_ERROR;
}

/*�����ļ�����Ϣ*/
long UlifsSetFile(FILE_DESC *fd, FILE_INFO *fi)
{
	if (fd->par && fi->name[0])
	{
		long res;
		if ((res = CheckName(fi->name)) != NO_ERROR)
			return res;
		strcpy(fd->file.name, fi->name);
	}
	if (fi->CreateTime != INVALID)
		fd->file.CreateTime = fi->CreateTime;
	if (fi->ModifyTime != INVALID)
		fd->file.ModifyTime = fi->ModifyTime;
	if (fi->AccessTime != INVALID)
		fd->file.AccessTime = fi->AccessTime;
	if (fi->attr != INVALID)
		fd->file.attr = fi->attr;
	InfoToData(fd->data, &fd->file);
	return UlifsRwFile(fd->par ? fd->par : fd, TRUE, (QWORD)fd->idx * sizeof(ULIFS_DIR), sizeof(ULIFS_DIR), fd->data, NULL);
}

/*��ȡĿ¼�б�*/
long UlifsReadDir(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *curc)
{
	QWORD siz;
	long res;

	for (siz = ((ULIFS_DIR*)fd->data)->size; *seek < siz;)
	{
		ULIFS_DIR dir;
		if ((res = UlifsRwFile(fd, FALSE, *seek, sizeof(ULIFS_DIR), &dir, curc)) != NO_ERROR)
			return res;
		*seek += sizeof(ULIFS_DIR);
		if (dir.name[0] && dir.name[0] != '/')	/*��Ч���ļ���,��ȥ��Ŀ¼*/
		{
			DataToInfo(fi, &dir);
			return NO_ERROR;
		}
	}
	*seek = 0;
	return FS_ERR_END_OF_FILE;
}

/*�ͷ��ļ��������е�˽������*/
void UlifsFreeData(FILE_DESC *fd)
{
	free(fd->data, sizeof(ULIFS_DIR));
	fd->data = NULL;
}

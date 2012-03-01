/*	ulifs.c for ulios file system
	���ߣ�����
	���ܣ�FAT32�ļ�ϵͳ��֧��
	����޸����ڣ�2009-06-06
*/

#include "fs.h"

typedef struct _FAT32_BPB
{
	BYTE	INS_JMP[3];/*��תָ��*/
	BYTE	OEM[8];	/*OEM ID(tian&uli2k_X)*/
	WORD	bps;	/*ÿ�����ֽ���512*/
	BYTE	spc;	/***ÿ��������*/
	WORD	ressec;	/***����������(��һ��FAT��ʼ֮ǰ��������)*/
	BYTE	fats;	/***FAT��һ��Ϊ2*/
	WORD	rtents;	/*��Ŀ¼����(FAT32Ϊ0)*/
	WORD	smlsec;	/*С������(FAT32Ϊ0)*/
	BYTE	media;	/*ý��������Ӳ��0xF8*/
	WORD	spf;	/*ÿFAT������(FAT32Ϊ0)*/
	WORD	spt;	/*ÿ��������*/
	WORD	heads;	/*��ͷ��*/
	DWORD	relsec;	/*��������������ǰ����������*/
	DWORD	totsec;	/***��������*/
	DWORD	spfat;	/***ÿFAT������FAT32ʹ��*/
	WORD	exflg;	/*��չ��־*/
	WORD	fsver;	/*�ļ�ϵͳ�汾*/
	DWORD	rtclu;	/***��Ŀ¼�غ�*/
	WORD	fsinfo;	/***�ļ�ϵͳ��Ϣ������һ��Ϊ1*/
	WORD	bkbot;	/*������������6*/
	BYTE	reser[12];	/*����12�ֽ�*/
	/*����Ϊ��չBPB*/
	BYTE	pdn;	/*������������,��һ��������Ϊ0x80*/
	BYTE	exres;	/*����*/
	BYTE	exbtsg;	/*��չ������ǩΪ0x29*/
	DWORD	volume;	/*�������,�������ִ���*/
	char	vollab[11];/*���*/
	BYTE	fsid[8];/*ϵͳID*/
	BYTE	code[420];	/*��������*/
	WORD	aa55;	/*������־*/
}__attribute__((packed)) FAT32_BPB;	/*FAT32��BPB*/

typedef struct _FAT32
{
	DWORD	spc;	/*ÿ��������*/
	DWORD	fats;	/*fat��Ŀ*/
	DWORD	res;	/*����������(FAT֮ǰ������)*/
	DWORD	fsinfo;	/*�ļ�ϵͳ��Ϣ������һ��Ϊ1*/
	DWORD	spfat;	/*ÿFAT������*/
	DWORD	clu0;	/*0��������*/
	DWORD	clus;	/*������*/
	DWORD	rtclu;	/*��Ŀ¼�غ�*/
	DWORD	fstec;	/*��һ���մغ�*/
	DWORD	rescc;	/*ʣ�������*/
	DWORD	fatl;	/*FAT��*/
}FAT32;	/*�ڴ��е��ļ�ϵͳ��Ϣ�ṹ*/

typedef struct _FAT32_FSI
{
	DWORD	RRaA;
	BYTE	res0[480];
	DWORD	rrAa;
	DWORD	frecou;	/*ʣ�������*/
	DWORD	nxtfre;	/*��һ���մغ�*/
	BYTE	res1[12];
	DWORD	end;
}FAT32_FSI;	/*�ļ���Ϣ����*/

#define FAT32_FILE_MAIN_SIZE	8		/*���ļ�������*/
#define FAT32_FILE_NAME_SIZE	11		/*�ļ����ܳ�*/
#define FAT32_FILE_ATTR_RDONLY	0x01	/*ֻ��*/
#define FAT32_FILE_ATTR_HIDDEN	0x02	/*����*/
#define FAT32_FILE_ATTR_SYSTEM	0x04	/*ϵͳ*/
#define FAT32_FILE_ATTR_LABEL	0x08	/*���(ֻ��)*/
#define FAT32_FILE_ATTR_DIREC	0x10	/*Ŀ¼(ֻ��)*/
#define FAT32_FILE_ATTR_ARCH	0x20	/*�鵵*/
#define FAT32_FILE_ATTR_LONG	(FAT32_FILE_ATTR_RDONLY | FAT32_FILE_ATTR_HIDDEN | FAT32_FILE_ATTR_SYSTEM | FAT32_FILE_ATTR_LABEL)	/*���ļ���*/

typedef struct _FAT32_DIR
{
	char name[FAT32_FILE_NAME_SIZE];	/*�ļ���*/
	BYTE attr;		/*����*/
	BYTE reserved;	/*����*/
	BYTE crtmils;	/*����ʱ��10����λ*/
	WORD crttime;	/*����ʱ��*/
	WORD crtdate;	/*��������*/
	WORD acsdate;	/*��������*/
	WORD idxh;		/*�״ظ�16λ*/
	WORD mdftime;	/*�޸�ʱ��*/
	WORD mdfdate;	/*�޸�����*/
	WORD idxl;		/*�״ص�16λ*/
	DWORD size;		/*�ļ��ֽ���*/
}FAT32_DIR;	/*FAT32Ŀ¼��ṹ*/

#define FAT32_ID		0x0C		/*FAT32�ļ�ϵͳID*/
#define FAT32_BPS		0x200		/*�����ÿ�����ֽ���*/
#define FAT32_MAX_SPC	0x80		/*ÿ������������*/
#define FAT32_MAX_DIR	0xFFFF		/*Ŀ¼��Ŀ¼����������*/

static inline long RwClu(PART_DESC *part, BOOL isWrite, DWORD clu, DWORD cou, void *buf)	/*��д������*/
{
	FAT32 *data = (FAT32*)part->data;
	return RwPart(part, isWrite, data->clu0 + data->spc * clu, data->spc * cou, buf);
}

/*����FAT32����*/
long Fat32MntPart(PART_DESC *pd)
{
	FAT32 *fs;
	DWORD i;
	long res;
	BYTE buf[FAT32_BPS];

	if (pd->FsID != 0x01 && pd->FsID != 0x0B && pd->FsID != 0x0C)
		return FS_ERR_WRONG_ARGS;
	if ((res = RwPart(pd, FALSE, 0, 1, buf)) != NO_ERROR)	/*����������*/
		return res;
	if (((FAT32_BPB*)buf)->aa55 != 0xAA55 || ((FAT32_BPB*)buf)->bps != FAT32_BPS)
		return FS_ERR_WRONG_ARGS;
	if ((fs = (FAT32*)malloc(sizeof(FAT32))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	fs->spc = ((FAT32_BPB*)buf)->spc;
	fs->fats = ((FAT32_BPB*)buf)->fats;
	fs->res = ((FAT32_BPB*)buf)->ressec;
	fs->spfat = ((FAT32_BPB*)buf)->spfat;
	fs->clu0 = ((FAT32_BPB*)buf)->ressec + (((FAT32_BPB*)buf)->spfat * ((FAT32_BPB*)buf)->fats) - (((FAT32_BPB*)buf)->spc << 1);
	fs->clus = (pd->SeCou - fs->clu0) / ((FAT32_BPB*)buf)->spc;
	fs->rtclu = ((FAT32_BPB*)buf)->rtclu;
	fs->fsinfo = ((FAT32_BPB*)buf)->fsinfo;
	strncpy(pd->part.label, ((FAT32_BPB*)buf)->vollab, 11);
	pd->part.label[11] = 0;
	if ((res = RwPart(pd, FALSE, fs->fsinfo, 1, buf)) != NO_ERROR)	/*����Ϣ����*/
	{
		free(fs, sizeof(FAT32));
		return res;
	}
	fs->rescc=((FAT32_FSI*)buf)->frecou;
	fs->fstec=((FAT32_FSI*)buf)->nxtfre;
	fs->fatl = FALSE;
	if (fs->rescc >= fs->clus || fs->fstec >= fs->clus)	/*�غŴ���,����ɨ��*/
	{
		fs->rescc = fs->fstec = 0;
		RwPart(pd, FALSE, fs->res, 1, buf);	/*��1��FAT��*/
		for (i = 2; i < fs->clus; i++)	/*���ҿմ�*/
		{
			if ((i & 0x7F) == 0)
				RwPart(pd, FALSE, fs->res + (i >> 7), 1, buf);
			if (((DWORD*)buf)[i & 0x7F] == 0)	/*(i%128)�ǿմ�*/
			{
				if (fs->fstec == 0)
					fs->fstec = i;	/*ȡ���׿մغ�*/
				fs->rescc++;
			}
		}
	}
	pd->part.size = (QWORD)fs->clus * fs->spc * FAT32_BPS;
	pd->part.remain = (QWORD)fs->rescc * fs->spc * FAT32_BPS;
	pd->part.attr = 0;
	pd->data = fs;
	return NO_ERROR;
}

/*ж��FAT32����*/
void Fat32UmntPart(PART_DESC *pd)
{
	FAT32_FSI buf;

	RwPart(pd, FALSE, ((FAT32*)pd->data)->fsinfo, 1, &buf);
	buf.frecou = ((FAT32*)pd->data)->rescc;
	buf.nxtfre = ((FAT32*)pd->data)->fstec;
	RwPart(pd, TRUE, ((FAT32*)pd->data)->fsinfo, 1, &buf);
	free(pd->data, sizeof(FAT32));
	pd->data = NULL;
	pd->FsID = FAT32_ID;
}

/*���÷�����Ϣ*/
long Fat32SetPart(PART_DESC *pd, PART_INFO *pi)
{
	const char *namep;
	long res;
	FAT32_BPB buf;

	for (namep = pi->label; *namep; namep++)
		if (namep - pi->label >= 11)	/*���������*/
			return FS_ERR_NAME_TOOLONG;
	if ((res = RwPart(pd, FALSE, 0, 1, &buf)) != NO_ERROR)
		return res;
	memcpy32(pd->part.label, pi->label, 3);
	strncpy(buf.vollab, pi->label, 11);
	if ((res = RwPart(pd, TRUE, 0, 1, &buf)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*��д�ļ�,���ݲ��������ļ�β*/
long Fat32RwFile(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *curc)
{
	FAT32_DIR *data;
	DWORD dati, end;	/*��ǰ�ֽ�λ��,�ֽڽ�βλ��*/
	PART_DESC *CurPart;	/*���ڷ����ṹָ��*/
	DWORD bpc;	/*ÿ���ֽ���*/
	DWORD PreIdx, CurIdx;	/*�����ڵ�����*/
	long res;
	DWORD idx[FAT32_BPS / sizeof(DWORD)];	/*��������*/
	BYTE dat[FAT32_BPS * FAT32_MAX_SPC];	/*���ݴ�*/

	data = (FAT32_DIR*)fd->data;
	end = (DWORD)seek + siz;
	CurPart = fd->part;
	bpc = ((FAT32*)(CurPart->data))->spc * FAT32_BPS;
	PreIdx = INVALID;
	if (curc && (*curc))
	{
		CurIdx = *curc;
		dati = (DWORD)seek - (DWORD)seek % bpc;
	}
	else
	{
		CurIdx = ((DWORD)data->idxh << 16) | data->idxl;
		dati = 0;
	}
	for (;;)
	{
		if (dati >= (DWORD)seek)	/*�Ѿ���ʼ*/
		{
			if (dati + bpc > end)	/*��Ҫ���*/
			{
				if ((res = RwClu(CurPart, FALSE, CurIdx, 1, dat)) != NO_ERROR)
					return res;
				if (isWrite)
				{
					memcpy8(dat, buf, end % bpc);
					if ((res = RwClu(CurPart, TRUE, CurIdx, 1, dat)) != NO_ERROR)
						return res;
				}
				else
					memcpy8(buf, dat, end % bpc);
			}
			else	/*�м�*/
			{
				if ((res = RwClu(CurPart, isWrite, CurIdx, 1, buf)) != NO_ERROR)
					return res;
				buf += bpc;
			}
		}
		else if (dati + bpc > (DWORD)seek)	/*��Ҫ��ʼ*/
		{
			DWORD tmpcou;
			if (dati + bpc >= end)	/*��ʼ�����*/
				tmpcou = siz;
			else
				tmpcou = bpc - (DWORD)seek % bpc;
			if ((res = RwClu(CurPart, FALSE, CurIdx, 1, dat)) != NO_ERROR)
				return res;
			if (isWrite)
			{
				memcpy8(dat + (DWORD)seek % bpc, buf, tmpcou);
				if ((res = RwClu(CurPart, TRUE, CurIdx, 1, dat)) != NO_ERROR)
					return res;
			}
			else
				memcpy8(buf, dat + (DWORD)seek % bpc, tmpcou);
			buf += tmpcou;
		}
		dati += bpc;
		if (dati > end)	/*���*/
		{
			if (curc)
				*curc = CurIdx;
			return NO_ERROR;
		}
		if ((CurIdx >> 7) != (PreIdx >> 7))	/*��һ�غŲ��ڻ�����*/
			if ((res = RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (CurIdx >> 7), 1, idx)) != NO_ERROR)
				return res;
		PreIdx = CurIdx;
		CurIdx = idx[CurIdx & 0x7F];
		if (dati >= end)	/*���*/
		{
			if (curc)
				*curc = (CurIdx & 0x0FFFFFFF) != 0x0FFFFFFF ? CurIdx : 0;
			return NO_ERROR;
		}
	}
}

/*�����*/
DWORD AllocClu(PART_DESC *pd, DWORD cou)
{
	DWORD CurIdx, PreIdx, *pidx, *cidx, *swp;
	DWORD idx[2][FAT32_BPS / sizeof(DWORD)];

	lock(&((FAT32*)pd->data)->fatl);
	PreIdx = CurIdx = ((FAT32*)pd->data)->fstec;
	pidx = cidx = idx[0];
	swp = idx[1];
	((FAT32*)pd->data)->rescc -= cou;
	cou--;
	RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (CurIdx >> 7), 1, cidx);
	for (CurIdx++;; CurIdx++)
	{
		if ((CurIdx & 0x7F) == 0)	/*������FAT����*/
		{
			cidx = swp;	/*�����»���*/
			RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (CurIdx >> 7), 1, cidx);
		}
		if (cidx[CurIdx & 0x7F] == 0)	/*�ҵ��մ�*/
		{
			if (cou)	/*����û�����*/
			{
				pidx[PreIdx & 0x7F] = CurIdx;
				if ((CurIdx >> 7) != (PreIdx >> 7))	/*��ǰ����鲻ͬ*/
				{
					RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (PreIdx >> 7), 1, pidx);
					RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (PreIdx >> 7), 1, pidx);
					swp = pidx;	/*��������*/
					pidx = cidx;	/*���õ�ǰ��Ϊǰһ��*/
				}
				PreIdx = CurIdx;
				cou--;
			}
			else	/*ȫ��������*/
			{
				pidx[PreIdx & 0x7F] = 0x0FFFFFFF;
				RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (PreIdx >> 7), 1, pidx);
				RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (PreIdx >> 7), 1, pidx);
				PreIdx = ((FAT32*)pd->data)->fstec;
				((FAT32*)pd->data)->fstec = CurIdx;	/*���������׿մ�*/
				ulock(&((FAT32*)pd->data)->fatl);
				return PreIdx;
			}
		}
	}
}

/*���մ�*/
DWORD FreeClu(PART_DESC *pd, DWORD clu)
{
	DWORD buf[FAT32_BPS / sizeof(DWORD)];	/*��������*/

	lock(&((FAT32*)pd->data)->fatl);
	RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (clu >> 7), 1, buf);
	for (;;)
	{
		DWORD CurIdx;
		if (((FAT32*)pd->data)->fstec > clu)
			((FAT32*)pd->data)->fstec = clu;
		CurIdx = buf[clu & 0x7F];
		buf[clu & 0x7F] = 0;
		((FAT32*)pd->data)->rescc++;
		if ((CurIdx & 0x0FFFFFFF) == 0x0FFFFFFF)
			break;
		if ((CurIdx >> 7) != (clu >> 7))
		{
			RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (clu >> 7), 1, buf);
			RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (clu >> 7), 1, buf);
			RwPart(pd, FALSE, ((FAT32*)pd->data)->res + (CurIdx >> 7), 1, buf);
		}
		clu = CurIdx;
	}
	RwPart(pd, TRUE, ((FAT32*)pd->data)->res + (clu >> 7), 1, buf);
	RwPart(pd, TRUE, ((FAT32*)pd->data)->res + ((FAT32*)pd->data)->spfat + (clu >> 7), 1, buf);
	ulock(&((FAT32*)pd->data)->fatl);
	return NO_ERROR;
}

/*�����ļ�����*/
long Fat32SetSize(FILE_DESC *fd, QWORD siz)
{
	FAT32_DIR *data;
	FILE_DESC *par;
	DWORD cun0, cun, clui, CurIdx, PreIdx;/*ԭ������,�޸ĺ������,���ݴ�����*/
	PART_DESC *CurPart;	/*���ڷ����ṹָ��*/
	DWORD bpc;	/*ÿ���ֽ���*/
	long res;
	DWORD idx[FAT32_BPS / sizeof(DWORD)];	/*��������*/

	if (siz > 0xFFFFFFFF)
		return FS_ERR_SIZE_LIMIT;
	data = (FAT32_DIR*)fd->data;
	par = fd->par;
	CurPart = fd->part;
	bpc = ((FAT32*)CurPart->data)->spc * FAT32_BPS;
	cun0 = (data->size + bpc - 1) / bpc;
	cun = ((DWORD)siz + bpc - 1) / bpc;
	if (cun == cun0)	/*����Ҫ�޸Ĵ�*/
	{
		data->size = (data->attr & FAT32_FILE_ATTR_DIREC) ? 0 : siz;
		fd->file.size = siz;
		if (par)
			return Fat32RwFile(par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL);
		return NO_ERROR;
	}
	if (cun > cun0 && cun - cun0 > ((FAT32*)CurPart->data)->rescc)
		return FS_ERR_HAVENO_SPACE;	/*����ʣ�������*/
	CurIdx = ((DWORD)data->idxh << 16) | data->idxl;
	PreIdx = INVALID;
	clui = 0;
	if (cun < cun0)	/*��С�ļ�*/
	{
		for (;;)
		{
			if (clui >= cun)	/*�Ӵ˿�ʼ����*/
				break;
			clui++;
			if ((CurIdx >> 7) != (PreIdx >> 7))	/*��һ�غŲ��ڻ�����*/
				RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (CurIdx >> 7), 1, idx);
			PreIdx = CurIdx;
			CurIdx = idx[CurIdx & 0x7F];
		}
		if ((res = FreeClu(CurPart, CurIdx)) != NO_ERROR)
			return res;
		if (cun)	/*���ý�����*/
		{
			RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
			idx[PreIdx & 0x7F] = 0x0FFFFFFF;
			RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
			RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + ((FAT32*)CurPart->data)->spfat + (PreIdx >> 7), 1, idx);
		}
		else
			data->idxh = data->idxl = 0;
		CurPart->part.remain += (cun0 - cun) * bpc;
		data->size = (data->attr & FAT32_FILE_ATTR_DIREC) ? 0 : siz;
		fd->file.size = siz;
		if (par)
			return Fat32RwFile(par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL);
		return NO_ERROR;
	}
	else	/*�����ļ�*/
	{
		DWORD clu;
		for (;;)
		{
			if (cun0 == 0 || CurIdx == 0x0FFFFFFF)
				break;
			clui++;
			if ((CurIdx >> 7) != (PreIdx >> 7))	/*��һ�غŲ��ڻ�����*/
				RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (CurIdx >> 7), 1, idx);
			PreIdx = CurIdx;
			CurIdx = idx[CurIdx & 0x7F];
		}
		if (cun - clui)
		{
			clu = AllocClu(CurPart, cun - clui);
			if (cun0)	/*�������Ӵ�*/
			{
				RwPart(CurPart, FALSE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
				idx[PreIdx & 0x7F] = clu;
				RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + (PreIdx >> 7), 1, idx);
				RwPart(CurPart, TRUE, ((FAT32*)CurPart->data)->res + ((FAT32*)CurPart->data)->spfat + (PreIdx >> 7), 1, idx);
			}
			else
			{
				data->idxh = clu >> 16;
				data->idxl = clu;
			}
			CurPart->part.remain -= (cun - cun0) * bpc;
		}
		data->size = (data->attr & FAT32_FILE_ATTR_DIREC) ? 0 : siz;
		fd->file.size = siz;
		if (par)
			return Fat32RwFile(par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL);
		return NO_ERROR;
	}
}

/*�ļ�������Ŀ¼�ṹ*/
static long NameToDir(char DirName[FAT32_FILE_NAME_SIZE], const char *name)
{
	char *namep;

	memset8(DirName, ' ', FAT32_FILE_NAME_SIZE);
	namep = DirName;
	if (*name == (char)0xE5)
	{
		*namep = (char)0x05;
		namep++;
		name++;
	}
	while (*name != '.' && *name != '/' && *name)
	{
		if (namep >= DirName + FAT32_FILE_MAIN_SIZE)
			return FS_ERR_NAME_TOOLONG;
		else if (*name >= 'a' && *name <= 'z')
			*namep = *name - 0x20;
		else
			*namep = *name;
		namep++;
		name++;
	}
	if (*name == '.')
	{
		name++;
		namep = DirName + FAT32_FILE_MAIN_SIZE;
		while (*name != '/' && *name)
		{
			if (namep >= DirName + FAT32_FILE_NAME_SIZE)
				return FS_ERR_NAME_TOOLONG;
			if (*name >= 'a' && *name <= 'z')
				*namep = *name - 0x20;
			else
				*namep = *name;
			namep++;
			name++;
		}
	}
	return NO_ERROR;
}

/*Ŀ¼�ṹ�����ļ���*/
static long DirToName(char *name, char DirName[FAT32_FILE_NAME_SIZE])
{
	char *namep;

	namep = DirName;
	if (*namep == (char)0x05)
	{
		*name = (char)0xE5;
		namep++;
		name++;
	}
	while (namep < DirName + FAT32_FILE_MAIN_SIZE)
	{
		if (*namep == ' ')
			break;
		*name++ = *namep++;
	}
	namep = DirName + FAT32_FILE_MAIN_SIZE;
	if (*namep != ' ')
	{
		*name++ = '.';
		while (namep < DirName + FAT32_FILE_NAME_SIZE)
		{
			if (*namep == ' ')
				break;
			*name++ = *namep++;
		}
	}
	*name = 0;
	return NO_ERROR;
}

/*ʱ������Ŀ¼�ṹ*/
static long TimeToDir(WORD *DirDate, WORD *DirTime, const TM *tm)
{
	if (tm->yer < 1980)
		return TIME_ERR_WRONG_TM;
	if (tm->mon == 0 || tm->mon > 12)
		return TIME_ERR_WRONG_TM;
	if (tm->day == 0 || tm->day > 31)
		return TIME_ERR_WRONG_TM;
	if (tm->hor > 23 || tm->min > 59 || tm->sec > 59)
		return TIME_ERR_WRONG_TM;
	if (DirTime)
		*DirTime = (tm->hor << 11) | (tm->min << 5) | (tm->sec >> 1);
	*DirDate = ((tm->yer - 1980) << 9) | (tm->mon << 5) | tm->day;
	return NO_ERROR;
}

/*Ŀ¼�ṹ����ʱ��*/
static long DirToTime(TM *tm, WORD DirDate, WORD DirTime)
{
	tm->sec = (DirTime << 1) & 0x1F;
	tm->min = (DirTime >> 5) & 0x1F;
	tm->hor = DirTime >> 11;
	tm->day = DirDate & 0x1F;
	tm->mon = (DirDate >> 5) & 0x0F;
	tm->yer = (DirDate >> 9) + 1980;
	return NO_ERROR;
}

/*�Ƚ�·���ַ������ļ����Ƿ�ƥ��*/
BOOL Fat32CmpFile(FILE_DESC *fd, const char *path)
{
	char newnam[FAT32_FILE_NAME_SIZE], *name, *namep;

	if (NameToDir(newnam, path) != NO_ERROR)
		return FALSE;
	name = ((FAT32_DIR*)fd->data)->name;
	for (namep = newnam; namep < newnam + FAT32_FILE_NAME_SIZE; namep++, name++)
		if (*namep != *name)
			return FALSE;
	return TRUE;
}

/*data��Ϣ������info*/
static void DataToInfo(FILE_INFO *fi, FAT32_DIR *fd)
{
	TM tm;
	DirToName(fi->name, fd->name);
	DirToTime(&tm, fd->crtdate, fd->crttime);
	TMMkTime(&fi->CreateTime, &tm);
	DirToTime(&tm, fd->mdfdate, fd->mdftime);
	TMMkTime(&fi->ModifyTime, &tm);
	DirToTime(&tm, fd->acsdate, 0);
	TMMkTime(&fi->AccessTime, &tm);
	fi->attr = fd->attr;
	fi->size = fd->size;
}

/*info��Ϣ������data*/
static void InfoToData(FAT32_DIR *fd, FILE_INFO *fi)
{
	TM tm;
	NameToDir(fd->name, fi->name);
	TMLocalTime(fi->CreateTime, &tm);
	TimeToDir(&fd->crtdate, &fd->crttime, &tm);
	TMLocalTime(fi->ModifyTime, &tm);
	TimeToDir(&fd->mdfdate, &fd->mdftime, &tm);
	TMLocalTime(fi->AccessTime, &tm);
	TimeToDir(&fd->acsdate, NULL, &tm);
	fd->attr = fi->attr & 0x3F;
	fd->size = fi->size;
}

/*�����������ļ���*/
long Fat32SchFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	FAT32_DIR *dir;
	long res;

	if ((dir = (FAT32_DIR*)malloc(sizeof(FAT32_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	if ((par = fd->par) == NULL)	/*ȡ�ø�Ŀ¼��*/
	{
		memset32(dir, 0, sizeof(FAT32_DIR) / sizeof(DWORD));
		dir->idxh = ((FAT32*)fd->part->data)->rtclu >> 16;
		dir->idxl = ((FAT32*)fd->part->data)->rtclu;
		dir->attr = FAT32_FILE_ATTR_DIREC;	/*����:Ŀ¼*/
		fd->file.name[0] = 0;
		fd->file.AccessTime = fd->file.ModifyTime = fd->file.CreateTime = INVALID;
		fd->file.attr = FILE_ATTR_DIREC;
		fd->file.size = 0;
		fd->data = dir;
		return NO_ERROR;
	}
	else	/*ȡ������Ŀ¼��*/
	{
		DWORD curc, seek;

		fd->data = dir;
		for (curc = 0, seek = 0;; seek += sizeof(FAT32_DIR))
		{
			if ((res = Fat32RwFile(par, FALSE, seek, sizeof(FAT32_DIR), dir, &curc)) != NO_ERROR)
			{
				fd->data = NULL;
				free(dir, sizeof(FAT32_DIR));
				return res;
			}
			if (dir->attr & FAT32_FILE_ATTR_LABEL)
				continue;
			if (dir->name[0] == 0)
				break;
			if (Fat32CmpFile(fd, path))
			{
				DataToInfo(&fd->file, dir);
				fd->idx = seek / sizeof(FAT32_DIR);
				return NO_ERROR;
			}
			if (curc == 0)
				break;
		}
		fd->data = NULL;
		free(dir, sizeof(FAT32_DIR));
		return FS_ERR_PATH_NOT_FOUND;
	}
}

/*����ļ�����ȷ��*/
static long CheckName(const char *name)
{
	static const char ErrChar[] = {	/*�����ַ�*/
		0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, 0x7C
	};
	const char *namep, *errcp;
	if (*name == '.')
		return FS_ERR_NAME_FORMAT;
	namep = name;
	while (*namep != '.' && *namep)	/*�����зǷ��ַ��򳬳�*/
	{
		if (namep - name >= FAT32_FILE_MAIN_SIZE)
			return FS_ERR_NAME_TOOLONG;
		if (*namep <= 0x20)
			return FS_ERR_NAME_FORMAT;
		for (errcp = ErrChar; errcp < &ErrChar[sizeof(ErrChar)]; errcp++)
			if (*namep == *errcp)
				return FS_ERR_NAME_FORMAT;
			namep++;
	}
	if (*namep == '.')
	{
		namep++;
		name = namep;
		while (*namep)	/*��չ���зǷ��ַ��򳬳�*/
		{
			if (namep - name >= FAT32_FILE_NAME_SIZE - FAT32_FILE_MAIN_SIZE)
				return FS_ERR_NAME_TOOLONG;
			if (*namep <= 0x20)
				return FS_ERR_NAME_FORMAT;
			for (errcp = ErrChar; errcp < &ErrChar[sizeof(ErrChar)]; errcp++)
				if (*namep == *errcp)
					return FS_ERR_NAME_FORMAT;
				namep++;
		}
	}
	return NO_ERROR;
}

/*�����������ļ���*/
long Fat32NewFile(FILE_DESC *fd, const char *path)
{
	FILE_DESC *par;
	FAT32_DIR *dir;
	DWORD curc, seek;
	DWORD bpc;
	long res;
	FAT32_DIR dat[FAT32_BPS * FAT32_MAX_SPC / sizeof(FAT32_DIR)];	/*���ݴ�*/

	if ((res = CheckName(path)) != NO_ERROR)	/*����ļ���*/
		return res;
	if ((res = Fat32SchFile(fd, path)) != FS_ERR_PATH_NOT_FOUND)	/*�ļ����Ѿ�������*/
	{
		if (res == NO_ERROR)
		{
			free(fd->data, sizeof(FAT32_DIR));	/*�ͷ�SchFile���������*/
			fd->data = NULL;
			return FS_ERR_PATH_EXISTS;
		}
		return res;
	}
	if ((dir = (FAT32_DIR*)malloc(sizeof(FAT32_DIR))) == NULL)
		return FS_ERR_HAVENO_MEMORY;
	bpc = ((FAT32*)fd->part->data)->spc * FAT32_BPS;
	/*������λ*/
	par = fd->par;
	for (curc = 0, seek = 0;; seek += sizeof(FAT32_DIR))
	{
		DWORD tmpcurc = curc;
		if ((res = Fat32RwFile(par, FALSE, seek, sizeof(FAT32_DIR), dir, &curc)) != NO_ERROR)
		{
			free(dir, sizeof(FAT32_DIR));
			return res;
		}
		if (dir->name[0] == (char)0xE5)
		{
			strcpy(fd->file.name, path);
			fd->file.size = 0;
			InfoToData(dir, &fd->file);
			if ((res = Fat32RwFile(par, TRUE, seek, sizeof(FAT32_DIR), dir, &tmpcurc)) != NO_ERROR)
			{
				free(dir, sizeof(FAT32_DIR));
				return res;
			}
			goto crtdir;
		}
		if (dir->name[0] == 0)
			break;
		if (curc == 0)
		{
			seek += sizeof(FAT32_DIR);
			break;
		}
	}
	if (seek >= sizeof(FAT32_DIR) * FAT32_MAX_DIR)
	{
		free(dir, sizeof(FAT32_DIR));
		return FS_ERR_SIZE_LIMIT;
	}
	((FAT32_DIR*)par->data)->size = seek;
	if ((res = Fat32SetSize(par, seek + sizeof(FAT32_DIR))) != NO_ERROR)	/*����Ŀ¼*/
	{
		((FAT32_DIR*)par->data)->size = 0;
		free(dir, sizeof(FAT32_DIR));
		return res;
	}
	strcpy(fd->file.name, path);
	fd->file.size = 0;
	InfoToData(dir, &fd->file);
	memcpy32(&dat[0], dir, sizeof(FAT32_DIR) / sizeof(DWORD));
	memset32(&dat[1], 0, (bpc - seek % bpc - sizeof(FAT32_DIR)) / sizeof(DWORD));
	if ((res = Fat32RwFile(par, TRUE, seek, bpc - seek % bpc, dat, NULL)) != NO_ERROR)
	{
		free(dir, sizeof(FAT32_DIR));
		return res;
	}
crtdir:
	fd->idx = seek / sizeof(FAT32_DIR);
	fd->data = dir;
	if (dir->attr & FAT32_FILE_ATTR_DIREC)	/*Ŀ¼�д���Ŀ¼��.��..*/
	{
		dir->size = 0;
		if ((res = Fat32SetSize(fd, sizeof(FAT32_DIR) * 2)) != NO_ERROR)	/*���ó�ʼĿ¼��С*/
		{
			fd->data = NULL;
			free(dir, sizeof(FAT32_DIR));
			return res;
		}
		memcpy32(&dat[0], dir, sizeof(FAT32_DIR) / sizeof(DWORD));
		dat[0].name[0] = '.';
		memset8(&dat[0].name[1], ' ', FAT32_FILE_NAME_SIZE - 1);
		memcpy32(&dat[1], par->data, sizeof(FAT32_DIR) / sizeof(DWORD));
		dat[1].name[1] = dat[1].name[0] = '.';
		memset8(&dat[1].name[2], ' ', FAT32_FILE_NAME_SIZE - 2);
		if (par->par == NULL)
			dat[1].idxl = dat[1].idxh = 0;
		memset32(&dat[2], 0, (bpc - sizeof(FAT32_DIR) * 2) / sizeof(DWORD));
		if ((res = Fat32RwFile(fd, TRUE, 0, bpc, dat, NULL)) != NO_ERROR)
		{
			fd->data = NULL;
			free(dir, sizeof(FAT32_DIR));
			return res;
		}
	}
	return NO_ERROR;
}

/*ɾ���ļ���*/
long Fat32DelFile(FILE_DESC *fd)
{
	FAT32_DIR *data;
	FILE_DESC *par;
	DWORD curc, seek;
	long res;

	data = (FAT32_DIR*)fd->data;
	if (data->attr & FAT32_FILE_ATTR_DIREC)
	{
		FAT32_DIR dir;
		for (seek = sizeof(FAT32_DIR) * 2, curc = 0;; seek += sizeof(FAT32_DIR))	/*���Ŀ¼�Ƿ��*/
		{
			if ((res = Fat32RwFile(fd, FALSE, seek, sizeof(FAT32_DIR), &dir, &curc)) != NO_ERROR)
				return res;
			if (dir.name[0] == 0)
				break;
			if (curc == 0)
			{
				seek += sizeof(FAT32_DIR);
				break;
			}
			if (dir.name[0] != (char)0xE5)	/*���յ�Ŀ¼����ɾ��*/
				return FS_ERR_DIR_NOT_EMPTY;
		}
		data->size = seek;
		if ((res = Fat32SetSize(fd, 0)) != NO_ERROR)	/*���Ŀ¼*/
		{
			data->size = 0;
			return res;
		}
	}
	else if (data->size)
		if ((res = Fat32SetSize(fd, 0)) != NO_ERROR)	/*����ļ�ռ�õĿռ�*/
			return res;
	par = fd->par;
	seek = fd->idx * sizeof(FAT32_DIR);
	curc = 0;
	data->name[0] = (char)0xE5;
	if ((res = Fat32RwFile(par, TRUE, seek, sizeof(FAT32_DIR), data, &curc)) != NO_ERROR)	/*���Ϊɾ��������ǲ������һ��*/
		return res;
	if (curc && (res = Fat32RwFile(par, FALSE, seek + sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL)) != NO_ERROR)
		return res;
	if (curc == 0 || data->name[0] == 0)	/*�������һ��,���Ŀ¼ռ�õĿռ�*/
	{
		DWORD tmpseek = seek + sizeof(FAT32_DIR);
		while (seek)
		{
			if ((res = Fat32RwFile(par, FALSE, seek - sizeof(FAT32_DIR), sizeof(FAT32_DIR), data, NULL)) != NO_ERROR)
				return res;
			if (data->name[0] != (char)0xE5)
				break;
			seek -= sizeof(FAT32_DIR);
		}
		((FAT32_DIR*)par->data)->size = tmpseek;
		if ((res = Fat32SetSize(par, seek)) != NO_ERROR)
		{
			((FAT32_DIR*)par->data)->size = 0;
			return res;
		}
	}
	free(data, sizeof(FAT32_DIR));
	fd->data = NULL;
	return NO_ERROR;
}

/*�����ļ�����Ϣ*/
long Fat32SetFile(FILE_DESC *fd, FILE_INFO *fi)
{
	if (fd->par)
	{
		if (fi->name[0])
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
		{
			if ((fi->attr & FAT32_FILE_ATTR_LONG) == FAT32_FILE_ATTR_LONG)
				return FS_ERR_WRONG_ARGS;
			fd->file.attr = fi->attr & 0x3F;
		}
		InfoToData(fd->data, &fd->file);
		if (((FAT32_DIR*)fd->data)->attr & FAT32_FILE_ATTR_DIREC)
			((FAT32_DIR*)fd->data)->size = 0;
		return Fat32RwFile(fd->par, TRUE, fd->idx * sizeof(FAT32_DIR), sizeof(FAT32_DIR), fd->data, NULL);
	}
	return NO_ERROR;
}

/*��ȡĿ¼�б�*/
long Fat32ReadDir(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *curc)
{
	long res;

	for (;;)
	{
		FAT32_DIR dir;
		if ((res = Fat32RwFile(fd, FALSE, *seek, sizeof(FAT32_DIR), &dir, curc)) != NO_ERROR)
			return res;
		*seek += sizeof(FAT32_DIR);
		if (dir.name[0] == 0)
			break;
		if (dir.name[0] != (char)0xE5 && !(dir.attr & FAT32_FILE_ATTR_LABEL))	/*��Ч���ļ���,��ȥ���ļ����;��*/
		{
			DataToInfo(fi, &dir);
			return NO_ERROR;
		}
		if (*curc == 0)
			break;
	}
	*seek = 0;
	return FS_ERR_END_OF_FILE;
}

/*�ͷ��ļ��������е�˽������*/
void Fat32FreeData(FILE_DESC *fd)
{
	free(fd->data, sizeof(FAT32_DIR));
	fd->data = NULL;
}

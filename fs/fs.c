/*	fs.c for ulios file system
	���ߣ�����
	���ܣ��ļ�ϵͳ�ϲ㹦�ܣ�ʵ�ַ�������·�������Ͷ����񻷾��µ��ļ���������ʣ�ʹ�ýӿڵ��þ�����ļ�ϵͳʵ��
	����޸����ڣ�2009-06-04
*/

#include "fs.h"

extern long UlifsMntPart(PART_DESC *pd);
extern void UlifsUmntPart(PART_DESC *pd);
extern long UlifsSetPart(PART_DESC *pd, PART_INFO *pi);
extern BOOL UlifsCmpFile(FILE_DESC *fd, const char *path);
extern long UlifsSchFile(FILE_DESC *fd, const char *path);
extern long UlifsNewFile(FILE_DESC *fd, const char *path);
extern long UlifsDelFile(FILE_DESC *fd);
extern long UlifsSetFile(FILE_DESC *fd, FILE_INFO *fi);
extern long UlifsSetSize(FILE_DESC *fd, QWORD siz);
extern long UlifsRwFile(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *avl);
extern long UlifsReadDir(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *avl);
extern void UlifsFreeData(FILE_DESC *fd);

extern long Fat32MntPart(PART_DESC *pd);
extern void Fat32UmntPart(PART_DESC *pd);
extern long Fat32SetPart(PART_DESC *pd, PART_INFO *pi);
extern BOOL Fat32CmpFile(FILE_DESC *fd, const char *path);
extern long Fat32SchFile(FILE_DESC *fd, const char *path);
extern long Fat32NewFile(FILE_DESC *fd, const char *path);
extern long Fat32DelFile(FILE_DESC *fd);
extern long Fat32SetFile(FILE_DESC *fd, FILE_INFO *fi);
extern long Fat32SetSize(FILE_DESC *fd, QWORD siz);
extern long Fat32RwFile(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *avl);
extern long Fat32ReadDir(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *avl);
extern void Fat32FreeData(FILE_DESC *fd);

#define FSUIT_LEN	2
FSUI fsuit[FSUIT_LEN] = {
	{"ulifs", UlifsMntPart, UlifsUmntPart, UlifsSetPart, UlifsCmpFile, UlifsSchFile, UlifsNewFile, UlifsDelFile, UlifsSetFile, UlifsSetSize, UlifsRwFile, UlifsReadDir, UlifsFreeData},
	{"fat32", Fat32MntPart, Fat32UmntPart, Fat32SetPart, Fat32CmpFile, Fat32SchFile, Fat32NewFile, Fat32DelFile, Fat32SetFile, Fat32SetSize, Fat32RwFile, Fat32ReadDir, Fat32FreeData}
};

THREAD_ID AthdPtid;	/*���̷���ID*/
THREAD_ID TimePtid;	/*ʱ�����ID*/
PART_DESC part[PART_LEN];	/*������Ϣ��*/
FILE_DESC* filt[FILT_LEN];	/*���ļ�ָ���*/
FILE_DESC** FstFd;			/*��һ�����ļ�������ָ��*/
FILE_DESC** EndFd;			/*���ǿ��ļ���������һ��ָ��*/
DWORD filtl;				/*�ļ��������*/
PROCRES_DESC* pret[PRET_LEN];	/*������Դ��*/

/*��ʼ���ļ�ϵͳ,������ɹ������˳�*/
long InitFS()
{
	long res;

	if ((res = KRegKnlPort(SRV_FS_PORT)) != NO_ERROR)	/*ע�����˿�*/
		return res;
	if ((res = KGetKptThed(SRV_ATHD_PORT, &AthdPtid)) != NO_ERROR)	/*ȡ�ô������������߳�*/
		return res;
	if ((res = KGetKptThed(SRV_TIME_PORT, &TimePtid)) != NO_ERROR)	/*ȡ��ʱ�����������߳�*/
		return res;
	if ((res = KMapUserAddr(&cache, BLK_SIZ * BMT_LEN + FDAT_SIZ)) != NO_ERROR)	/*������ٻ���������ڴ�ռ�*/
		return res;
	fmt->addr = &fmt[2];	/*��ʼ�������ڴ����*/
	fmt->siz = FDAT_SIZ;
	fmt->nxt = &fmt[1];
	fmt[1].addr = cache + BLK_SIZ * BMT_LEN;
	fmt[1].siz = FDAT_SIZ;
	fmt[1].nxt = NULL;
	memset32(&fmt[2], 0, (FMT_LEN - 2) * sizeof(FREE_BLK_DESC) / sizeof(DWORD));
	fmtl = FALSE;
	memset32(bmt, BLKID_MASK, BMT_LEN * sizeof(CACHE_DESC) / sizeof(DWORD));	/*��ʼ�����ٻ������*/
	cahl = FALSE;
//	memset32(cache, 0, BLK_SIZ * BMT_LEN / sizeof(DWORD));	/*��ո��ٻ���*/
	memset32(part, INVALID, PART_LEN * sizeof(PART_DESC) / sizeof(DWORD));	/*��ʼ�����������*/
	memset32(filt, 0, FILT_LEN * sizeof(FILE_DESC*) / sizeof(DWORD));	/*��ʼ�����ļ������*/
	EndFd = FstFd = filt;
	filtl = FALSE;
	memset32(pret, 0, PRET_LEN * sizeof(PROCRES_DESC*) / sizeof(DWORD));	/*��ʼ��������Դ��*/
	return NO_ERROR;
}

/*��ʼ������*/
void InitPart()
{
	typedef struct _BOOT_SEC_PART
	{
		BYTE boot;	/*����ָʾ�������Ϊ0x80*/
		BYTE beg[3];/*��ʼ��ͷ/����/����*/
		BYTE fsid;	/*�ļ�ϵͳID*/
		BYTE end[3];/*������ͷ/����/����*/
		DWORD fst;	/*�Ӵ��̿�ʼ������������*/
		DWORD cou;	/*��������*/
	}BOOT_SEC_PART;	/*��������*/
	typedef struct _BOOT_SEC
	{
		BYTE code[446];	/*��������*/
		BOOT_SEC_PART part[4];	/*������*/
		WORD aa55;		/*������־*/
	}__attribute__((packed)) BOOT_SEC;	/*���������ṹ*/
	PART_DESC *CurPart;
	DWORD i, j;

	CurPart = part;
	for (i = 0; i < 2; i++)	/*ÿ��Ӳ��*/
	{
		BOOT_SEC mbr;

		if (RwHd(i, FALSE, 0, 1, &mbr) != NO_ERROR)
			continue;
		if (mbr.aa55 != 0xAA55)	/*bad MBR*/
			continue;
		for (j = 0; j < 4; j++)	/*4����������*/
		{
			BOOT_SEC_PART *TmpPart;

			TmpPart = &mbr.part[j];
			if (TmpPart->cou == 0)
				continue;	/*�շ���*/
			if (TmpPart->fsid == 0x05 || TmpPart->fsid == 0x0F)	/*��չ����*/
			{
				DWORD fst;

				fst = TmpPart->fst;
				for (;;)	/*��չ������ÿ���߼�������*/
				{
					BOOT_SEC ebr;

					if (RwHd(i, FALSE, fst, 1, &ebr) != NO_ERROR)
						break;
					if (ebr.aa55 != 0xAA55)	/*bad EBR*/
						break;
					if (ebr.part[0].cou)	/*���ǿ���չ����*/
					{
						CurPart->FsID = ebr.part[0].fsid;
						CurPart->DrvID = i;
						CurPart->SecID = fst + ebr.part[0].fst;
						CurPart->SeCou = ebr.part[0].cou;
						CurPart->data = NULL;
						if (++CurPart >= &part[PART_LEN])
							goto skip;
					}
					if (ebr.part[1].cou == 0)	/*���һ���߼�������*/
						break;
					fst = TmpPart->fst + ebr.part[1].fst;
				}
			}
			else	/*��ͨ������*/
			{
				CurPart->FsID = TmpPart->fsid;
				CurPart->DrvID = i;
				CurPart->SecID = TmpPart->fst;
				CurPart->SeCou = TmpPart->cou;
				CurPart->data = NULL;
				if (++CurPart >= &part[PART_LEN])
					goto skip;
			}
		}
	}
skip:
	for (CurPart = part; CurPart < &part[PART_LEN]; CurPart++)	/*���Թ���*/
		if (CurPart->FsID != INVALID)
			for (i = 0; i < FSUIT_LEN; i++)
				if (fsuit[i].MntPart(CurPart) == NO_ERROR)	/*���سɹ�*/
				{
					CurPart->FsID = i;
					break;
				}
}

/*�ر��ļ�ϵͳ*/
void CloseFS()
{
	PART_DESC *CurPart;

	for (CurPart = part; CurPart < &part[PART_LEN]; CurPart++)	/*ж�ط���*/
		if (CurPart->data)
			fsuit[CurPart->FsID].UmntPart(CurPart);
	SaveCache();	/*���ٻ����д*/
	KFreeAddr(cache);
	KUnregKnlPort(SRV_FS_PORT);
}

/*������ļ�������ID*/
static long AllocFild(FILE_DESC *fd)
{
	lock(&filtl);
	if (FstFd >= &filt[FILT_LEN])
	{
		ulock(&filtl);
		return FS_ERR_HAVENO_FILD;
	}
	fd->id = FstFd - filt;
	*FstFd = fd;
	do
		FstFd++;
	while (FstFd < &filt[FILT_LEN] && *FstFd);
	if (EndFd < FstFd)
		EndFd = FstFd;
	ulock(&filtl);
	return NO_ERROR;
}

/*�ͷſ��ļ�������ID*/
static void FreeFild(WORD fid)
{
	FILE_DESC **fd;

	fd = &filt[fid];
	*fd = NULL;
	if (FstFd > fd)
		FstFd = fd;
	while (EndFd > filt && *(EndFd - 1) == NULL)
		EndFd--;
}

/*������������������*/
static void IncFildLn(FILE_DESC *fd)
{
	while (fd)	/*��Ŀ¼�ϲ�������Ӵ򿪼���*/
	{
		fd->cou++;
		fd = fd->par;
	}
}

/*������������������*/
static void DecFildLn(FILE_DESC *fd)
{
	lock(&filtl);
	while (fd)	/*��Ŀ¼�ϲ���㴦��ڵ�*/
	{
		FILE_DESC *par;

		par = fd->par;
		if (--(fd->cou) == 0)
		{
			if (fd->data)
				fsuit[fd->part->FsID].FreeData(fd);
			FreeFild(fd->id);
			free(fd, sizeof(FILE_DESC));
		}
		fd = par;
	}
	ulock(&filtl);
}

/*���������������Ѽ�¼���ļ�*/
static FILE_DESC *FindFild(FILE_DESC *par, BOOL (*CmpFile)(FILE_DESC *, const char *), const char *path)
{
	FILE_DESC *fd;
	DWORD i;

	lock(&filtl);
	for (i = 0; &filt[i] < EndFd; i++)
		if ((fd = filt[i]) != NULL && fd->par == par && (par ? CmpFile(fd, path) : fd->part == (PART_DESC*)path))	/*����Ŀ¼������*/
		{
			fd->cou++;
			ulock(&filtl);
			return fd;
		}
	ulock(&filtl);
	return NULL;
}

/*����·���ִ������ļ���������*/
static long SetFildLn(PROCRES_DESC *pres, const char *path, BOOL isWrite, FILE_DESC **fd)
{
	PART_DESC *CurPart;
	FILE_DESC *CurFile;
	FSUI *CurFsui;
	BOOL isSch;	/*�Ƿ��������filt*/
	long res;

	if (*path == '/')	/*�Ӹ�Ŀ¼��ʼ*/
	{
		DWORD i;
		for (i = 0, path++; *path >= '0' && *path <= '9'; path++)	/*ȡ�÷�����*/
			i = i * 10 + *path - '0';
		if ((*path != '/' && *path) || *(path - 1) == '/')	/*/0a // /��ʽ,����*/
			return FS_ERR_PATH_FORMAT;
		if (i >= PART_LEN || (CurPart = &part[i])->data == NULL)	/*�����Ŵ�*/
			return FS_ERR_WRONG_PARTID;
		CurFsui = &fsuit[CurPart->FsID];
		if ((CurFile = FindFild(NULL, NULL, (const char*)CurPart)) != NULL)	/*���Ҹ�Ŀ¼��������*/
		{
			if (CurFile->flag & FILE_FLAG_WRITE)	/*�Ѿ�����д��*/
			{
				DecFildLn(CurFile);
				return FS_ERR_PATH_WRITTEN;
			}
			isSch = TRUE;
		}
		else	/*û�ҵ�,�½���Ŀ¼������*/
		{
			if ((CurFile = (FILE_DESC*)malloc(sizeof(FILE_DESC))) == NULL)
				return FS_ERR_HAVENO_MEMORY;
			CurFile->part = CurPart;
			CurFile->flag = 0;
			CurFile->cou = 1;
			CurFile->par = NULL;
			CurFile->data = NULL;
			if ((res = CurFsui->SchFile(CurFile, NULL)) != NO_ERROR)
			{
				free(CurFile, sizeof(FILE_DESC));
				return res;
			}
			if ((res = AllocFild(CurFile)) != NO_ERROR)
			{
				free(CurFile, sizeof(FILE_DESC));
				return res;
			}
			isSch = FALSE;
		}
		while (*path != '/' && *path)	/*�ƶ�·���ַ���ָ��*/
			path++;
		if (*path)
			path++;
	}
	else	/*�ӽ��̵�ǰĿ¼��ȡ�ò���*/
	{
		if ((CurFile = pres->CurDir) == NULL)
			return FS_ERR_WRONG_CURDIR;
		CurPart = CurFile->part;
		CurFsui = &fsuit[CurPart->FsID];
		lock(&filtl);
		IncFildLn(CurFile);
		ulock(&filtl);
		isSch = TRUE;
	}
	while (*path)
	{
		FILE_DESC *TmpFile;

		if (*path == '/')	/*//��ʽ,����*/
		{
			DecFildLn(CurFile);
			return FS_ERR_PATH_FORMAT;
		}
		if (*path == '.')
		{
			if (*(path + 1) == '/' || *(path + 1) == 0)	/*������뵱ǰĿ¼*/
				goto FoundSub;
			if (*(path + 1) == '.' && (*(path + 2) == '/' || *(path + 2) == 0))	/*˫������ϲ�Ŀ¼*/
			{
				if ((TmpFile = CurFile->par) != NULL)	/*���Ǹ�Ŀ¼*/
				{
					if (--(CurFile->cou) == 0)
					{
						if (CurFile->data)
							CurFsui->FreeData(CurFile);
						lock(&filtl);
						FreeFild(CurFile->id);
						ulock(&filtl);
						free(CurFile, sizeof(FILE_DESC));
					}
					CurFile = TmpFile;
				}
				goto FoundSub;
			}
		}
		if (isSch)	/*���Դ����еĽڵ�������*/
		{
			if ((TmpFile = FindFild(CurFile, CurFsui->CmpFile, path)) != NULL)	/*������Ŀ¼������*/
			{
				if (TmpFile->flag & FILE_FLAG_WRITE)	/*�Ѿ�����д��*/
				{
					DecFildLn(TmpFile);
					return FS_ERR_PATH_WRITTEN;
				}
				CurFile = TmpFile;
				goto FoundSub;
			}
			isSch = FALSE;	/*û���ҵ�*/
		}
		/*û�ҵ�,�½���Ŀ¼������*/
		if ((TmpFile = (FILE_DESC*)malloc(sizeof(FILE_DESC))) == NULL)
		{
			DecFildLn(CurFile);
			return FS_ERR_HAVENO_MEMORY;
		}
		TmpFile->part = CurPart;
		TmpFile->flag = 0;
		TmpFile->cou = 1;
		TmpFile->par = CurFile;
		TmpFile->data = NULL;
		if ((res = CurFsui->SchFile(TmpFile, path)) != NO_ERROR)
		{
			free(TmpFile, sizeof(FILE_DESC));
			DecFildLn(CurFile);
			return res;
		}
		if ((res = AllocFild(TmpFile)) != NO_ERROR)
		{
			if (TmpFile->data)
				CurFsui->FreeData(TmpFile);
			free(TmpFile, sizeof(FILE_DESC));
			DecFildLn(CurFile);
			return res;
		}
		CurFile = TmpFile;
FoundSub:
		while (*path != '/' && *path)	/*�ƶ�·���ַ���ָ��*/
			path++;
		if (*path)
		{
			if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*����Ŀ¼���޷���������*/
			{
				DecFildLn(CurFile);
				return FS_ERR_PATH_NOT_FOUND;
			}
			path++;
		}
	}
	if (isWrite)	/*·���������,��д��ʽ��ô*/
	{
		if (CurFile->cou > 1)	/*�����в�ֹһ��������*/
		{
			DecFildLn(CurFile);
			return FS_ERR_PATH_READED;
		}
		CurFile->flag |= FILE_FLAG_WRITE;	/*����д��־*/
	}
	*fd = CurFile;
	return NO_ERROR;
}

/*�������ļ�������*/
static long CreateFild(PROCRES_DESC *pres, const char *path, DWORD attr, FILE_DESC **fd)
{
	PART_DESC *CurPart;
	FILE_DESC *CurFile, *TmpFile;
	FSUI *CurFsui;
	const char *name;
	char DirPath[MAX_PATH];
	long res;

	name = path;	/*����·��������*/
	while (*name)
		name++;
	while (name > path && *name != '/')
		name--;
	if (name == path && *name == '/')	/*/name��ʽ,·����ʽ����*/
		return FS_ERR_PATH_FORMAT;
	if (*(name + 1) == 0)	/*/path/��ʽ,û��ָ������*/
		return FS_ERR_PATH_FORMAT;
	if (*(name + 1) == '.')
	{
		if (*(name + 2) == 0)	/*/path/.��ʽ,���ƴ���*/
			return FS_ERR_PATH_FORMAT;
		if (*(name + 2) == '.' && *(name + 3) == 0)	/*/path/..��ʽ,���ƴ���*/
			return FS_ERR_PATH_FORMAT;
	}
	memcpy8(DirPath, path, name - path);	/*����·��*/
	DirPath[name - path] = 0;
	if ((res = SetFildLn(pres, DirPath, FALSE, &CurFile)) != NO_ERROR)	/*�������Ŀ¼�Ƿ����*/
		return res;
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*�����������ļ�*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_DIR;
	}
	if ((TmpFile = (FILE_DESC*)malloc(sizeof(FILE_DESC))) == NULL)
	{
		DecFildLn(CurFile);
		return FS_ERR_HAVENO_MEMORY;
	}
	CurPart = CurFile->part;
	CurFsui = &fsuit[CurPart->FsID];
	TmpFile->part = CurPart;
	TmpFile->flag = FILE_FLAG_WRITE;
	TmpFile->cou = 1;
	TmpFile->par = CurFile;
	if (TMCurSecond(TimePtid, &TmpFile->file.CreateTime) != NO_ERROR)	/*����Ϊ��ǰʱ��*/
		TmpFile->file.CreateTime = INVALID;
	TmpFile->file.AccessTime = TmpFile->file.ModifyTime = TmpFile->file.CreateTime;
	TmpFile->file.attr = attr;
	TmpFile->data = NULL;
	if (*name == '/')
		name++;
	if ((res = CurFsui->NewFile(TmpFile, name)) != NO_ERROR)	/*�����ļ�������*/
	{
		free(TmpFile, sizeof(FILE_DESC));
		DecFildLn(CurFile);
		return res;
	}
	if ((res = AllocFild(TmpFile)) != NO_ERROR)
	{
		if (TmpFile->data)
			CurFsui->FreeData(TmpFile);
		free(TmpFile, sizeof(FILE_DESC));
		DecFildLn(CurFile);
		return res;
	}
	*fd = TmpFile;
	return NO_ERROR;
}

/*���ҿ��ļ����*/
static FILE_HANDLE *FindFh(FILE_HANDLE *fht)
{
	FILE_HANDLE *fh;

	for (fh = fht; fh < &fht[FHT_LEN]; fh++)
		if (fh->fd == NULL)
			return fh;
	return NULL;
}

/*ȡ�ÿ�ִ���ļ���Ϣ*/
long GetExec(PROCRES_DESC *pres, const char *path, DWORD pid, DWORD *exec)
{
	typedef struct
	{
		DWORD ei_mag;		// �ļ���ʶ
		BYTE ei_class;		// �ļ���
		BYTE ei_data;		// ���ݱ���
		BYTE ei_version;	// �ļ��汾
		BYTE ei_pad;		// �����ֽڿ�ʼ��
		BYTE ei_res[8];		// ����
		WORD e_type;		// Ŀ���ļ�����
		WORD e_machine;		// �ļ���Ŀ����ϵ�ṹ����
		DWORD e_version;	// Ŀ���ļ��汾
		DWORD e_entry;		// ������ڵ������ַ
		DWORD e_phoff;		// ����ͷ������ƫ����
		DWORD e_shoff;		// ����ͷ������ƫ����
		DWORD e_flags;		// �������ļ���ص��ض��ڴ������ı�־
		WORD e_ehsize;		// ELFͷ���Ĵ�С
		WORD e_phentsize;	// ����ͷ�����ı����С
		WORD e_phnum;		// ����ͷ�����ı�����Ŀ
		WORD e_shentsize;	// ����ͷ�����ı����С
		WORD e_shnum;		// ����ͷ�����ı�����Ŀ
		WORD e_shstrndx;	// ����ͷ�����������������ַ�������صı��������
	}ELF32_EHDR;	// ELFͷ�ṹ
	typedef struct
	{
		DWORD p_type;		// ������Ԫ�������Ķε�����
		DWORD p_offset;		// �˳�Ա�������ļ�ͷ���öε�һ���ֽڵ�ƫ��
		DWORD p_vaddr;		// �˳�Ա�����εĵ�һ���ֽڽ����ŵ��ڴ��е������ַ
		DWORD p_paddr;		// �˳�Ա�������������ַ��ص�ϵͳ��
		DWORD p_filesz;		// �˳�Ա���������ļ�ӳ������ռ���ֽ���
		DWORD p_memsz;		// �˳�Ա���������ڴ�ӳ����ռ�õ��ֽ���
		DWORD p_flags;		// �˳�Ա���������صı�־
		DWORD p_align;		// �˳�Ա���������ļ��к��ڴ�����ζ���
	}ELF32_PHDR;	// ����ͷ�ṹ
#define EIM_ELF	0x464C457F	// �ļ���ʾ
#define EV_CURRENT	1		// ��ǰ�汾
#define EIC_32		1		// 32λĿ��
#define ET_EXEC		2		// ��ִ���ļ�
#define EM_386		3		// Intel 80386
#define EM_486		6		// Intel 80486
#define PT_LOAD		1		// �ɼ��صĶ�
#define PF_X		1		// ��ִ��
#define PF_W		2		// ��д

	PROCRES_DESC *ChlPres;	/*�ӽ�����Դ*/
	FILE_DESC *CurFile;
	FSUI *CurFsui;
	long res;
	DWORD i, seek, avl;
	ELF32_EHDR ehdr;
	ELF32_PHDR phdr;

	if (pres->CurDir == NULL)
		return FS_ERR_WRONG_CURDIR;	/*û�����õ�ǰĿ¼*/
	if ((ChlPres = (PROCRES_DESC*)malloc(sizeof(PROCRES_DESC))) == NULL)	/*�����½��̵���Դ*/
		return FS_ERR_HAVENO_MEMORY;
	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*����·��*/
	{
		free(ChlPres, sizeof(PROCRES_DESC));
		return res;
	}
	if (CurFile->file.attr & FILE_ATTR_DIREC)
	{
		DecFildLn(CurFile);
		free(ChlPres, sizeof(PROCRES_DESC));
		return FS_ERR_PATH_NOT_FILE;	/*����������Ŀ¼*/
	}
	if (CurFile->file.size == 0)
	{
		DecFildLn(CurFile);
		free(ChlPres, sizeof(PROCRES_DESC));
		return FS_ERR_FILE_EMPTY;	/*���ļ�����ִ��*/
	}
	memset32(ChlPres, 0, sizeof(PROCRES_DESC) / sizeof(DWORD));
	ChlPres->exec = CurFile;
	ChlPres->CurDir = pres->CurDir;
	IncFildLn(ChlPres->CurDir);	/*���õ�ǰĿ¼*/
	cli();	/*��֤�ӽ�����Դ���ͷ�*/
	while (*((volatile DWORD*)&pret[pid]))
		KGiveUp();
	sti();
	lock(&filtl);
	for (i = 0; i < PRET_LEN; i++)	/*���Ҹ������̺�*/
		if (pret[i] && pret[i]->exec == CurFile)
		{
			memcpy32(&ChlPres->CodeOff, &pret[i]->CodeOff, 7);
			ulock(&filtl);
			exec[0] = i;
			memcpy32(&exec[1], &ChlPres->CodeOff, 7);
			pret[pid] = ChlPres;
			return NO_ERROR;
		}
	ulock(&filtl);
	exec[0] = 0xFFFF;
	CurFsui = &fsuit[CurFile->part->FsID];	/*��ʼȡ��ELF�ļ���Ϣ*/
	if ((res = CurFsui->RwFile(CurFile, FALSE, 0, sizeof(ELF32_EHDR), &ehdr, NULL)) != NO_ERROR)
	{
		DecFildLn(ChlPres->CurDir);
		DecFildLn(CurFile);
		free(ChlPres, sizeof(PROCRES_DESC));
		return res;
	}
	if (ehdr.ei_mag != EIM_ELF ||
		ehdr.ei_class != EIC_32 ||
		ehdr.ei_version != EV_CURRENT ||
		ehdr.e_type != ET_EXEC ||
		(ehdr.e_machine != EM_386 && ehdr.e_machine != EM_486) ||
		ehdr.e_version != EV_CURRENT)	/*ELF��ʽ���*/
	{
		ChlPres->CodeOff = EXEC_DFTENTRY;	/*��ELF��ʽ�İ�BIN����*/
		ChlPres->CodeEnd = EXEC_DFTENTRY + (DWORD)CurFile->file.size;
		ChlPres->CodeSeek = 0;
		ChlPres->DataOff = 0;
		ChlPres->DataEnd = 0;
		ChlPres->DataSeek = 0;
		ChlPres->entry = EXEC_DFTENTRY;
	}
	else
	{
		ChlPres->entry = ehdr.e_entry;
		seek = ehdr.e_phoff;
		avl = 0;
		for (i = 0; i < ehdr.e_phnum; i++)	/*��ȡ����ͷ����*/
		{
			if ((res = CurFsui->RwFile(CurFile, FALSE, seek, ehdr.e_phentsize, &phdr, &avl)) != NO_ERROR)
			{
				DecFildLn(ChlPres->CurDir);
				DecFildLn(CurFile);
				free(ChlPres, sizeof(PROCRES_DESC));
				return res;
			}
			if (phdr.p_type == PT_LOAD)
			{
				if (phdr.p_flags & PF_X)	/*�����*/
				{
					ChlPres->CodeOff = phdr.p_vaddr;
					ChlPres->CodeEnd = phdr.p_vaddr + phdr.p_filesz;
					ChlPres->CodeSeek = phdr.p_offset;
				}
				else if (phdr.p_flags & PF_W)	/*���ݶ�*/
				{
					ChlPres->DataOff = phdr.p_vaddr;
					ChlPres->DataEnd = phdr.p_vaddr + phdr.p_filesz;
					ChlPres->DataSeek = phdr.p_offset;
				}
			}
			seek += ehdr.e_phentsize;
		}
	}
	memcpy32(&exec[1], &ChlPres->CodeOff, 7);
	pret[pid] = ChlPres;
	return NO_ERROR;
}

/*��ȡ��ִ���ļ�ҳ*/
long ReadPage(PROCRES_DESC *pres, void *buf, DWORD siz, DWORD seek)
{
	FILE_DESC *CurFile;
	long res;

	CurFile = pres->exec;
	if (seek >= CurFile->file.size || seek + siz > CurFile->file.size)	/*����,��ȡҳ����*/
		return FS_ERR_WRONG_ARGS;
	if ((res = fsuit[CurFile->part->FsID].RwFile(CurFile, FALSE, seek, siz, buf, NULL)) != NO_ERROR)	/*���ļ�*/
		return res;
	return NO_ERROR;
}

/*�����˳�*/
long ProcExit(PROCRES_DESC *pres)
{
	DWORD i;

	for (i = 0; i < FHT_LEN; i++)	/*�ر������Ѵ򿪵��ļ�*/
		DecFildLn(pres->fht[i].fd);
	DecFildLn(pres->exec);
	DecFildLn(pres->CurDir);
	return NO_ERROR;
}

/*ö�ٷ���*/
long EnumPart(PROCRES_DESC *pres, DWORD *pid)
{
	while (*pid < PART_LEN)
	{
		if (part[*pid].data)
			return NO_ERROR;
		(*pid)++;
	}
	return FS_ERR_HAVENO_PART;
}

/*ȡ�÷�����Ϣ*/
long GetPart(PROCRES_DESC *pres, DWORD pid, PART_INFO *pi)
{
	if (pid >= PART_LEN || part[pid].data == NULL)
		return FS_ERR_WRONG_PARTID;
	memcpy32(pi, &part[pid].part, sizeof(PART_INFO) / sizeof(DWORD));
	memcpy32((void*)pi + sizeof(PART_INFO), fsuit[part[pid].FsID].name, 2);
	return NO_ERROR;
}

/*�����ļ�*/
long creat(PROCRES_DESC *pres, const char *path, DWORD *fhi)
{
	FILE_HANDLE *fh;
	long res;

	if ((fh = FindFh(pres->fht)) == NULL)	/*���ļ�����*/
		return FS_ERR_HAVENO_HANDLE;
	if ((res = CreateFild(pres, path, 0, &fh->fd)) != NO_ERROR)	/*�����ļ������*/
		return res;
	fh->seek = fh->avl = 0;	/*��ʼ���Զ�ֵ�Ͷ�дָ��*/
	*fhi = fh - pres->fht;
	return NO_ERROR;
}

/*���ļ�*/
long open(PROCRES_DESC *pres, const char *path, BOOL isWrite, DWORD *fhi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if ((fh = FindFh(pres->fht)) == NULL)	/*���ļ�����*/
		return FS_ERR_HAVENO_HANDLE;
	if ((res = SetFildLn(pres, path, isWrite, &CurFile)) != NO_ERROR)	/*����·��*/
		return res;
	if (CurFile->file.attr & FILE_ATTR_DIREC)	/*����������Ŀ¼*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_FILE;
	}
	if (isWrite && CurFile->file.attr & FILE_ATTR_RDONLY)	/*��Ҫдֻ���ļ�*/
	{
		DecFildLn(CurFile);
		return FS_ERR_ATTR_RDONLY;
	}
	fh->fd = CurFile;
	fh->seek = fh->avl = 0;	/*��ʼ���Զ�ֵ�Ͷ�дָ��*/
	*fhi = fh - pres->fht;
	return NO_ERROR;
}

/*�ر��ļ�*/
long close(PROCRES_DESC *pres, DWORD fhi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	FILE_INFO fi;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*���������*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*�վ��*/
	fi.name[0] = 0;
	fi.ModifyTime = fi.CreateTime = INVALID;
	if (TMCurSecond(TimePtid, &fi.AccessTime) != NO_ERROR)	/*����Ϊ��ǰʱ��*/
		fi.AccessTime = INVALID;
	fi.attr = INVALID;
	if (CurFile->flag & FILE_FLAG_WRITE)	/*��д���ļ�Ҫ�����޸�ʱ��*/
		fi.ModifyTime = fi.AccessTime;
	res = fsuit[CurFile->part->FsID].SetFile(CurFile, &fi);
	DecFildLn(CurFile);
	fh->fd = NULL;
	return res;
}

/*��ȡ�ļ�*/
long read(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*���������*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*�վ��*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*��Ŀ¼���*/
	if (fh->seek >= CurFile->file.size)
		*siz = 0;
	if (*siz == 0)
		return NO_ERROR;
	if (fh->seek + *siz > CurFile->file.size)
		*siz = CurFile->file.size - fh->seek;	/*����,����������*/
	if ((res = fsuit[CurFile->part->FsID].RwFile(CurFile, FALSE, fh->seek, *siz, buf, &fh->avl)) != NO_ERROR)	/*���ļ�*/
		return res;
	fh->seek += *siz;	/*�޸Ķ�дָ��*/
	return NO_ERROR;
}

/*д���ļ�*/
long write(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	FSUI *CurFsui;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*���������*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*�վ��*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*��Ŀ¼���*/
	if (!(CurFile->flag & FILE_FLAG_WRITE))
		return FS_ERR_PATH_READED;	/*����д�򿪶����ļ�*/
	if (*siz == 0)
		return NO_ERROR;
	CurFsui = &fsuit[CurFile->part->FsID];
	if (fh->seek + *siz > CurFile->file.size)	/*����,�����ļ��ֽ���*/
		if ((res = CurFsui->SetSize(CurFile, fh->seek + *siz)) != NO_ERROR)
			return res;
	if ((res = CurFsui->RwFile(CurFile, TRUE, fh->seek, *siz, buf, &fh->avl)) != NO_ERROR)	/*д�ļ�*/
		return res;
	fh->seek += *siz;	/*�޸Ķ�дָ��*/
	return NO_ERROR;
}

/*���ö�дָ��*/
long seek(PROCRES_DESC *pres, DWORD fhi, SQWORD seek, DWORD from)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*���������*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*�վ��*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*��Ŀ¼���*/
	switch (from)
	{
	case FS_SEEK_SET:
		if (seek < 0)
			return FS_ERR_WRONG_ARGS;
		fh->seek = seek;
		break;
	case FS_SEEK_CUR:
		if ((SQWORD)fh->seek + seek < 0)
			return FS_ERR_WRONG_ARGS;
		fh->seek = (SQWORD)fh->seek + seek;
		break;
	case FS_SEEK_END:
		if ((SQWORD)CurFile->file.size + seek < 0)
			return FS_ERR_WRONG_ARGS;
		fh->seek = (SQWORD)CurFile->file.size + seek;
		break;
	default:
		return FS_ERR_WRONG_ARGS;
	}
	return NO_ERROR;
}

/*�����ļ���С*/
long SetSize(PROCRES_DESC *pres, DWORD fhi, QWORD siz)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*���������*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*�վ��*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*��Ŀ¼���*/
	if (!(CurFile->flag & FILE_FLAG_WRITE))
		return FS_ERR_PATH_READED;	/*����д�򿪶����ļ�*/
	if ((res = fsuit[CurFile->part->FsID].SetSize(CurFile, siz)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*��Ŀ¼*/
long OpenDir(PROCRES_DESC *pres, const char *path, DWORD *fhi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if ((fh = FindFh(pres->fht)) == NULL)	/*���ļ�����*/
		return FS_ERR_HAVENO_HANDLE;
	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*����·��*/
		return res;
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*�����������ļ�*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_DIR;
	}
	fh->fd = CurFile;
	fh->seek = fh->avl = 0;	/*��ʼ���Զ�ֵ�Ͷ�дָ��*/
	*fhi = fh - pres->fht;
	return NO_ERROR;
}

/*��ȡĿ¼*/
long ReadDir(PROCRES_DESC *pres, DWORD fhi, FILE_INFO *fi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*���������*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*�վ��*/
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))
		return FS_ERR_PATH_NOT_DIR;	/*���ļ����*/
	if ((res = fsuit[CurFile->part->FsID].ReadDir(CurFile, &fh->seek, fi, &fh->avl)) != NO_ERROR)	/*��Ŀ¼*/
		return res;
	return NO_ERROR;
}

/*�л���ǰĿ¼*/
long ChDir(PROCRES_DESC *pres, const char *path)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*����·��*/
		return res;
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*�����������ļ�*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_DIR;
	}
	DecFildLn(pres->CurDir);	/*����ԭ��ǰĿ¼*/
	pres->CurDir = CurFile;
	return NO_ERROR;
}

/*����Ŀ¼*/
long MkDir(PROCRES_DESC *pres, const char *path)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = CreateFild(pres, path, FILE_ATTR_DIREC, &CurFile)) != NO_ERROR)	/*�����ļ������*/
		return res;
	DecFildLn(CurFile);
	return NO_ERROR;
}

/*ɾ���ļ����Ŀ¼*/
long remove(PROCRES_DESC *pres, const char *path)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*����·��*/
		return res;
	if (CurFile->file.attr & (FILE_ATTR_RDONLY || FILE_ATTR_SYSTEM))	/*ֻ����ϵͳ�ļ�����ɾ��*/
	{
		DecFildLn(CurFile);
		return FS_ERR_ATTR_RDONLY;
	}
	res = fsuit[CurFile->part->FsID].DelFile(CurFile);
	DecFildLn(CurFile);
	return res;
}

/*�������ļ���Ŀ¼*/
long rename(PROCRES_DESC *pres, const char *path, const char *name)
{
	PART_DESC *CurPart;
	FILE_DESC *CurFile, TmpFile;
	FSUI *CurFsui;
	char *namep;
	long res;

	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*�������Ŀ¼�Ƿ����*/
		return res;
	CurPart = CurFile->part;
	CurFsui = &fsuit[CurPart->FsID];
	TmpFile.part = CurPart;	/*����ļ��Ƿ����*/
	TmpFile.par = CurFile->par;
	TmpFile.data = NULL;
	if ((res = CurFsui->SchFile(&TmpFile, name)) != FS_ERR_PATH_NOT_FOUND)
	{
		if (TmpFile.data)
			CurFsui->FreeData(&TmpFile);
		DecFildLn(CurFile);
		if (res != NO_ERROR)
			return res;
		return FS_ERR_PATH_EXISTS;
	}
	namep = TmpFile.file.name;
	while (*name)
	{
		if (*name == '/')	/*��ʽ����*/
		{
			if (TmpFile.data)
				CurFsui->FreeData(&TmpFile);
			DecFildLn(CurFile);
			return FS_ERR_PATH_FORMAT;
		}
		*namep++ = *name++;
		if (namep - TmpFile.file.name >= FILE_NAME_SIZE)	/*���Ƴ���*/
		{
			if (TmpFile.data)
				CurFsui->FreeData(&TmpFile);
			DecFildLn(CurFile);
			return FS_ERR_NAME_TOOLONG;
		}
	}
	*namep = 0;
	TmpFile.file.AccessTime = TmpFile.file.ModifyTime = TmpFile.file.CreateTime = INVALID;
	TmpFile.file.attr = INVALID;
	res = CurFsui->SetFile(CurFile, &TmpFile.file);	/*�����ļ�������*/
	if (TmpFile.data)
		CurFsui->FreeData(&TmpFile);
	DecFildLn(CurFile);
	return res;
}

/*ȡ���ļ���Ŀ¼��������Ϣ*/
long GetAttr(PROCRES_DESC *pres, const char *path, FILE_INFO *fi)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*����·��*/
		return res;
	memcpy32(fi, &CurFile->file, sizeof(FILE_INFO) / sizeof(DWORD));
	DecFildLn(CurFile);
	return NO_ERROR;
}

/*�����ļ���Ŀ¼������*/
long SetAttr(PROCRES_DESC *pres, const char *path, DWORD attr)
{
	FILE_DESC *CurFile;
	FILE_INFO fi;
	long res;

	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*����·��*/
		return res;
	fi.name[0] = 0;
	fi.AccessTime = fi.ModifyTime = fi.CreateTime = INVALID;
	fi.attr = CurFile->file.attr;
	if (fi.attr & FILE_ATTR_UNMDFY)
	{
		DecFildLn(CurFile);
		return FS_ERR_ATTR_UNMDFY;	/*���Բ����޸�*/
	}
	if (fi.attr & FILE_ATTR_DIREC)
		attr |= FILE_ATTR_DIREC;
	else
		attr &= (~FILE_ATTR_DIREC);
	if (fi.attr & FILE_ATTR_LABEL)
		attr |= FILE_ATTR_LABEL;
	else
		attr &= (~FILE_ATTR_LABEL);
	fi.attr = attr;
	res = fsuit[CurFile->part->FsID].SetFile(CurFile, &fi);
	DecFildLn(CurFile);
	return res;
}

/*�����ļ���Ŀ¼��ʱ��*/
long SetTime(PROCRES_DESC *pres, const char *path, DWORD time, DWORD cma)
{
	FILE_DESC *CurFile;
	FILE_INFO fi;
	long res;

	if (!(cma & (FS_SETIM_CREATE | FS_SETIM_MODIFY | FS_SETIM_ACCESS)))
		return FS_ERR_WRONG_ARGS;
	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*����·��*/
		return res;
	fi.name[0] = 0;
	fi.CreateTime = (cma & FS_SETIM_CREATE) ? time : INVALID;
	fi.ModifyTime = (cma & FS_SETIM_MODIFY) ? time : INVALID;
	fi.AccessTime = (cma & FS_SETIM_ACCESS) ? time : INVALID;
	fi.attr = INVALID;
	res = fsuit[CurFile->part->FsID].SetFile(CurFile, &fi);
	DecFildLn(CurFile);
	return res;
}

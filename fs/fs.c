/*	fs.c for ulios
	���ߣ�����
	���ܣ��ļ�ϵͳ�ϲ㹦�ܣ�ʵ�ַ�������·�������Ͷ����񻷾��µ��ļ���������ʣ�ʹ�ýӿڵ��þ�����ļ�ϵͳʵ��
	����޸����ڣ�2009-06-04
*/

#include "../include/kernel.h"
#include "../include/error.h"

/*��ʼ���ļ�ϵͳ��*/
void InitFS()
{
	memset32(filmt, 0, FiltLen * sizeof(FILE_DESC *) / sizeof(DWORD));
	FildEID = FildFID = 1;
	memset32(part, 0, PartLen * sizeof(PART_DESC *) / sizeof(DWORD));
	memset32(fsuit, 0, FsuitLen * sizeof(FSUI *) / sizeof(DWORD));
}

/*�ر��ļ�ϵͳ*/
void CloseFS()
{
}

/*���ط���*/
long sys_mount(DWORD ptid)
{
	PART_DESC *CurPd;
	DWORD i;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*���������*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*����������*/
	}
	if (CurPd->FsID != INVALID)
	{
		ULOCK(fs_l);
		return ERROR_FS_PART_MOUNTED;	/*�����Ѿ�������*/
	}
	for (i = 0; i < FsuitLen; i++)
		if (fsuit[i] && fsuit[i]->MntPart(ptid) == NO_ERROR)	/*�ֱ���й��ز���*/
		{
			CurPd->FsID = i;
			ULOCK(fs_l);
			return NO_ERROR;
		}
	ULOCK(fs_l);
	return ERROR_FS_PART_UNRECOGN;
}

/*ж�ط���*/
long sys_umount(DWORD ptid)
{
	DWORD i;
	PART_DESC *CurPd;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*���������*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*����������*/
	}
	if (CurPd->FsID == INVALID)
	{
		ULOCK(fs_l);
		return ERROR_FS_PART_UNMOUNTED;	/*����û�б�����*/
	}
	for (i = 1; i < FildEID; i++)
		if (filmt[i] && filmt[i]->PartID == ptid)
		{
			ULOCK(fs_l);
			return ERROR_FS_PART_INUSED;	/*�������ڱ�ʹ��*/
		}
	fsuit[CurPd->FsID]->UmntPart(CurPd);	/*����ж��*/
	CurPd->FsID = INVALID;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*ȡ�÷�����Ϣ*/
long sys_getpart(DWORD ptid, PART_ATTR *pa)
{
	PART_DESC *CurPd;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*���������*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*����������*/
	}
	*pa = CurPd->attr;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*���÷�����Ϣ*/
long sys_setpart(DWORD ptid, PART_ATTR *pa)
{
	PART_DESC *CurPd;
	long res;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*���������*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*����������*/
	}
	if (CurPd->FsID == INVALID)
	{
		ULOCK(fs_l);
		return ERROR_FS_PART_UNMOUNTED;	/*����û�б�����*/
	}
	res = fsuit[CurPd->FsID]->SetPart(CurPd, pa);	/*���÷�����Ϣ*/
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	strcpy(CurPd->attr.label, pa->label);	/*ֻ���ƾ��*/
	ULOCK(fs_l);
	return NO_ERROR;
}

/*��ʽ������*/
long sys_fmtpart(DWORD ptid, DWORD FsID)
{
	DWORD i;
	PART_DESC *CurPd;
	long res;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*���������*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*����������*/
	}
	if (FsID >= FsuitLen)
	{
		ULOCK(fs_l);
		return ERROR_FS_WRONG_FSID;		/*���������*/
	}
	if (fsuit[FsID] == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_FS;		/*�ļ�ϵͳʵ��������*/
	}
	if (CurPd->FsID != INVALID)	/*�����Ѿ������أ�����ж��*/
	{
		for (i = 1; i < FildEID; i++)
			if (filmt[i] && filmt[i]->PartID == ptid)
			{
				ULOCK(fs_l);
				return ERROR_FS_PART_INUSED;	/*�������ڱ�ʹ��*/
			}
		fsuit[CurPd->FsID]->UmntPart(CurPd);	/*����ж��*/
		CurPd->FsID = INVALID;
	}
	res = fsuit[FsID]->FmtPart(CurPd);	/*���и�ʽ��*/
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	CurPd->FsID = FsID;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*�½�һ��������*/
long NewFild(DWORD *id)
{
	FILE_DESC *fd;

	if ((fd = (FILE_DESC *)kmalloc(sizeof(FILE_DESC))) == NULL)
		return ERROR_FS_HAVENO_MEMORY;
	fd->flag = 0;
	fd->cou = 1;
	fd->par = *id;
	fd->data = NULL;
	if (FildFID >= FiltLen)
	{
		kfree(fd);
		return ERROR_FS_HAVENO_FILD;	/*filmt����*/
	}
	filmt[*id = FildFID++] = fd;
	while (FildFID < FiltLen && filmt[FildFID])	/*��������id*/
		FildFID++;
	if (FildEID < FildFID)	/*����ĩ��id*/
		FildEID = FildFID;
	return NO_ERROR;
}

/*ɾ��һ��������*/
void DelFild(DWORD id)
{
	FILE_DESC *fd;

	fd = filmt[id];
	fd->cou--;
	if (fd->cou == 0)
	{
		filmt[id] = NULL;
		if (FildFID > id)	/*��������id*/
			FildFID = id;
		while (FildEID > 1 && filmt[FildEID - 1] == NULL)	/*����ĩ��id*/
			FildEID--;
		if (fd->data)
			fsuit[part[fd->PartID]->FsID]->FreeData(fd);
		kfree(fd, sizeof(FILE_DESC));
	}
}

/*������������������*/
void AddFildLn(DWORD id)
{
	while (id)	/*��Ŀ¼�ϲ�������Ӵ򿪼���*/
	{
		FILE_DESC *fd;

		fd = filmt[id];
		fd->cou++;
		id = fd->par;
	}
}

/*����һ����������*/
void DecFildLn(DWORD id)
{
	while (id)	/*��Ŀ¼�ϲ���㴦��ڵ�*/
	{
		DWORD tid = filmt[id]->par;

		DelFild(id);
		id = tid;
	}
}

/*����һ��·���ִ������ļ���������,�ɹ���������ͷID*/
long SetFildLn(BYTE *path, BOOL isWrite, DWORD *id)
{
	DWORD i, PartID;	/*Ŀ¼ID,������*/
	BOOL isSch;	/*�Ƿ��������filmt*/
	FILE_DESC *CurFd;
	FSUI *CurFsui;
	long res;

	if (*path == '/')	/*�Ӹ�Ŀ¼��ʼ*/
	{
		for (PartID = 0, path++; *path >= '0' && *path <= '9'; path++)	/*ȡ�÷�����*/
			PartID = PartID * 10 + *path - '0';
		if ((*path != '/' && *path) || *(path - 1) == '/')	/*/0a // /��ʽ,����*/
			return ERROR_FS_PATH_FORMAT;
		if (PartID >= PartLen || part[PartID] == NULL)	/*�����Ŵ�*/
			return ERROR_FS_WRONG_PARTID;
		CurFsui = fsuit[part[PartID]->FsID];
		for (i = 1; i < FildEID; i++)
			if ((CurFd = filmt[i]) != NULL && CurFd->par == 0 && CurFd->PartID == PartID)	/*�ҵ���Ŀ¼��������*/
			{
				if (CurFd->flag & FILE_FLAG_WRITE)	/*�Ѿ�����д��*/
					return ERROR_FS_PATH_WRITTEN;
				CurFd->cou++;
				isSch = TRUE;
				goto FoundRoot;
			}
		/*û�ҵ�,�½���Ŀ¼������*/
		i = 0;
		res = NewFild(&i);
		if (res != NO_ERROR)
			return res;
		CurFd = filmt[i];
		CurFd->PartID = PartID;
		res = CurFsui->SchFile(CurFd, NULL);
		if (res != NO_ERROR)
		{
			DelFild(i);
			return res;
		}
		isSch = FALSE;
FoundRoot:	/*�ƶ�·���ַ���ָ��*/
		while (*path != '/' && *path)
			path++;
		if (*path)
			path++;
		else if (isWrite)	/*·���������,��д�뷽ʽ��*/
		{
			if (CurFd->cou > 1)	/*�����в�ֹһ��������*/
			{
				DelFild(i);
				return ERROR_FS_PATH_READED;
			}
			else
				CurFd->flag |= FILE_FLAG_WRITE;	/*����д���־*/
		}
	}
	else	/*������ǰĿ¼��ȡ�ò���*/
	{
		if (*path == 0 && isWrite)	/*��ͼ�򿪵�ǰĿ¼����д��*/
			return ERROR_FS_PATH_READED;
		i = pmt[pid].ts->CurFID;
		CurFd = filmt[i];
		PartID = CurFd->PartID;
		CurFsui = fsuit[part[PartID]->FsID];
		AddFildLn(i);
		isSch = TRUE;
	}
	while (*path)
	{
		if (*path == '/')
		{
			DecFildLn(i);
			return ERROR_FS_PATH_FORMAT;
		}
		if (*path == '.' && *(path + 1) == '.' && (*(path + 2) == '/' || *(path + 2) == 0))	/*�����ϲ�Ŀ¼*/
		{
			if (CurFd->par)	/*���Ǹ�Ŀ¼*/
			{
				DWORD tid = filmt[i]->par;

				DelFild(i);
				i = tid;
			}
			goto FoundFile;
		}
		if (isSch)	/*���Դ����еĽڵ�������*/
		{
			DWORD j;

			for (j = 1; j < FildEID; j++)
				if ((CurFd = filmt[j]) != NULL && CurFd->par == i && CurFsui->CmpFile(CurFd, path))	/*�ҵ�������*/
				{
					if (CurFd->flag & FILE_FLAG_WRITE)	/*�Ѿ�����д��*/
					{
						DecFildLn(i);
						return ERROR_FS_PATH_WRITTEN;
					}
					CurFd->cou++;
					i = j;
					goto FoundFile;
				}
			isSch = FALSE;	/*û���ҵ�*/
		}
		/*û�ҵ�,�½���Ŀ¼������*/
		res = NewFild(&i);
		if (res != NO_ERROR)
		{
			DecFildLn(i);
			return res;
		}
		CurFd = filmt[i];
		CurFd->PartID = PartID;
		res = CurFsui->SchFile(CurFd, path);
		if (res != NO_ERROR)
		{
			DecFildLn(i);
			return res;
		}
FoundFile:	/*�ƶ�·���ַ���ָ��*/
		while (*path != '/' && *path)
			path++;
		if (*path)
		{
			if (!(CurFd->attr.attr & FILE_ATTR_DIREC))	/*����Ŀ¼���޷���������*/
			{
				DecFildLn(i);
				return ERROR_FS_FILE_NOT_DIR;
			}
			path++;
		}
		else if (isWrite)	/*·���������,��д�뷽ʽ��*/
		{
			if (CurFd->cou > 1)	/*�����в�ֹһ��������*/
			{
				DecFildLn(i);
				return ERROR_FS_PATH_READED;
			}
			else
				CurFd->flag |= FILE_FLAG_WRITE;	/*����д���־*/
		}
	}
	*id = i;
	return NO_ERROR;
}

/*�������ļ���*/
long NewFile(BYTE *path, DWORD attr, BOOL isWrite, DWORD *id)
{
	DWORD i, PartID;
	BYTE *name, *namep;
	FILE_DESC *CurFd;
	FSUI *CurFsui;
	long res;

	/*����·��������*/
	name = path;
	while (*name)
		name++;
	while (name > path && *name != '/')
		name--;
	if (*name == '/')	/*��ָ��·���д���*/
	{
		if (*(name + 1) == 0)	/*/path/��ʽ,û��ָ������*/
			return ERROR_FS_PATH_FORMAT;
		*name = 0;	/*���ȼ������Ŀ¼�Ƿ����,�ٴ���*/
		res = SetFildLn(path, FALSE, &i);
		*(name++) = '/';	/*�ָ�·���ַ���*/
		if (res != NO_ERROR)	/*����·��*/
			return res;
		if (!(filmt[i]->attr.attr & FILE_ATTR_DIREC))	/*�����������ļ�*/
		{
			DecFildLn(i);
			return ERROR_FS_FILE_NOT_DIR;
		}
	}
	else	/*�ڵ�ǰĿ¼����*/
	{
		i = pmt[pid].ts->CurFID;
		AddFildLn(i);
	}
	CurFd = filmt[i];
	PartID = CurFd->PartID;
	CurFsui = fsuit[part[PartID]->FsID];
	res = NewFild(&i);
	if (res != NO_ERROR)
	{
		DecFildLn(i);
		return res;
	}
	CurFd = filmt[i];
	CurFd->PartID = PartID;
	CurFd->flag = isWrite ? FILE_FLAG_WRITE : 0;
	CurFd->attr.AccessTime = CurFd->attr.ModifyTime = CurFd->attr.CreateTime = clock;
	CurFd->attr.attr = attr;
	res = CurFsui->NewFile(CurFd, name);
	if (res != NO_ERROR)
	{
		DecFildLn(i);
		return res;
	}
	*id = i;
	return NO_ERROR;
}

/*���ҿ��ļ����*/
long FindFh(FILE_HANDLE *fht, DWORD *id)
{
	DWORD i;

	for (i = 0; i < FhtLen; i++)
		if (fht[i].fid == 0)
		{
			*id = i;
			return NO_ERROR;
		}
	return ERROR_FS_HAVENO_FH;
}

/*�����ļ�,���ؾ��*/
long sys_creat(BYTE *path)
{
	DWORD i, j;
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	long res;

	if (FindFh(fh, &j) != NO_ERROR)	/*���ļ�����*/
		return ERROR_FS_HAVENO_FH;
	LOCK(fs_l);
	res = NewFile(path, 0, TRUE, &i);
	ULOCK(fs_l);
	if (res != NO_ERROR)	/*�����ļ������*/
		return res;
	fh += j;
	fh->fid = i;
	fh->seek = 0;	/*��ʼ����дָ��͵�ǰ�غ�*/
	return j;
}

/*���ļ�,���ؾ��*/
long sys_open(BYTE *path, BOOL isWrite)
{
	DWORD i, j;
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (FindFh(fh, &j) != NO_ERROR)	/*���ļ�����*/
		return ERROR_FS_HAVENO_FH;
	LOCK(fs_l);
	res = SetFildLn(path, isWrite, &i);
	if (res != NO_ERROR)	/*����·��*/
	{
		ULOCK(fs_l);
		return res;
	}
	CurFd = filmt[i];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)	/*����������Ŀ¼*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_DIR_NOT_FILE;
	}
	if (isWrite && CurFd->attr.attr & FILE_ATTR_RDONLY)	/*��Ҫдֻ���ļ�*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_WRITE_RDONLY;
	}
	ULOCK(fs_l);
	fh += j;
	fh->fid = i;
	fh->seek = 0;	/*��ʼ����дָ��͵�ǰ�غ�*/
	return j;
}

/*�ر��ļ�*/
long sys_close(DWORD fhi)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_ATTR fa;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*���������*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*�վ��*/
	fa.name[0] = 0;
	fa.ModifyTime = fa.CreateTime = INVALID;
	fa.AccessTime = clock;
	fa.attr = INVALID;
	LOCK(fs_l);
	CurFd = filmt[fh->fid];
	if (CurFd->flag & FILE_FLAG_WRITE)	/*��д���ļ�Ҫ�����޸�ʱ��*/
		fa.ModifyTime = fa.AccessTime;
	res = fsuit[part[CurFd->PartID]->FsID]->SetFile(CurFd, &fa);
	DecFildLn(fh->fid);
	ULOCK(fs_l);
	fh->fid = 0;
	return res;
}

/*���ļ��ֽ�,���ض����ֽ���*/
long sys_read(DWORD fhi, BYTE *buf, DWORD siz)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*���������*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*�վ��*/
	CurFd = filmt[fh->fid];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*��Ŀ¼���*/
	if (siz == 0)
		return 0;
	if (fh->seek + siz > CurFd->attr.size)
		siz = CurFd->attr.size - fh->seek;	/*����,����������*/
	res = fsuit[part[CurFd->PartID]->FsID]->RwFile(CurFd, FALSE, fh->seek, siz, buf);	/*���ļ�*/
	if (res != NO_ERROR)
		return res;
	fh->seek += siz;	/*�޸Ķ�дָ��*/
	return siz;
}

/*д�ļ��ֽ�,����д���ֽ���*/
long sys_write(DWORD fhi, BYTE *buf, DWORD siz)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	FSUI *CurFsui;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*���������*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*�վ��*/
	CurFd = filmt[fh->fid];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*��Ŀ¼���*/
	if (!(CurFd->flag & FILE_FLAG_WRITE))
		return ERROR_FS_FILE_READED;	/*����д�򿪶����ļ�*/
	if (siz == 0)
		return 0;
	CurFsui = fsuit[part[CurFd->PartID]->FsID];
	if (fh->seek + siz > CurFd->attr.size)	/*����,�����ļ��ֽ���*/
	{
		res = CurFsui->SetSize(CurFd, fh->seek + siz);
		if (res != NO_ERROR)
			return res;
	}
	res = CurFsui->RwFile(CurFd, TRUE, fh->seek, siz, buf);	/*д�ļ�*/
	if (res != NO_ERROR)
		return res;
	fh->seek += siz;	/*�޸Ķ�дָ��*/
	return siz;
}

/*���ö�дָ��*/
long sys_lseek(DWORD fhi, long long off, DWORD from)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*���������*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*�վ��*/
	if (filmt[fh->fid]->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*��Ŀ¼���*/
	switch (from)
	{
	case FS_SEEK_SET:
		fh->seek = off;
		break;
	case FS_SEEK_CUR:
		fh->seek += off;
		break;
	case FS_SEEK_END:
		fh->seek = filmt[fh->fid]->attr.size + off;
		break;
	default:
		return ERROR_FS_SEEKARGV;
	}
	return NO_ERROR;
}

/*�����ļ���С*/
long sys_setsize(DWORD fhi, QWORD siz)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*���������*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*�վ��*/
	CurFd = filmt[fh->fid];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*��Ŀ¼���*/
	if (!(CurFd->flag & FILE_FLAG_WRITE))
		return ERROR_FS_FILE_READED;	/*����д�򿪶����ļ�*/
	res = fsuit[part[CurFd->PartID]->FsID]->SetSize(CurFd, siz);
	if (res != NO_ERROR)
		return res;
	fh->seek = 0;
	return NO_ERROR;
}

/*��Ŀ¼,���ؾ��*/
long sys_opendir(BYTE *path)
{
	DWORD i, j;
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	long res;

	if (FindFh(fh, &j) != NO_ERROR)	/*���ļ�����*/
		return ERROR_FS_HAVENO_FH;
	LOCK(fs_l);
	res = SetFildLn(path, FALSE, &i);
	if (res != NO_ERROR)	/*����·��*/
	{
		ULOCK(fs_l);
		return res;
	}
	if (!(filmt[i]->attr.attr & FILE_ATTR_DIREC))	/*�����������ļ�*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_FILE_NOT_DIR;
	}
	ULOCK(fs_l);
	fh += j;
	fh->fid = i;
	fh->seek = 0;	/*��ʼ����дָ��*/
	return j;
}

/*��Ŀ¼�е�Ŀ¼����*/
long sys_readdir(DWORD fhi, FILE_ATTR *fa)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*���������*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*�վ��*/
	CurFd = filmt[fh->fid];
	if (!(CurFd->attr.attr & FILE_ATTR_DIREC))
		return ERROR_FS_FILE_NOT_DIR;	/*���ļ����*/
	res = fsuit[part[CurFd->PartID]->FsID]->ReadDir(CurFd, &fh->seek, fa);	/*��Ŀ¼*/
	if (res != NO_ERROR)
		return res;
	if (fh->seek == 0)
		return ERROR_FS_END_OF_FILE;
	return NO_ERROR;
}

/*ת����ǰĿ¼*/
long sys_chdir(BYTE *path)
{
	DWORD i;
	long res;

	LOCK(fs_l);
	res = SetFildLn(path, FALSE, &i);
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	if (!(filmt[i]->attr.attr & FILE_ATTR_DIREC))	/*�����������ļ�*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_FILE_NOT_DIR;
	}
	DecFildLn(pmt[pid].ts->CurFID);	/*����ԭ��ǰĿ¼*/
	pmt[pid].ts->CurFID = i;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*����Ŀ¼*/
long sys_mkdir(BYTE *path)
{
	DWORD i;
	long res;

	ULOCK(fs_l);
	res = NewFile(path, FILE_ATTR_DIREC, FALSE, &i);
	if (res != NO_ERROR)	/*�����ļ������*/
	{
		ULOCK(fs_l);
		return res;
	}
	DecFildLn(i);
	ULOCK(fs_l);
	return NO_ERROR;
}

/*ɾ���ļ����Ŀ¼*/
long sys_remove(BYTE *path)
{
	DWORD i;
	FILE_DESC *CurFd;
	FSUI *CurFsui;
	long res;

	ULOCK(fs_l);
	res = SetFildLn(path, TRUE, &i);
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	CurFd = filmt[i];
	if (CurFd->attr.attr & FILE_ATTR_RDONLY)	/*ֻ���ļ�����ɾ��*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_WRITE_RDONLY;
	}
	CurFsui = fsuit[part[CurFd->PartID]->FsID];
	if (CurFd->attr.size)
	{
		if (CurFd->attr.attr & FILE_ATTR_DIREC)	/*���յ�Ŀ¼����ɾ��*/
		{
			DecFildLn(i);
			ULOCK(fs_l);
			return ERROR_FS_DIR_NOEMPTY;
		}
		else
		{
			res = CurFsui->SetSize(CurFd, 0);
			if (res != NO_ERROR)
			{
				DecFildLn(i);
				ULOCK(fs_l);
				return res;
			}
		}
	}
	res = CurFsui->DelFile(CurFd);
	DecFildLn(i);
	ULOCK(fs_l);
	return res;
}

/*�������ļ���Ŀ¼*/
long sys_rename(BYTE *path, BYTE *name)
{
	DWORD i;
	BYTE *namep;
	FILE_ATTR fa;
	FILE_DESC *CurFd;
	long res;

	ULOCK(fs_l);
	res = SetFildLn(path, TRUE, &i);
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	namep = fa.name;
	while (*name != '/' && *name)
	{
		*namep++ = *name++;
		if (namep - fa.name >= FILE_NAME_SIZE)	/*���Ƴ���*/
		{
			DecFildLn(i);
			ULOCK(fs_l);
			return ERROR_FS_NAME_TOOLONG;
		}
	}
	*namep = 0;
	fa.AccessTime = fa.ModifyTime = fa.CreateTime = INVALID;
	fa.attr = INVALID;
	CurFd = filmt[i];
	res = fsuit[part[CurFd->PartID]->FsID]->SetFile(CurFd, &fa);
	DecFildLn(i);
	ULOCK(fs_l);
	return res;
}

/*ȡ���ļ���Ŀ¼��������Ϣ*/
long sys_getattr(BYTE *path, FILE_ATTR *fa)
{
	DWORD i;
	long res;

	ULOCK(fs_l);
	res = SetFildLn(path, FALSE, &i);
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	*fa = filmt[i]->attr;
	DecFildLn(i);
	ULOCK(fs_l);
	return NO_ERROR;
}

/*�����ļ���Ŀ¼������*/
long sys_setattr(BYTE *path, DWORD attr)
{
	DWORD i;
	FILE_ATTR fa;
	FILE_DESC *CurFd;
	long res;

	ULOCK(fs_l);
	res = SetFildLn(path, TRUE, &i);
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	CurFd = filmt[i];
	fa.name[0] = 0;
	fa.AccessTime = fa.ModifyTime = fa.CreateTime = INVALID;
	fa.attr = CurFd->attr.attr;
	if (fa.attr & FILE_ATTR_UNMDFY)
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_ATTR_UNMDFY;	/*���Բ����޸�*/
	}
	if (fa.attr & FILE_ATTR_DIREC)
		attr |= FILE_ATTR_DIREC;
	else
		attr &= (~FILE_ATTR_DIREC);
	fa.attr = attr;
	res = fsuit[part[CurFd->PartID]->FsID]->SetFile(CurFd, &fa);
	DecFildLn(i);
	ULOCK(fs_l);
	return res;
}

/*�����ļ���Ŀ¼��ʱ��*/
long sys_setftime(BYTE *path, DWORD time, DWORD cma)
{
	DWORD i;
	FILE_ATTR fa;
	FILE_DESC *CurFd;
	long res;

	if (!(cma & 0x07))
		return ERROR_FS_TIMEARGV;
	ULOCK(fs_l);
	res = SetFildLn(path, TRUE, &i);
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	fa.name[0] = 0;
	fa.CreateTime = (cma & 0x01) ? time : INVALID;
	fa.ModifyTime = (cma & 0x02) ? time : INVALID;
	fa.AccessTime = (cma & 0x04) ? time : INVALID;
	fa.attr = INVALID;
	CurFd = filmt[i];
	res = fsuit[part[CurFd->PartID]->FsID]->SetFile(CurFd, &fa);
	DecFildLn(i);
	ULOCK(fs_l);
	return res;
}

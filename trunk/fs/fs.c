/*	fs.c for ulios
	作者：孙亮
	功能：文件系统上层功能，实现分区管理，路径解析和多任务环境下的文件共享互斥访问，使用接口调用具体的文件系统实例
	最后修改日期：2009-06-04
*/

#include "../include/kernel.h"
#include "../include/error.h"

/*初始化文件系统表*/
void InitFS()
{
	memset32(filmt, 0, FiltLen * sizeof(FILE_DESC *) / sizeof(DWORD));
	FildEID = FildFID = 1;
	memset32(part, 0, PartLen * sizeof(PART_DESC *) / sizeof(DWORD));
	memset32(fsuit, 0, FsuitLen * sizeof(FSUI *) / sizeof(DWORD));
}

/*关闭文件系统*/
void CloseFS()
{
}

/*挂载分区*/
long sys_mount(DWORD ptid)
{
	PART_DESC *CurPd;
	DWORD i;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*检查句柄错误*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*分区不存在*/
	}
	if (CurPd->FsID != INVALID)
	{
		ULOCK(fs_l);
		return ERROR_FS_PART_MOUNTED;	/*分区已经被挂载*/
	}
	for (i = 0; i < FsuitLen; i++)
		if (fsuit[i] && fsuit[i]->MntPart(ptid) == NO_ERROR)	/*分别进行挂载测试*/
		{
			CurPd->FsID = i;
			ULOCK(fs_l);
			return NO_ERROR;
		}
	ULOCK(fs_l);
	return ERROR_FS_PART_UNRECOGN;
}

/*卸载分区*/
long sys_umount(DWORD ptid)
{
	DWORD i;
	PART_DESC *CurPd;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*检查句柄错误*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*分区不存在*/
	}
	if (CurPd->FsID == INVALID)
	{
		ULOCK(fs_l);
		return ERROR_FS_PART_UNMOUNTED;	/*分区没有被挂载*/
	}
	for (i = 1; i < FildEID; i++)
		if (filmt[i] && filmt[i]->PartID == ptid)
		{
			ULOCK(fs_l);
			return ERROR_FS_PART_INUSED;	/*分区正在被使用*/
		}
	fsuit[CurPd->FsID]->UmntPart(CurPd);	/*进行卸载*/
	CurPd->FsID = INVALID;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*取得分区信息*/
long sys_getpart(DWORD ptid, PART_ATTR *pa)
{
	PART_DESC *CurPd;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*检查句柄错误*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*分区不存在*/
	}
	*pa = CurPd->attr;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*设置分区信息*/
long sys_setpart(DWORD ptid, PART_ATTR *pa)
{
	PART_DESC *CurPd;
	long res;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*检查句柄错误*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*分区不存在*/
	}
	if (CurPd->FsID == INVALID)
	{
		ULOCK(fs_l);
		return ERROR_FS_PART_UNMOUNTED;	/*分区没有被挂载*/
	}
	res = fsuit[CurPd->FsID]->SetPart(CurPd, pa);	/*设置分区信息*/
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	strcpy(CurPd->attr.label, pa->label);	/*只复制卷标*/
	ULOCK(fs_l);
	return NO_ERROR;
}

/*格式化分区*/
long sys_fmtpart(DWORD ptid, DWORD FsID)
{
	DWORD i;
	PART_DESC *CurPd;
	long res;

	if (ptid >= PartLen)
		return ERROR_FS_WRONG_PARTID;	/*检查句柄错误*/
	LOCK(fs_l);
	CurPd = part[ptid];
	if (CurPd == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_PART;	/*分区不存在*/
	}
	if (FsID >= FsuitLen)
	{
		ULOCK(fs_l);
		return ERROR_FS_WRONG_FSID;		/*检查句柄错误*/
	}
	if (fsuit[FsID] == NULL)
	{
		ULOCK(fs_l);
		return ERROR_FS_HAVENO_FS;		/*文件系统实例不存在*/
	}
	if (CurPd->FsID != INVALID)	/*分区已经被挂载，尝试卸载*/
	{
		for (i = 1; i < FildEID; i++)
			if (filmt[i] && filmt[i]->PartID == ptid)
			{
				ULOCK(fs_l);
				return ERROR_FS_PART_INUSED;	/*分区正在被使用*/
			}
		fsuit[CurPd->FsID]->UmntPart(CurPd);	/*进行卸载*/
		CurPd->FsID = INVALID;
	}
	res = fsuit[FsID]->FmtPart(CurPd);	/*进行格式化*/
	if (res != NO_ERROR)
	{
		ULOCK(fs_l);
		return res;
	}
	CurPd->FsID = FsID;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*新建一个描述符*/
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
		return ERROR_FS_HAVENO_FILD;	/*filmt表满*/
	}
	filmt[*id = FildFID++] = fd;
	while (FildFID < FiltLen && filmt[FildFID])	/*调整空项id*/
		FildFID++;
	if (FildEID < FildFID)	/*调整末项id*/
		FildEID = FildFID;
	return NO_ERROR;
}

/*删除一个描述符*/
void DelFild(DWORD id)
{
	FILE_DESC *fd;

	fd = filmt[id];
	fd->cou--;
	if (fd->cou == 0)
	{
		filmt[id] = NULL;
		if (FildFID > id)	/*调整空项id*/
			FildFID = id;
		while (FildEID > 1 && filmt[FildEID - 1] == NULL)	/*调整末项id*/
			FildEID--;
		if (fd->data)
			fsuit[part[fd->PartID]->FsID]->FreeData(fd);
		kfree(fd, sizeof(FILE_DESC));
	}
}

/*增加描述符链读计数*/
void AddFildLn(DWORD id)
{
	while (id)	/*向目录上层逐层增加打开计数*/
	{
		FILE_DESC *fd;

		fd = filmt[id];
		fd->cou++;
		id = fd->par;
	}
}

/*减少一条描述符链*/
void DecFildLn(DWORD id)
{
	while (id)	/*向目录上层逐层处理节点*/
	{
		DWORD tid = filmt[id]->par;

		DelFild(id);
		id = tid;
	}
}

/*根据一个路径字串设置文件描述符链,成功则设置链头ID*/
long SetFildLn(BYTE *path, BOOL isWrite, DWORD *id)
{
	DWORD i, PartID;	/*目录ID,分区号*/
	BOOL isSch;	/*是否可以搜索filmt*/
	FILE_DESC *CurFd;
	FSUI *CurFsui;
	long res;

	if (*path == '/')	/*从根目录开始*/
	{
		for (PartID = 0, path++; *path >= '0' && *path <= '9'; path++)	/*取得分区号*/
			PartID = PartID * 10 + *path - '0';
		if ((*path != '/' && *path) || *(path - 1) == '/')	/*/0a // /格式,错误*/
			return ERROR_FS_PATH_FORMAT;
		if (PartID >= PartLen || part[PartID] == NULL)	/*分区号错*/
			return ERROR_FS_WRONG_PARTID;
		CurFsui = fsuit[part[PartID]->FsID];
		for (i = 1; i < FildEID; i++)
			if ((CurFd = filmt[i]) != NULL && CurFd->par == 0 && CurFd->PartID == PartID)	/*找到根目录的描述符*/
			{
				if (CurFd->flag & FILE_FLAG_WRITE)	/*已经被打开写了*/
					return ERROR_FS_PATH_WRITTEN;
				CurFd->cou++;
				isSch = TRUE;
				goto FoundRoot;
			}
		/*没找到,新建根目录描述符*/
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
FoundRoot:	/*移动路径字符串指针*/
		while (*path != '/' && *path)
			path++;
		if (*path)
			path++;
		else if (isWrite)	/*路径处理完成,以写入方式打开*/
		{
			if (CurFd->cou > 1)	/*发现有不止一个访问者*/
			{
				DelFild(i);
				return ERROR_FS_PATH_READED;
			}
			else
				CurFd->flag |= FILE_FLAG_WRITE;	/*设置写入标志*/
		}
	}
	else	/*从任务当前目录中取得参数*/
	{
		if (*path == 0 && isWrite)	/*企图打开当前目录进行写入*/
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
		if (*path == '.' && *(path + 1) == '.' && (*(path + 2) == '/' || *(path + 2) == 0))	/*进入上层目录*/
		{
			if (CurFd->par)	/*不是根目录*/
			{
				DWORD tid = filmt[i]->par;

				DelFild(i);
				i = tid;
			}
			goto FoundFile;
		}
		if (isSch)	/*可以从已有的节点中搜索*/
		{
			DWORD j;

			for (j = 1; j < FildEID; j++)
				if ((CurFd = filmt[j]) != NULL && CurFd->par == i && CurFsui->CmpFile(CurFd, path))	/*找到描述符*/
				{
					if (CurFd->flag & FILE_FLAG_WRITE)	/*已经被打开写了*/
					{
						DecFildLn(i);
						return ERROR_FS_PATH_WRITTEN;
					}
					CurFd->cou++;
					i = j;
					goto FoundFile;
				}
			isSch = FALSE;	/*没有找到*/
		}
		/*没找到,新建根目录描述符*/
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
FoundFile:	/*移动路径字符串指针*/
		while (*path != '/' && *path)
			path++;
		if (*path)
		{
			if (!(CurFd->attr.attr & FILE_ATTR_DIREC))	/*不是目录，无法继续打开了*/
			{
				DecFildLn(i);
				return ERROR_FS_FILE_NOT_DIR;
			}
			path++;
		}
		else if (isWrite)	/*路径处理完成,以写入方式打开*/
		{
			if (CurFd->cou > 1)	/*发现有不止一个访问者*/
			{
				DecFildLn(i);
				return ERROR_FS_PATH_READED;
			}
			else
				CurFd->flag |= FILE_FLAG_WRITE;	/*设置写入标志*/
		}
	}
	*id = i;
	return NO_ERROR;
}

/*创建空文件项*/
long NewFile(BYTE *path, DWORD attr, BOOL isWrite, DWORD *id)
{
	DWORD i, PartID;
	BYTE *name, *namep;
	FILE_DESC *CurFd;
	FSUI *CurFsui;
	long res;

	/*分离路径和名称*/
	name = path;
	while (*name)
		name++;
	while (name > path && *name != '/')
		name--;
	if (*name == '/')	/*在指定路径中创建*/
	{
		if (*(name + 1) == 0)	/*/path/格式,没有指定名称*/
			return ERROR_FS_PATH_FORMAT;
		*name = 0;	/*首先检查所在目录是否存在,再创建*/
		res = SetFildLn(path, FALSE, &i);
		*(name++) = '/';	/*恢复路径字符串*/
		if (res != NO_ERROR)	/*搜索路径*/
			return res;
		if (!(filmt[i]->attr.attr & FILE_ATTR_DIREC))	/*搜索到的是文件*/
		{
			DecFildLn(i);
			return ERROR_FS_FILE_NOT_DIR;
		}
	}
	else	/*在当前目录创建*/
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

/*查找空文件句柄*/
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

/*创建文件,返回句柄*/
long sys_creat(BYTE *path)
{
	DWORD i, j;
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	long res;

	if (FindFh(fh, &j) != NO_ERROR)	/*打开文件已满*/
		return ERROR_FS_HAVENO_FH;
	LOCK(fs_l);
	res = NewFile(path, 0, TRUE, &i);
	ULOCK(fs_l);
	if (res != NO_ERROR)	/*创建文件项错误*/
		return res;
	fh += j;
	fh->fid = i;
	fh->seek = 0;	/*初始化读写指针和当前簇号*/
	return j;
}

/*打开文件,返回句柄*/
long sys_open(BYTE *path, BOOL isWrite)
{
	DWORD i, j;
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (FindFh(fh, &j) != NO_ERROR)	/*打开文件已满*/
		return ERROR_FS_HAVENO_FH;
	LOCK(fs_l);
	res = SetFildLn(path, isWrite, &i);
	if (res != NO_ERROR)	/*搜索路径*/
	{
		ULOCK(fs_l);
		return res;
	}
	CurFd = filmt[i];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)	/*搜索到的是目录*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_DIR_NOT_FILE;
	}
	if (isWrite && CurFd->attr.attr & FILE_ATTR_RDONLY)	/*想要写只读文件*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_WRITE_RDONLY;
	}
	ULOCK(fs_l);
	fh += j;
	fh->fid = i;
	fh->seek = 0;	/*初始化读写指针和当前簇号*/
	return j;
}

/*关闭文件*/
long sys_close(DWORD fhi)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_ATTR fa;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*检查句柄错误*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*空句柄*/
	fa.name[0] = 0;
	fa.ModifyTime = fa.CreateTime = INVALID;
	fa.AccessTime = clock;
	fa.attr = INVALID;
	LOCK(fs_l);
	CurFd = filmt[fh->fid];
	if (CurFd->flag & FILE_FLAG_WRITE)	/*打开写的文件要更新修改时间*/
		fa.ModifyTime = fa.AccessTime;
	res = fsuit[part[CurFd->PartID]->FsID]->SetFile(CurFd, &fa);
	DecFildLn(fh->fid);
	ULOCK(fs_l);
	fh->fid = 0;
	return res;
}

/*读文件字节,返回读出字节数*/
long sys_read(DWORD fhi, BYTE *buf, DWORD siz)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*检查句柄错误*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*空句柄*/
	CurFd = filmt[fh->fid];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*是目录句柄*/
	if (siz == 0)
		return 0;
	if (fh->seek + siz > CurFd->attr.size)
		siz = CurFd->attr.size - fh->seek;	/*超长,修正读长度*/
	res = fsuit[part[CurFd->PartID]->FsID]->RwFile(CurFd, FALSE, fh->seek, siz, buf);	/*读文件*/
	if (res != NO_ERROR)
		return res;
	fh->seek += siz;	/*修改读写指针*/
	return siz;
}

/*写文件字节,返回写入字节数*/
long sys_write(DWORD fhi, BYTE *buf, DWORD siz)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	FSUI *CurFsui;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*检查句柄错误*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*空句柄*/
	CurFd = filmt[fh->fid];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*是目录句柄*/
	if (!(CurFd->flag & FILE_FLAG_WRITE))
		return ERROR_FS_FILE_READED;	/*尝试写打开读的文件*/
	if (siz == 0)
		return 0;
	CurFsui = fsuit[part[CurFd->PartID]->FsID];
	if (fh->seek + siz > CurFd->attr.size)	/*超长,增加文件字节数*/
	{
		res = CurFsui->SetSize(CurFd, fh->seek + siz);
		if (res != NO_ERROR)
			return res;
	}
	res = CurFsui->RwFile(CurFd, TRUE, fh->seek, siz, buf);	/*写文件*/
	if (res != NO_ERROR)
		return res;
	fh->seek += siz;	/*修改读写指针*/
	return siz;
}

/*设置读写指针*/
long sys_lseek(DWORD fhi, long long off, DWORD from)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*检查句柄错误*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*空句柄*/
	if (filmt[fh->fid]->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*是目录句柄*/
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

/*设置文件大小*/
long sys_setsize(DWORD fhi, QWORD siz)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*检查句柄错误*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*空句柄*/
	CurFd = filmt[fh->fid];
	if (CurFd->attr.attr & FILE_ATTR_DIREC)
		return ERROR_FS_DIR_NOT_FILE;	/*是目录句柄*/
	if (!(CurFd->flag & FILE_FLAG_WRITE))
		return ERROR_FS_FILE_READED;	/*尝试写打开读的文件*/
	res = fsuit[part[CurFd->PartID]->FsID]->SetSize(CurFd, siz);
	if (res != NO_ERROR)
		return res;
	fh->seek = 0;
	return NO_ERROR;
}

/*打开目录,返回句柄*/
long sys_opendir(BYTE *path)
{
	DWORD i, j;
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	long res;

	if (FindFh(fh, &j) != NO_ERROR)	/*打开文件已满*/
		return ERROR_FS_HAVENO_FH;
	LOCK(fs_l);
	res = SetFildLn(path, FALSE, &i);
	if (res != NO_ERROR)	/*搜索路径*/
	{
		ULOCK(fs_l);
		return res;
	}
	if (!(filmt[i]->attr.attr & FILE_ATTR_DIREC))	/*搜索到的是文件*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_FILE_NOT_DIR;
	}
	ULOCK(fs_l);
	fh += j;
	fh->fid = i;
	fh->seek = 0;	/*初始化读写指针*/
	return j;
}

/*读目录中的目录项名*/
long sys_readdir(DWORD fhi, FILE_ATTR *fa)
{
	FILE_HANDLE *fh = pmt[pid].ts->fht;
	FILE_DESC *CurFd;
	long res;

	if (fhi >= FhtLen)
		return ERROR_FS_WRONG_FH;	/*检查句柄错误*/
	fh += fhi;
	if (fh->fid == 0)
		return ERROR_FS_WRONG_FH;	/*空句柄*/
	CurFd = filmt[fh->fid];
	if (!(CurFd->attr.attr & FILE_ATTR_DIREC))
		return ERROR_FS_FILE_NOT_DIR;	/*是文件句柄*/
	res = fsuit[part[CurFd->PartID]->FsID]->ReadDir(CurFd, &fh->seek, fa);	/*读目录*/
	if (res != NO_ERROR)
		return res;
	if (fh->seek == 0)
		return ERROR_FS_END_OF_FILE;
	return NO_ERROR;
}

/*转换当前目录*/
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
	if (!(filmt[i]->attr.attr & FILE_ATTR_DIREC))	/*搜索到的是文件*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_FILE_NOT_DIR;
	}
	DecFildLn(pmt[pid].ts->CurFID);	/*减少原当前目录*/
	pmt[pid].ts->CurFID = i;
	ULOCK(fs_l);
	return NO_ERROR;
}

/*创建目录*/
long sys_mkdir(BYTE *path)
{
	DWORD i;
	long res;

	ULOCK(fs_l);
	res = NewFile(path, FILE_ATTR_DIREC, FALSE, &i);
	if (res != NO_ERROR)	/*创建文件项错误*/
	{
		ULOCK(fs_l);
		return res;
	}
	DecFildLn(i);
	ULOCK(fs_l);
	return NO_ERROR;
}

/*删除文件或空目录*/
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
	if (CurFd->attr.attr & FILE_ATTR_RDONLY)	/*只读文件不能删除*/
	{
		DecFildLn(i);
		ULOCK(fs_l);
		return ERROR_FS_WRITE_RDONLY;
	}
	CurFsui = fsuit[part[CurFd->PartID]->FsID];
	if (CurFd->attr.size)
	{
		if (CurFd->attr.attr & FILE_ATTR_DIREC)	/*不空的目录不能删除*/
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

/*重命名文件或目录*/
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
		if (namep - fa.name >= FILE_NAME_SIZE)	/*名称超长*/
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

/*取得文件或目录的属性信息*/
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

/*设置文件或目录的属性*/
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
		return ERROR_FS_ATTR_UNMDFY;	/*属性不可修改*/
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

/*设置文件或目录的时间*/
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

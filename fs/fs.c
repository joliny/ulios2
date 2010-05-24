/*	fs.c for ulios file system
	作者：孙亮
	功能：文件系统上层功能，实现分区管理，路径解析和多任务环境下的文件共享互斥访问，使用接口调用具体的文件系统实例
	最后修改日期：2009-06-04
*/

#include "fs.h"

extern long UlifsMntPart(PART_DESC *pd);
extern void UlifsUmntPart(PART_DESC *pd);
extern long UlifsSetPart(PART_DESC *pd, PART_INFO *pi);
extern BOOL UlifsCmpFile(FILE_DESC *fd, const BYTE *path);
extern long UlifsSchFile(FILE_DESC *fd, const BYTE *path);
extern long UlifsNewFile(FILE_DESC *fd, const BYTE *path);
extern long UlifsDelFile(FILE_DESC *fd);
extern long UlifsSetFile(FILE_DESC *fd, FILE_INFO *fi);
extern long UlifsSetSize(FILE_DESC *fd, QWORD siz);
extern long UlifsRwFile(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *avl);
extern long UlifsReadDir(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *avl);
extern void UlifsFreeData(FILE_DESC *fd);

extern long Fat32MntPart(PART_DESC *pd);
extern void Fat32UmntPart(PART_DESC *pd);
extern long Fat32SetPart(PART_DESC *pd, PART_INFO *pi);
extern BOOL Fat32CmpFile(FILE_DESC *fd, const BYTE *path);
extern long Fat32SchFile(FILE_DESC *fd, const BYTE *path);
extern long Fat32NewFile(FILE_DESC *fd, const BYTE *path);
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

THREAD_ID AthdPtid;	/*磁盘服务ID*/
THREAD_ID TimePtid;	/*时间服务ID*/
PART_DESC part[PART_LEN];	/*分区信息表*/
FILE_DESC* filt[FILT_LEN];	/*打开文件指针表*/
FILE_DESC** FstFd;			/*第一个空文件描述符指针*/
FILE_DESC** EndFd;			/*最后非空文件描述符下一项指针*/
DWORD filtl;				/*文件管理表锁*/
PROCRES_DESC* pret[PRET_LEN];	/*进程资源表*/

/*初始化文件系统,如果不成功必须退出*/
long InitFS()
{
	long res;

	if ((res = KRegKnlPort(SRV_FS_PORT)) != NO_ERROR)	/*注册服务端口*/
		return res;
	if ((res = KGetKptThed(SRV_ATHD_PORT, &AthdPtid)) != NO_ERROR)	/*取得磁盘驱动服务线程*/
		return res;
	if ((res = KGetKptThed(SRV_TIME_PORT, &TimePtid)) != NO_ERROR)	/*取得时钟驱动服务线程*/
		return res;
	if ((res = KMapUserAddr(&cache, BLK_SIZ * BMT_LEN + FDAT_SIZ)) != NO_ERROR)	/*申请高速缓冲和自由内存空间*/
		return res;
	fmt->addr = &fmt[2];	/*初始化自由内存管理*/
	fmt->siz = FDAT_SIZ;
	fmt->nxt = &fmt[1];
	fmt[1].addr = cache + BLK_SIZ * BMT_LEN;
	fmt[1].siz = FDAT_SIZ;
	fmt[1].nxt = NULL;
	memset32(&fmt[2], 0, (FMT_LEN - 2) * sizeof(FREE_BLK_DESC) / sizeof(DWORD));
	fmtl = FALSE;
	memset32(bmt, BLKID_MASK, BMT_LEN * sizeof(CACHE_DESC) / sizeof(DWORD));	/*初始化高速缓冲管理*/
	cahl = FALSE;
//	memset32(cache, 0, BLK_SIZ * BMT_LEN / sizeof(DWORD));	/*清空高速缓冲*/
	memset32(part, INVALID, PART_LEN * sizeof(PART_DESC) / sizeof(DWORD));	/*初始化分区管理表*/
	memset32(filt, 0, FILT_LEN * sizeof(FILE_DESC*) / sizeof(DWORD));	/*初始化打开文件管理表*/
	EndFd = FstFd = filt;
	filtl = FALSE;
	memset32(pret, 0, PRET_LEN * sizeof(PROCRES_DESC*) / sizeof(DWORD));	/*初始化进程资源表*/
	return NO_ERROR;
}

/*初始化分区*/
void InitPart()
{
	typedef struct _BOOT_SEC_PART
	{
		BYTE boot;	/*引导指示符活动分区为0x80*/
		BYTE beg[3];/*开始磁头/扇区/柱面*/
		BYTE fsid;	/*文件系统ID*/
		BYTE end[3];/*结束磁头/扇区/柱面*/
		DWORD fst;	/*从磁盘开始到分区扇区数*/
		DWORD cou;	/*总扇区数*/
	}BOOT_SEC_PART;	/*分区表项*/
	typedef struct _BOOT_SEC
	{
		BYTE code[446];	/*引导程序*/
		BOOT_SEC_PART part[4];	/*分区表*/
		WORD aa55;		/*启动标志*/
	}__attribute__((packed)) BOOT_SEC;	/*引导扇区结构*/
	PART_DESC *CurPart;
	DWORD i, j;

	CurPart = part;
	for (i = 0; i < 2; i++)	/*每个硬盘*/
	{
		BOOT_SEC mbr;

		if (RwCache(i, FALSE, 0, 1, &mbr) != NO_ERROR)
			continue;
		if (mbr.aa55 != 0xAA55)	/*bad MBR*/
			continue;
		for (j = 0; j < 4; j++)	/*4个基本分区*/
		{
			BOOT_SEC_PART *TmpPart;

			TmpPart = &mbr.part[j];
			if (TmpPart->cou == 0)
				continue;	/*空分区*/
			if (TmpPart->fsid == 0x05 || TmpPart->fsid == 0x0F)	/*扩展分区*/
			{
				DWORD fst;

				fst = TmpPart->fst;
				for (;;)	/*扩展分区的每个逻辑驱动器*/
				{
					BOOT_SEC ebr;

					if (RwCache(i, FALSE, fst, 1, &ebr) != NO_ERROR)
						break;
					if (ebr.aa55 != 0xAA55)	/*bad EBR*/
						break;
					if (ebr.part[0].cou)	/*不是空扩展分区*/
					{
						CurPart->FsID = ebr.part[0].fsid;
						CurPart->DrvID = i;
						CurPart->SecID = fst + ebr.part[0].fst;
						CurPart->SeCou = ebr.part[0].cou;
						CurPart->data = NULL;
						if (++CurPart >= &part[PART_LEN])
							goto skip;
					}
					if (ebr.part[1].cou == 0)	/*最后一个逻辑驱动器*/
						break;
					fst = TmpPart->fst + ebr.part[1].fst;
				}
			}
			else	/*普通主分区*/
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
	for (CurPart = part; CurPart < &part[PART_LEN]; CurPart++)	/*尝试挂载*/
		if (CurPart->FsID != INVALID)
			for (i = 0; i < FSUIT_LEN; i++)
				if (fsuit[i].MntPart(CurPart) == NO_ERROR)	/*挂载成功*/
				{
					CurPart->FsID = i;
					break;
				}
}

/*关闭文件系统*/
void CloseFS()
{
	PART_DESC *CurPart;

	for (CurPart = part; CurPart < &part[PART_LEN]; CurPart++)	/*卸载分区*/
		if (CurPart->data)
			fsuit[CurPart->FsID].UmntPart(CurPart);
	SaveCache();	/*高速缓存回写*/
	KFreeAddr(cache);
	KUnregKnlPort(SRV_FS_PORT);
}

/*分配空文件描述符ID*/
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

/*释放空文件描述符ID*/
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

/*增加描述符链读计数*/
static void IncFildLn(FILE_DESC *fd)
{
	while (fd)	/*向目录上层逐层增加打开计数*/
	{
		fd->cou++;
		fd = fd->par;
	}
}

/*减少描述符链读计数*/
static void DecFildLn(FILE_DESC *fd)
{
	lock(&filtl);
	while (fd)	/*向目录上层逐层处理节点*/
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

/*查找描述符表中已记录的文件*/
static FILE_DESC *FindFild(FILE_DESC *par, BOOL (*CmpFile)(FILE_DESC *, const BYTE *), const BYTE *path)
{
	FILE_DESC *fd;
	DWORD i;

	lock(&filtl);
	for (i = 0; &filt[i] < EndFd; i++)
		if ((fd = filt[i]) != NULL && fd->par == par && (par ? CmpFile(fd, path) : fd->part == (PART_DESC*)path))	/*查找目录描述符*/
		{
			fd->cou++;
			ulock(&filtl);
			return fd;
		}
	ulock(&filtl);
	return NULL;
}

/*根据路径字串设置文件描述符链*/
static long SetFildLn(PROCRES_DESC *pres, const BYTE *path, BOOL isWrite, FILE_DESC **fd)
{
	PART_DESC *CurPart;
	FILE_DESC *CurFile;
	FSUI *CurFsui;
	BOOL isSch;	/*是否可以搜索filt*/
	long res;

	if (*path == '/')	/*从根目录开始*/
	{
		DWORD i;
		for (i = 0, path++; *path >= '0' && *path <= '9'; path++)	/*取得分区号*/
			i = i * 10 + *path - '0';
		if ((*path != '/' && *path) || *(path - 1) == '/')	/*/0a // /格式,错误*/
			return FS_ERR_PATH_FORMAT;
		if (i >= PART_LEN || (CurPart = &part[i])->data == NULL)	/*分区号错*/
			return FS_ERR_WRONG_PARTID;
		CurFsui = &fsuit[CurPart->FsID];
		if ((CurFile = FindFild(NULL, NULL, (const BYTE*)CurPart)) != NULL)	/*查找根目录的描述符*/
		{
			if (CurFile->flag & FILE_FLAG_WRITE)	/*已经被打开写了*/
			{
				DecFildLn(CurFile);
				return FS_ERR_PATH_WRITTEN;
			}
			isSch = TRUE;
		}
		else	/*没找到,新建根目录描述符*/
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
		while (*path != '/' && *path)	/*移动路径字符串指针*/
			path++;
		if (*path)
			path++;
	}
	else	/*从进程当前目录中取得参数*/
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

		if (*path == '/')	/*//格式,错误*/
		{
			DecFildLn(CurFile);
			return FS_ERR_PATH_FORMAT;
		}
		if (*path == '.')
		{
			if (*(path + 1) == '/' || *(path + 1) == 0)	/*单点进入当前目录*/
				goto FoundSub;
			if (*(path + 1) == '.' && (*(path + 2) == '/' || *(path + 2) == 0))	/*双点进入上层目录*/
			{
				if ((TmpFile = CurFile->par) != NULL)	/*不是根目录*/
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
		if (isSch)	/*可以从已有的节点中搜索*/
		{
			if ((TmpFile = FindFild(CurFile, CurFsui->CmpFile, path)) != NULL)	/*查找子目录描述符*/
			{
				if (TmpFile->flag & FILE_FLAG_WRITE)	/*已经被打开写了*/
				{
					DecFildLn(TmpFile);
					return FS_ERR_PATH_WRITTEN;
				}
				CurFile = TmpFile;
				goto FoundSub;
			}
			isSch = FALSE;	/*没有找到*/
		}
		/*没找到,新建子目录描述符*/
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
		while (*path != '/' && *path)	/*移动路径字符串指针*/
			path++;
		if (*path)
		{
			if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*不是目录，无法继续打开了*/
			{
				DecFildLn(CurFile);
				return FS_ERR_PATH_NOT_FOUND;
			}
			path++;
		}
	}
	if (isWrite)	/*路径处理完成,以写方式打开么*/
	{
		if (CurFile->cou > 1)	/*发现有不止一个访问者*/
		{
			DecFildLn(CurFile);
			return FS_ERR_PATH_READED;
		}
		CurFile->flag |= FILE_FLAG_WRITE;	/*设置写标志*/
	}
	*fd = CurFile;
	return NO_ERROR;
}

/*创建空文件描述符*/
static long CreateFild(PROCRES_DESC *pres, const BYTE *path, DWORD attr, FILE_DESC **fd)
{
	PART_DESC *CurPart;
	FILE_DESC *CurFile, *TmpFile;
	FSUI *CurFsui;
	const BYTE *name;
	BYTE DirPath[MAX_PATH];
	long res;

	name = path;	/*分离路径和名称*/
	while (*name)
		name++;
	while (name > path && *name != '/')
		name--;
	if (name == path && *name == '/')	/*/name格式,路径格式错误*/
		return FS_ERR_PATH_FORMAT;
	if (*(name + 1) == 0)	/*/path/格式,没有指定名称*/
		return FS_ERR_PATH_FORMAT;
	if (*(name + 1) == '.')
	{
		if (*(name + 2) == 0)	/*/path/.格式,名称错误*/
			return FS_ERR_PATH_FORMAT;
		if (*(name + 2) == '.' && *(name + 3) == 0)	/*/path/..格式,名称错误*/
			return FS_ERR_PATH_FORMAT;
	}
	memcpy8(DirPath, path, name - path);	/*复制路径*/
	DirPath[name - path] = 0;
	if ((res = SetFildLn(pres, DirPath, FALSE, &CurFile)) != NO_ERROR)	/*检查所在目录是否存在*/
		return res;
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*搜索到的是文件*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_DIR;
	}
	CurPart = CurFile->part;
	CurFsui = &fsuit[CurPart->FsID];
	name++;
	if ((TmpFile = (FILE_DESC*)malloc(sizeof(FILE_DESC))) == NULL)
	{
		DecFildLn(CurFile);
		return FS_ERR_HAVENO_MEMORY;
	}
	TmpFile->part = CurPart;
	TmpFile->flag = FILE_FLAG_WRITE;
	TmpFile->cou = 1;
	TmpFile->par = CurFile;
	if (TMCurSecond(TimePtid, &TmpFile->file.CreateTime) != NO_ERROR)	/*设置为当前时间*/
		TmpFile->file.CreateTime = INVALID;
	TmpFile->file.AccessTime = TmpFile->file.ModifyTime = TmpFile->file.CreateTime;
	TmpFile->file.attr = attr;
	TmpFile->data = NULL;
	if ((res = CurFsui->NewFile(TmpFile, name)) != NO_ERROR)	/*创建文件描述符*/
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

/*查找空文件句柄*/
static FILE_HANDLE *FindFh(FILE_HANDLE *fht)
{
	FILE_HANDLE *fh;

	for (fh = fht; fh < &fht[FHT_LEN]; fh++)
		if (fh->fd == NULL)
			return fh;
	return NULL;
}

/*取得可执行文件信息*/
long GetExec(PROCRES_DESC *pres, PROCRES_DESC *par, DWORD fi, DWORD *exec)
{
	typedef struct
	{
		DWORD ei_mag;		// 文件标识
		BYTE ei_class;		// 文件类
		BYTE ei_data;		// 数据编码
		BYTE ei_version;	// 文件版本
		BYTE ei_pad;		// 补齐字节开始处
		BYTE ei_res[8];		// 保留
		WORD e_type;		// 目标文件类型
		WORD e_machine;		// 文件的目标体系结构类型
		DWORD e_version;	// 目标文件版本
		DWORD e_entry;		// 程序入口的虚拟地址
		DWORD e_phoff;		// 程序头部表格的偏移量
		DWORD e_shoff;		// 节区头部表格的偏移量
		DWORD e_flags;		// 保存与文件相关的特定于处理器的标志
		WORD e_ehsize;		// ELF头部的大小
		WORD e_phentsize;	// 程序头部表格的表项大小
		WORD e_phnum;		// 程序头部表格的表项数目
		WORD e_shentsize;	// 节区头部表格的表项大小
		WORD e_shnum;		// 节区头部表格的表项数目
		WORD e_shstrndx;	// 节区头部表格中与节区名称字符串表相关的表项的索引
	}ELF32_EHDR;	// ELF头结构
	typedef struct
	{
		DWORD p_type;		// 此数组元素描述的段的类型
		DWORD p_offset;		// 此成员给出从文件头到该段第一个字节的偏移
		DWORD p_vaddr;		// 此成员给出段的第一个字节将被放到内存中的虚拟地址
		DWORD p_paddr;		// 此成员仅用于与物理地址相关的系统中
		DWORD p_filesz;		// 此成员给出段在文件映像中所占的字节数
		DWORD p_memsz;		// 此成员给出段在内存映像中占用的字节数
		DWORD p_flags;		// 此成员给出与段相关的标志
		DWORD p_align;		// 此成员给出段在文件中和内存中如何对齐
	}ELF32_PHDR;	// 程序头结构
#define EIM_ELF	0x464C457F	// 文件标示
#define EV_CURRENT	1		// 当前版本
#define EIC_32		1		// 32位目标
#define ET_EXEC		2		// 可执行文件
#define EM_386		3		// Intel 80386
#define EM_486		6		// Intel 80486
#define PT_LOAD		1		// 可加载的段
#define PF_X		1		// 可执行
#define PF_W		2		// 可写

	FILE_DESC *CurFile;
	FSUI *CurFsui;
	long res;
	DWORD seek, avl;
	ELF32_EHDR ehdr;
	ELF32_PHDR phdr;

	if (fi >= FILT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*检查句柄错误*/
	lock(&filtl);
	if (filt[fi] == NULL)
	{
		ulock(&filtl);
		return FS_ERR_WRONG_HANDLE;	/*空句柄*/
	}
	CurFile = filt[fi];
	if (CurFile->flag & FILE_FLAG_WRITE)
	{
		ulock(&filtl);
		return FS_ERR_PATH_WRITTEN;	/*尝试执行打开写的文件*/
	}
	if (CurFile->file.attr & FILE_ATTR_DIREC)
	{
		ulock(&filtl);
		return FS_ERR_PATH_NOT_FILE;	/*是目录句柄*/
	}
	if (CurFile->file.size == 0)
	{
		ulock(&filtl);
		return FS_ERR_WRONG_ARGS;	/*非法文件*/
	}
	if (par->CurDir == NULL)
	{
		ulock(&filtl);
		return FS_ERR_WRONG_CURDIR;	/*没有设置当前目录*/
	}
	pres->CurDir = par->CurDir;
	IncFildLn(pres->CurDir);	/*设置当前目录*/
	for (fi = 0; fi < PRET_LEN; fi++)	/*查找副本进程号*/
		if (pret[fi] && pret[fi]->exec == CurFile)
		{
			pres->exec = CurFile;
			memcpy32(&pres->CodeOff, &pret[fi]->CodeOff, 7);
			ulock(&filtl);
			exec[0] = fi;
			memcpy32(&exec[1], &pres->CodeOff, 7);
			return NO_ERROR;
		}
	pres->exec = CurFile;
	ulock(&filtl);
	exec[0] = 0xFFFF;
	CurFsui = &fsuit[CurFile->part->FsID];	/*开始取得ELF文件信息*/
	if ((res = CurFsui->RwFile(CurFile, FALSE, 0, sizeof(ELF32_EHDR), &ehdr, NULL)) != NO_ERROR)
	{
		DecFildLn(CurFile);
		DecFildLn(pres->CurDir);
		return res;
	}
	if (ehdr.ei_mag != EIM_ELF ||
		ehdr.ei_class != EIC_32 ||
		ehdr.ei_version != EV_CURRENT ||
		ehdr.e_type != ET_EXEC ||
		(ehdr.e_machine != EM_386 && ehdr.e_machine != EM_486) ||
		ehdr.e_version != EV_CURRENT)	/*ELF格式检查*/
	{
		pres->CodeOff = 0x8048000;	/*非ELF格式的按BIN处理*/
		pres->CodeEnd = 0x8048000 + (DWORD)CurFile->file.size;
		pres->CodeSeek = 0;
		pres->DataOff = 0;
		pres->DataEnd = 0;
		pres->DataSeek = 0;
		pres->entry = 0x8048000;
	}
	else
	{
		pres->entry = ehdr.e_entry;
		seek = ehdr.e_phoff;
		avl = 0;
		for (fi = 0; fi < ehdr.e_phnum; fi++)	/*读取程序头部表*/
		{
			if ((res = CurFsui->RwFile(CurFile, FALSE, seek, ehdr.e_phentsize, &phdr, &avl)) != NO_ERROR)
			{
				DecFildLn(CurFile);
				DecFildLn(pres->CurDir);
				return res;
			}
			if (phdr.p_type == PT_LOAD)
			{
				if (phdr.p_flags & PF_X)	/*代码段*/
				{
					pres->CodeOff = phdr.p_vaddr;
					pres->CodeEnd = phdr.p_vaddr + phdr.p_filesz;
					pres->CodeSeek = phdr.p_offset;
				}
				else if (phdr.p_flags & PF_W)	/*数据段*/
				{
					pres->DataOff = phdr.p_vaddr;
					pres->DataEnd = phdr.p_vaddr + phdr.p_filesz;
					pres->DataSeek = phdr.p_offset;
				}
			}
			seek += ehdr.e_phentsize;
		}
	}
	memcpy32(&exec[1], &pres->CodeOff, 7);
	return NO_ERROR;
}

/*读取可执行文件页*/
long ReadPage(PROCRES_DESC *pres, void *buf, DWORD siz, DWORD seek)
{
	FILE_DESC *CurFile;
	long res;

	CurFile = pres->exec;
	if (seek >= CurFile->file.size || seek + siz > CurFile->file.size)	/*超长,读取页有误*/
		return FS_ERR_WRONG_ARGS;
	if ((res = fsuit[CurFile->part->FsID].RwFile(CurFile, FALSE, seek, siz, buf, NULL)) != NO_ERROR)	/*读文件*/
		return res;
	return NO_ERROR;
}

/*进程退出*/
long ProcExt(PROCRES_DESC *pres)
{
	DWORD i;

	for (i = 0; i < FHT_LEN; i++)	/*关闭所有已打开的文件*/
		DecFildLn(pres->fht[i].fd);
	DecFildLn(pres->exec);
	DecFildLn(pres->CurDir);
	return NO_ERROR;
}

/*取得可执行文件ID*/
long GetExid(PROCRES_DESC *pres, const BYTE *path, DWORD *fi)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*搜索路径*/
		return res;
	if (CurFile->file.attr & FILE_ATTR_DIREC)	/*搜索到的是目录*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_FILE;
	}
	*fi = CurFile->id;
	return NO_ERROR;
}

/*枚举分区*/
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

/*取得分区信息*/
long GetPart(PROCRES_DESC *pres, DWORD pid, PART_INFO *pi)
{
	if (pid >= PART_LEN || part[pid].data == NULL)
		return FS_ERR_WRONG_PARTID;
	memcpy32(pi, &part[pid].part, sizeof(PART_INFO) / sizeof(DWORD));
	memcpy32((void*)pi + sizeof(PART_INFO), fsuit[part[pid].FsID].name, 2);
	return NO_ERROR;
}

/*创建文件*/
long creat(PROCRES_DESC *pres, const BYTE *path, DWORD *fhi)
{
	FILE_HANDLE *fh;
	long res;

	if ((fh = FindFh(pres->fht)) == NULL)	/*打开文件已满*/
		return FS_ERR_HAVENO_HANDLE;
	if ((res = CreateFild(pres, path, 0, &fh->fd)) != NO_ERROR)	/*创建文件项错误*/
		return res;
	fh->seek = fh->avl = 0;	/*初始化自定值和读写指针*/
	*fhi = fh - pres->fht;
	return NO_ERROR;
}

/*打开文件*/
long open(PROCRES_DESC *pres, const BYTE *path, BOOL isWrite, DWORD *fhi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if ((fh = FindFh(pres->fht)) == NULL)	/*打开文件已满*/
		return FS_ERR_HAVENO_HANDLE;
	if ((res = SetFildLn(pres, path, isWrite, &CurFile)) != NO_ERROR)	/*搜索路径*/
		return res;
	if (CurFile->file.attr & FILE_ATTR_DIREC)	/*搜索到的是目录*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_FILE;
	}
	if (isWrite && CurFile->file.attr & FILE_ATTR_RDONLY)	/*想要写只读文件*/
	{
		DecFildLn(CurFile);
		return FS_ERR_ATTR_RDONLY;
	}
	fh->fd = CurFile;
	fh->seek = fh->avl = 0;	/*初始化自定值和读写指针*/
	*fhi = fh - pres->fht;
	return NO_ERROR;
}

/*关闭文件*/
long close(PROCRES_DESC *pres, DWORD fhi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	FILE_INFO fi;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*检查句柄错误*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*空句柄*/
	fi.name[0] = 0;
	fi.ModifyTime = fi.CreateTime = INVALID;
	if (TMCurSecond(TimePtid, &fi.AccessTime) != NO_ERROR)	/*设置为当前时间*/
		fi.AccessTime = INVALID;
	fi.attr = INVALID;
	if (CurFile->flag & FILE_FLAG_WRITE)	/*打开写的文件要更新修改时间*/
		fi.ModifyTime = fi.AccessTime;
	res = fsuit[CurFile->part->FsID].SetFile(CurFile, &fi);
	DecFildLn(CurFile);
	fh->fd = NULL;
	return res;
}

/*读取文件*/
long read(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*检查句柄错误*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*空句柄*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*是目录句柄*/
	if (fh->seek >= CurFile->file.size)
		*siz = 0;
	if (*siz == 0)
		return NO_ERROR;
	if (fh->seek + *siz > CurFile->file.size)
		*siz = CurFile->file.size - fh->seek;	/*超长,修正读长度*/
	if ((res = fsuit[CurFile->part->FsID].RwFile(CurFile, FALSE, fh->seek, *siz, buf, &fh->avl)) != NO_ERROR)	/*读文件*/
		return res;
	fh->seek += *siz;	/*修改读写指针*/
	return NO_ERROR;
}

/*写入文件*/
long write(PROCRES_DESC *pres, DWORD fhi, void *buf, DWORD *siz)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	FSUI *CurFsui;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*检查句柄错误*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*空句柄*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*是目录句柄*/
	if (!(CurFile->flag & FILE_FLAG_WRITE))
		return FS_ERR_PATH_READED;	/*尝试写打开读的文件*/
	if (*siz == 0)
		return NO_ERROR;
	CurFsui = &fsuit[CurFile->part->FsID];
	if (fh->seek + *siz > CurFile->file.size)	/*超长,增加文件字节数*/
		if ((res = CurFsui->SetSize(CurFile, fh->seek + *siz)) != NO_ERROR)
			return res;
	if ((res = CurFsui->RwFile(CurFile, TRUE, fh->seek, *siz, buf, &fh->avl)) != NO_ERROR)	/*写文件*/
		return res;
	fh->seek += *siz;	/*修改读写指针*/
	return NO_ERROR;
}

/*设置读写指针*/
long seek(PROCRES_DESC *pres, DWORD fhi, SQWORD seek, DWORD from)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*检查句柄错误*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*空句柄*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*是目录句柄*/
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

/*设置文件大小*/
long SetSize(PROCRES_DESC *pres, DWORD fhi, QWORD siz)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*检查句柄错误*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*空句柄*/
	if (CurFile->file.attr & FILE_ATTR_DIREC)
		return FS_ERR_PATH_NOT_FILE;	/*是目录句柄*/
	if (!(CurFile->flag & FILE_FLAG_WRITE))
		return FS_ERR_PATH_READED;	/*尝试写打开读的文件*/
	if ((res = fsuit[CurFile->part->FsID].SetSize(CurFile, siz)) != NO_ERROR)
		return res;
	return NO_ERROR;
}

/*打开目录*/
long OpenDir(PROCRES_DESC *pres, const BYTE *path, DWORD *fhi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if ((fh = FindFh(pres->fht)) == NULL)	/*打开文件已满*/
		return FS_ERR_HAVENO_HANDLE;
	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*搜索路径*/
		return res;
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*搜索到的是文件*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_DIR;
	}
	fh->fd = CurFile;
	fh->seek = fh->avl = 0;	/*初始化自定值和读写指针*/
	*fhi = fh - pres->fht;
	return NO_ERROR;
}

/*读取目录*/
long ReadDir(PROCRES_DESC *pres, DWORD fhi, FILE_INFO *fi)
{
	FILE_HANDLE *fh;
	FILE_DESC *CurFile;
	long res;

	if (fhi >= FHT_LEN)
		return FS_ERR_WRONG_HANDLE;	/*检查句柄错误*/
	fh = &pres->fht[fhi];
	if ((CurFile = fh->fd) == NULL)
		return FS_ERR_WRONG_HANDLE;	/*空句柄*/
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))
		return FS_ERR_PATH_NOT_DIR;	/*是文件句柄*/
	if ((res = fsuit[CurFile->part->FsID].ReadDir(CurFile, &fh->seek, fi, &fh->avl)) != NO_ERROR)	/*读目录*/
		return res;
	return NO_ERROR;
}

/*切换当前目录*/
long ChDir(PROCRES_DESC *pres, const BYTE *path)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*搜索路径*/
		return res;
	if (!(CurFile->file.attr & FILE_ATTR_DIREC))	/*搜索到的是文件*/
	{
		DecFildLn(CurFile);
		return FS_ERR_PATH_NOT_DIR;
	}
	DecFildLn(pres->CurDir);	/*处理原当前目录*/
	pres->CurDir = CurFile;
	return NO_ERROR;
}

/*创建目录*/
long MkDir(PROCRES_DESC *pres, const BYTE *path)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = CreateFild(pres, path, FILE_ATTR_DIREC, &CurFile)) != NO_ERROR)	/*创建文件项错误*/
		return res;
	DecFildLn(CurFile);
	return NO_ERROR;
}

/*删除文件或空目录*/
long remove(PROCRES_DESC *pres, const BYTE *path)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*搜索路径*/
		return res;
	if (CurFile->file.attr & (FILE_ATTR_RDONLY || FILE_ATTR_SYSTEM))	/*只读或系统文件不能删除*/
	{
		DecFildLn(CurFile);
		return FS_ERR_ATTR_RDONLY;
	}
	res = fsuit[CurFile->part->FsID].DelFile(CurFile);
	DecFildLn(CurFile);
	return res;
}

/*重命名文件或目录*/
long rename(PROCRES_DESC *pres, const BYTE *path, const BYTE *name)
{
	PART_DESC *CurPart;
	FILE_DESC *CurFile, TmpFile;
	FSUI *CurFsui;
	BYTE *namep;
	long res;

	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*检查所在目录是否存在*/
		return res;
	CurPart = CurFile->part;
	CurFsui = &fsuit[CurPart->FsID];
	TmpFile.part = CurPart;	/*检查文件是否存在*/
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
		if (*name == '/')	/*格式错误*/
		{
			if (TmpFile.data)
				CurFsui->FreeData(&TmpFile);
			DecFildLn(CurFile);
			return FS_ERR_PATH_FORMAT;
		}
		*namep++ = *name++;
		if (namep - TmpFile.file.name >= FILE_NAME_SIZE)	/*名称超长*/
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
	res = CurFsui->SetFile(CurFile, &TmpFile.file);	/*设置文件描述符*/
	if (TmpFile.data)
		CurFsui->FreeData(&TmpFile);
	DecFildLn(CurFile);
	return res;
}

/*取得文件或目录的属性信息*/
long GetAttr(PROCRES_DESC *pres, const BYTE *path, FILE_INFO *fi)
{
	FILE_DESC *CurFile;
	long res;

	if ((res = SetFildLn(pres, path, FALSE, &CurFile)) != NO_ERROR)	/*搜索路径*/
		return res;
	memcpy32(fi, &CurFile->file, sizeof(FILE_INFO) / sizeof(DWORD));
	DecFildLn(CurFile);
	return NO_ERROR;
}

/*设置文件或目录的属性*/
long SetAttr(PROCRES_DESC *pres, const BYTE *path, DWORD attr)
{
	FILE_DESC *CurFile;
	FILE_INFO fi;
	long res;

	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*搜索路径*/
		return res;
	fi.name[0] = 0;
	fi.AccessTime = fi.ModifyTime = fi.CreateTime = INVALID;
	fi.attr = CurFile->file.attr;
	if (fi.attr & FILE_ATTR_UNMDFY)
	{
		DecFildLn(CurFile);
		return FS_ERR_ATTR_UNMDFY;	/*属性不可修改*/
	}
	if (fi.attr & FILE_ATTR_DIREC)
		attr |= FILE_ATTR_DIREC;
	else
		attr &= (~FILE_ATTR_DIREC);
	fi.attr = attr;
	res = fsuit[CurFile->part->FsID].SetFile(CurFile, &fi);
	DecFildLn(CurFile);
	return res;
}

/*设置文件或目录的时间*/
long SetTime(PROCRES_DESC *pres, const BYTE *path, DWORD time, DWORD cma)
{
	FILE_DESC *CurFile;
	FILE_INFO fi;
	long res;

	if (!(cma & (FS_SETIM_CREATE | FS_SETIM_MODIFY | FS_SETIM_ACCESS)))
		return FS_ERR_WRONG_ARGS;
	if ((res = SetFildLn(pres, path, TRUE, &CurFile)) != NO_ERROR)	/*搜索路径*/
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

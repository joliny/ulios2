/*	fmtboot.c for ulios tools
	作者：孙亮
	功能：格式化分区成为可引导ulios分区
	最后修改日期：2009-12-11
	备注：使用Turbo C TCC编译器编译成DOS平台EXE文件
*/

#include <stdio.h>

typedef unsigned short	BOOL;
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

#define FAR2LINE(addr)	((WORD)(addr) + (((addr) & 0xFFFF0000) >> 12))
#define LINE2FAR(addr)	((WORD)(addr) | (((addr) & 0xFFFF0000) << 12))

typedef struct _DAP
{
	BYTE	PacketSize;	/*数据包尺寸=16*/
	BYTE	Reserved;	/*0*/
	WORD	BlockCount;	/*要传输的扇区数*/
	DWORD	BufferAddr;	/*传输缓冲地址(segment:offset)*/
	DWORD	BlockAddr[2];/*磁盘起始绝对块地址*/
}DAP;	/*磁盘地址数据包*/

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
}BOOT_SEC;	/*引导扇区结构*/

typedef struct _PART_INF
{
	BYTE fsid;	/*文件系统ID*/
	BYTE boot;	/*启动标示*/
	BYTE drv;	/*磁盘号*/
	BYTE lev;	/*层级*/
	DWORD fst;	/*起始扇区号*/
	DWORD cou;	/*扇区数*/
}PART_INF;	/*分区信息*/

typedef struct _BOOTSEC_FAT32
{
	BYTE	jmpi[3];/*跳转指令*/
	BYTE	BS_OEMName[8];	/*OEM ID(tian&uli2k_X)*/
	WORD	bps;	/*每扇区字节数*/
	BYTE	spc;	/*每簇扇区数*/
	WORD	res;	/*保留扇区数*/
	BYTE	nf;		/*FAT数*/
	WORD	nd;		/*根目录项数*/
	WORD	sms;	/*小扇区数(FAT32不用)*/
	BYTE	md;		/*媒体描述符*/
	WORD	spf16;	/*每FAT扇区数(FAT32不用)*/
	WORD	spt;	/*每道扇区数*/
	WORD	nh;		/*磁头数*/
	DWORD	hs;		/*隐藏扇区数*/
	DWORD	ls;		/*总扇区数*/
	DWORD	spf;	/*每FAT扇区数(FAT32专用)*/
	WORD	ef;		/*扩展标志(FAT32专用)*/
	WORD	fv;		/*文件系统版本(FAT32专用)*/
	DWORD	rcn;	/*根目录簇号(FAT32专用)*/
	WORD	fsis;	/*文件系统信息扇区号(FAT32专用)*/
	WORD	backup;	/*备份引导扇区(FAT32专用)*/
	DWORD	res1[3];/*保留(FAT32专用)*/

	BYTE	pdn;	/*物理驱动器号*/
	BYTE	res2;	/*保留*/
	BYTE	ebs;	/*扩展引导标签*/
	DWORD	vsn;	/*分区序号*/
	BYTE	vl[11];	/*卷标*/
	BYTE	sid[8];	/*系统ID*/

	WORD	ldroff;	/*ulios载入程序所在扇区偏移*/
	DWORD	secoff;	/*分区在磁盘上的起始扇区偏移*/
	BYTE	BootPath[32];/*启动列表文件路径*/
	BYTE	code[384];	/*引导代码*/
}BOOTSEC_FAT32;	/*FAT32引导扇区数据79*/

typedef struct _BOOTSEC_ULIFS
{
	BYTE	jmpi[4];/*跳转指令*/
	BYTE	BS_OEMName[8];	/*OEM ID(tian&uli2k_X)*/
	BYTE	fsid[4];/*文件系统标志"ULTN",文件系统代码:0x7C*/
	WORD	ver;	/*版本号0,以下为0版本的BPB*/
	WORD	bps;	/*每扇区字节数*/
	WORD	spc;	/*每簇扇区数*/
	WORD	res;	/*保留扇区数,包括引导记录*/
	DWORD	secoff;	/*分区在磁盘上的起始扇区偏移*/
	DWORD	seccou;	/*分区占总扇区数,包括剩余扇区*/
	WORD	spbm;	/*每个位图占扇区数*/
	WORD	cluoff;	/*分区内首簇开始扇区偏移*/
	DWORD	clucou;	/*数据簇数目*/
	BYTE	BootPath[88];/*启动列表文件路径*/
	BYTE	code[384];	/*引导代码*/
}BOOTSEC_ULIFS;	/*ULIFS引导扇区数据24*/

#define NO_ERROR	0
#define ERROR_DISK	1
#define ERROR_FILE	2

/*读写扇区*/
int RwSector(	BYTE	DrvNum,		/*输入：驱动器号*/
				BOOL	isWrite,	/*输入：是否写入*/
				DWORD	BlockAddr,	/*输入：起始扇区号*/
				WORD	BlockCount,	/*输入：扇区数*/
				DWORD	BufferAddr)	/*输出：存放数据线性地址*/
{
	DAP dap;

	dap.PacketSize = 0x10;
	dap.Reserved = 0;
	dap.BlockCount = BlockCount;
	dap.BufferAddr = LINE2FAR(BufferAddr);
	dap.BlockAddr[0] = BlockAddr;
	dap.BlockAddr[1] = 0;

	if (isWrite)
		_AX = 0x4300;
	else
		_AX = 0x4200;
	_DL = DrvNum;
	_SI = (WORD)(&dap);
	asm int 13h;
	asm jc short DiskError;
	return NO_ERROR;
DiskError:
	return ERROR_DISK;
}

/*取得分区信息*/
void ReadPart(PART_INF *part)
{
	BYTE i, j;
	for (i = 0; i < 0x7F; i++)	/*每个硬盘*/
	{
		BOOT_SEC mbr;
		if (RwSector(i + 0x80, 0, 0, 1, FAR2LINE((DWORD)((void far *)&mbr))) != NO_ERROR)
			continue;
		if (mbr.aa55 != 0xAA55)	/*bad MBR*/
			continue;
		for (j = 0; j < 4; j++)	/*4个基本分区*/
		{
			BOOT_SEC_PART *CurPart = &mbr.part[j];
			if (CurPart->cou == 0)
				continue;	/*空分区*/
			if (CurPart->fsid == 0x05 || CurPart->fsid == 0x0F)	/*扩展分区*/
			{
				DWORD fst = CurPart->fst;
				for (;;)	/*扩展分区的每个逻辑驱动器*/
				{
					BOOT_SEC ebr;
					if (RwSector(i + 0x80, 0, fst, 1, FAR2LINE((DWORD)((void far *)&ebr))) != NO_ERROR)
						break;
					if (ebr.aa55 != 0xAA55)	/*bad EBR*/
						break;
					if (ebr.part[0].cou)	/*不是空扩展分区*/
					{
						part->fsid = ebr.part[0].fsid;
						part->boot = ebr.part[0].boot;
						part->drv = i + 0x80;
						part->lev = 1;
						part->fst = fst + ebr.part[0].fst;
						part->cou = ebr.part[0].cou;
						part++;
					}
					if (ebr.part[1].cou == 0)	/*最后一个逻辑驱动器*/
						break;
					fst = CurPart->fst + ebr.part[1].fst;
				}
			}
			else	/*普通主分区*/
			{
				part->fsid = CurPart->fsid;
				part->boot = CurPart->boot;
				part->drv = i + 0x80;
				part->lev = 0;
				part->fst = CurPart->fst;
				part->cou = CurPart->cou;
				part++;
			}
		}
	}
	part->fsid = 0;
}

int Fat32Setup(BYTE drv, DWORD fst)
{
	BOOTSEC_FAT32 dbr, boot;
	BYTE buf[0xE00];
	FILE *f;

	printf("Reading... boot sector\n");
	if (RwSector(drv, 0, fst, 1, FAR2LINE((DWORD)((void far *)&dbr))) != NO_ERROR)
		return ERROR_DISK;

	printf("Reading... f32boot\n");
	if ((f = fopen("F32BOOT", "rb")) == NULL)
		return ERROR_FILE;
	fread(&boot, sizeof(BOOTSEC_FAT32), 1, f);
	fclose(f);

	memcpy(&boot.bps, dbr.bps, 79);
	boot.secoff = fst;

	memset(buf, 0, sizeof(buf));

	printf("Reading... f32ldr\n");
	if ((f = fopen("F32LDR", "rb")) == NULL)
		return ERROR_FILE;
	fread(buf, 0xA00, 1, f);
	fclose(f);

	printf("Reading... setup\n");
	if ((f = fopen("SETUP", "rb")) == NULL)
		return ERROR_FILE;
	fread(&buf[0xA00], 0x400, 1, f);
	fclose(f);
}

void UlifsSetup(BYTE drv, DWORD fst)
{

}

int main()
{
	PART_INF part[32], *partp;
	WORD partn, selp;
	printf("Welcome to ulios file system setup program!\nI will setup ulios boot program to your hard disk.\nWrite f32boot/f32ldr/setup to partition when you select FAT32\nwrite uliboot/ulildr/setup when you select ULIFS.\n\n");
	printf("Checking partition information...");
	ReadPart(part);
	printf("Done\n");
	printf("\tFS_TYPE\tACTIVE\tDRV_ID\tTYPE\tSTART\tCOUNT\n");
	for (partp = part, partn = 0; partp->fsid; partp++)
	{
		printf("%u:\t", ++partn);
		switch (partp->fsid)
		{
		case 0x0B:
			printf("fat32");
			break;
		case 0x7C:
			printf("ulifs");
			break;
		default:
			printf("unknown");
		}
		printf("\t%s\t0x%X\t%s\t%lu\t%lu\n", partp->boot ? "yes" : "no", partp->drv, partp->lev ? "Logical" : "Primary", partp->fst, partp->cou);
	}
	printf("Which partition you want to setup ulios?\n[0:Exit setup, 1 to %u:Select partition]:", partn);
	for (; ; )
	{
		char str[16];
		gets(str);
		selp = atoi(str);
		if (selp == 0)
			return NO_ERROR;
		selp--;
		if (selp >= partn)
		{
			printf("Select 1 to %u:", partn);
			continue;
		}
		if (part[selp].fsid != 0x7C && part[selp].fsid != 0x0B)
		{
			printf("Partition is not supported, Select again:");
			continue;
		}
		partp = &part[selp];
		break;
	}
	switch (partp->fsid)
	{
	case 0x0B:
		Fat32Setup(partp->drv, partp->fst);
		break;
	case 0x7C:
		UlifsSetup(partp->drv, partp->fst);
		break;
	}
	return NO_ERROR;
}

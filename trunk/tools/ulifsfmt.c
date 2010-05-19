/*	ulifsfmt.c for ulios tools
	���ߣ�����
	���ܣ���ʽ��������ΪULIFS����
	����޸����ڣ�2010-03-05
	��ע��ʹ��Turbo C TCC�����������DOSƽ̨COM��EXE�ļ�
*/

#include <stdio.h>
#include <time.h>

typedef unsigned short	BOOL;
typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned long	DWORD;

#define FAR2LINE(addr)	((WORD)(addr) + (((addr) & 0xFFFF0000) >> 12))
#define LINE2FAR(addr)	((WORD)(addr) | (((addr) & 0xFFFF0000) << 12))

typedef struct _DAP
{
	BYTE	PacketSize;	/*���ݰ��ߴ�=16*/
	BYTE	Reserved;	/*0*/
	WORD	BlockCount;	/*Ҫ�����������*/
	DWORD	BufferAddr;	/*���仺���ַ(segment:offset)*/
	DWORD	BlockAddr[2];/*������ʼ���Կ��ַ*/
}DAP;	/*���̵�ַ���ݰ�*/

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
}BOOT_SEC;	/*���������ṹ*/

typedef struct _PART_INF
{
	BYTE fsid;	/*�ļ�ϵͳID*/
	BYTE boot;	/*������ʾ*/
	BYTE drv;	/*���̺�*/
	BYTE lev;	/*�㼶*/
	DWORD fst;	/*��ʼ������*/
	DWORD cou;	/*������*/
	DWORD idsec;/*FSID����������*/
	WORD pid;	/*FSID���ڷ�����ID*/
}PART_INF;	/*������Ϣ*/

typedef struct _ULIFS_BOOTSEC
{
	DWORD	jmpi;	/*��תָ��*/
	BYTE	oem[8];	/*OEM ID(tian&uli2k_X)*/
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
	BYTE	BootPath[88];/*�����б��ļ�·��*/
	BYTE	code[382];	/*��������*/
	WORD	aa55;	/*������־*/
}ULIFS_BOOTSEC;	/*ULIFS������������*/

typedef struct _BLKID
{
	DWORD fst;	/*�״�*/
	DWORD cou;	/*����*/
}BLKID;	/*�������ڵ�*/

#define ULIFS_FILE_NAME_SIZE	80
#define ULIFS_FILE_ATTR_RDONLY	0x00000001	/*ֻ��*/
#define ULIFS_FILE_ATTR_HIDDEN	0x00000002	/*����*/
#define ULIFS_FILE_ATTR_SYSTEM	0x00000004	/*ϵͳ*/
#define ULIFS_FILE_ATTR_LABEL	0x00000008	/*���*/
#define ULIFS_FILE_ATTR_DIREC	0x00000010	/*Ŀ¼(ֻ��)*/
#define ULIFS_FILE_ATTR_ARCH	0x00000020	/*�鵵*/
#define ULIFS_FILE_ATTR_EXEC	0x00000040	/*��ִ��*/
#define ULIFS_FILE_ATTR_UNMDFY	0x80000000	/*���Բ����޸�*/

typedef struct _ULIFS_DIR
{
	BYTE name[ULIFS_FILE_NAME_SIZE];/*�ļ���*/
	DWORD CreateTime;	/*����ʱ��1970-01-01����������*/
	DWORD ModifyTime;	/*�޸�ʱ��*/
	DWORD AccessTime;	/*����ʱ��*/
	DWORD attr;			/*����*/
	DWORD size[2];		/*�ļ��ֽ���,Ŀ¼�ļ����ֽ�����Ч*/
	BLKID idx[3];		/*�����3���Դ���������,����һ���ļ��п����ò���������*/
}ULIFS_DIR;	/*Ŀ¼��ṹ*/

#define FALSE		0
#define TRUE		1

#define NO_ERROR	0
#define ERROR_DISK	1
#define ERROR_FILE	2

/*��д����*/
int RwSector(	BYTE	DrvNum,		/*���룺��������*/
				BOOL	isWrite,	/*���룺�Ƿ�д��*/
				DWORD	BlockAddr,	/*���룺��ʼ������*/
				WORD	BlockCount,	/*���룺������*/
				DWORD	BufferAddr)	/*���������������Ե�ַ*/
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

/*ȡ�÷�����Ϣ*/
void ReadPart(PART_INF *part)
{
	BYTE i, j;
	for (i = 0; i < 0x7F; i++)	/*ÿ��Ӳ��*/
	{
		BOOT_SEC mbr;
		if (RwSector(i + 0x80, FALSE, 0, 1, FAR2LINE((DWORD)((void far *)&mbr))) != NO_ERROR)
			continue;
		if (mbr.aa55 != 0xAA55)	/*bad MBR*/
			continue;
		for (j = 0; j < 4; j++)	/*4����������*/
		{
			BOOT_SEC_PART *CurPart = &mbr.part[j];
			if (CurPart->cou == 0)
				continue;	/*�շ���*/
			if (CurPart->fsid == 0x05 || CurPart->fsid == 0x0F)	/*��չ����*/
			{
				DWORD fst = CurPart->fst;
				for (;;)	/*��չ������ÿ���߼�������*/
				{
					BOOT_SEC ebr;
					if (RwSector(i + 0x80, FALSE, fst, 1, FAR2LINE((DWORD)((void far *)&ebr))) != NO_ERROR)
						break;
					if (ebr.aa55 != 0xAA55)	/*bad EBR*/
						break;
					if (ebr.part[0].cou)	/*���ǿ���չ����*/
					{
						part->fsid = ebr.part[0].fsid;
						part->boot = ebr.part[0].boot;
						part->drv = i + 0x80;
						part->lev = 1;
						part->fst = fst + ebr.part[0].fst;
						part->cou = ebr.part[0].cou;
						part->idsec = fst;
						part->pid = 0;
						part++;
					}
					if (ebr.part[1].cou == 0)	/*���һ���߼�������*/
						break;
					fst = CurPart->fst + ebr.part[1].fst;
				}
			}
			else	/*��ͨ������*/
			{
				part->fsid = CurPart->fsid;
				part->boot = CurPart->boot;
				part->drv = i + 0x80;
				part->lev = 0;
				part->fst = CurPart->fst;
				part->cou = CurPart->cou;
				part->idsec = 0;
				part->pid = j;
				part++;
			}
		}
	}
	part->fsid = 0;
}

int FormatUlifs(PART_INF *part, WORD spc, WORD res, BYTE *label)
{
	WORD i, spbm;
	BYTE *p;
	ULIFS_BOOTSEC dbr;
	ULIFS_DIR dir;
	BYTE buf[512];

	printf("Writing... boot sector");
	memset(&dbr, 0, sizeof(ULIFS_BOOTSEC));
	memcpy(dbr.oem, "TUX soft", 8);
	dbr.fsid = 0x4E544C55;
	dbr.bps = 512;
	dbr.spc = 1 << spc;
	dbr.res = res;
	dbr.secoff = part->fst;
	dbr.seccou = part->cou;
	for (;;)
	{
		dbr.clucou = (dbr.seccou - res - dbr.spbm - dbr.spbm) >> spc;
		spbm = (dbr.clucou + 0xFFF) >> 12;
		if (spbm == dbr.spbm)
			break;
		dbr.spbm = spbm;
	}
	dbr.cluoff = res + spbm + spbm;
	dbr.aa55 = 0xAA55;
	if (RwSector(part->drv, TRUE, dbr.secoff, 1, FAR2LINE((DWORD)((void far *)&dbr))) != NO_ERROR)
		return ERROR_DISK;

	printf("\nWriting... bad sector bmp");
	memset(buf, 0, sizeof(buf));
	for (i = 0; i < spbm; i++)
	{
		if (RwSector(part->drv, TRUE, dbr.secoff + res + i, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
			return ERROR_DISK;
		printf(".");
	}

	printf("\nWriting... used sector bmp");
	buf[0] = 1;
	if (RwSector(part->drv, TRUE, dbr.secoff + res + spbm, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;
	printf(".");
	buf[0] = 0;
	for (i = 1; i < spbm; i++)
	{
		if (RwSector(part->drv, TRUE, dbr.secoff + res + spbm + i, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
			return ERROR_DISK;
		printf(".");
	}

	printf("\nWriting... root dir");
	memset(&dir, 0, sizeof(ULIFS_DIR));
	p = label;
	for (p++; *p; p++)
		if (*p == '/' || (p - label) >= ULIFS_FILE_NAME_SIZE - 1)
			break;
	memcpy(dir.name, label, p - label);
	time((time_t*)&dir.CreateTime);
	dir.AccessTime = dir.ModifyTime = dir.CreateTime;
	dir.attr = ULIFS_FILE_ATTR_LABEL | ULIFS_FILE_ATTR_DIREC;
	dir.size[0] = sizeof(ULIFS_DIR);
	dir.idx[0].cou = 1;
	memcpy(buf, &dir, sizeof(ULIFS_DIR));
	if (RwSector(part->drv, TRUE, dbr.secoff + dbr.cluoff, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;

	printf("\nSetting... ulifs id\n");
	if (RwSector(part->drv, FALSE, part->idsec, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;
	((BOOT_SEC*)buf)->part[part->pid].fsid = 0x7C;
	if (RwSector(part->drv, TRUE, part->idsec, 1, FAR2LINE((DWORD)((void far *)buf))) != NO_ERROR)
		return ERROR_DISK;

	return NO_ERROR;
}

int main()
{
	PART_INF part[32], *partp;
	WORD partn, sel;
	BYTE buf[ULIFS_FILE_NAME_SIZE];
	int res;
	printf("Welcome to ulifs format program!\nWARNING! I will clean up all of the data on partition you selected.\n\n");
	printf("Checking partition information...");
	ReadPart(part);
	printf("Done\n");
	printf("\tFS_TYPE\tACTIVE\tDRV_ID\tTYPE\tSTART\tCOUNT\n");
	for (partp = part, partn = 0; partp->fsid; partp++)
	{
		printf("%u:\t", ++partn);
		switch (partp->fsid)
		{
		case 0x01:
		case 0x0B:
		case 0x0C:
			printf("fat32");
			break;
		case 0x7C:
			printf("ulifs");
			break;
		default:
			printf("0x%02X", partp->fsid);
		}
		printf("\t%s\t0x%X\t%s\t%lu\t%lu\n", partp->boot ? "yes" : "no", partp->drv, partp->lev ? "Logical" : "Primary", partp->fst, partp->cou);
	}
	printf("Which partition you want to format?\n[0:Exit, 1 to %u:Select partition]:", partn);
	for (;;)
	{
		gets(buf);
		sel = atoi(buf);
		if (sel == 0)
			return NO_ERROR;
		if (sel > partn)
		{
			printf("Select 0 to %u:", partn);
			continue;
		}
		partp = &part[--sel];
		break;
	}
	printf("How many bytes per cluster?\n[0:Exit, 1:512 byte, 2:1KB(default), 3:2KB, 4:4KB, 5:8KB, 6:16KB, 7:32KB, 8:64KB]:");
	for (;;)
	{
		gets(buf);
		if (buf[0] == '\0')
			sel = 1;
		else
		{
			sel = atoi(buf);
			if (sel == 0)
				return NO_ERROR;
			if (sel > 8)
			{
				printf("Select 0 to 8:");
				continue;
			}
			sel--;
		}
		break;
	}
	printf("Volume label?\n[78 characters, / for none, ENTER for Exit]:");
	gets(buf + 1);
	if (buf[1] == '\0')
		return NO_ERROR;
	buf[0] = '/';
	res = FormatUlifs(partp, sel, 8, buf);
	switch (res)
	{
	case NO_ERROR:
		printf("OK!\n");
		break;
	case ERROR_DISK:
		printf("Disk Error!\n");
		break;
	case ERROR_FILE:
		printf("File Error!\n");
		break;
	}
	return NO_ERROR;
}

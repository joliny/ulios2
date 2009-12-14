/*	fmtboot.c for ulios tools
	���ߣ�����
	���ܣ���ʽ��������Ϊ������ulios����
	����޸����ڣ�2009-12-11
	��ע��ʹ��Turbo C TCC�����������DOSƽ̨EXE�ļ�
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
}PART_INF;	/*������Ϣ*/

typedef struct _BOOTSEC_FAT32
{
	BYTE	jmpi[3];/*��תָ��*/
	BYTE	BS_OEMName[8];	/*OEM ID(tian&uli2k_X)*/
	WORD	bps;	/*ÿ�����ֽ���*/
	BYTE	spc;	/*ÿ��������*/
	WORD	res;	/*����������*/
	BYTE	nf;		/*FAT��*/
	WORD	nd;		/*��Ŀ¼����*/
	WORD	sms;	/*С������(FAT32����)*/
	BYTE	md;		/*ý��������*/
	WORD	spf16;	/*ÿFAT������(FAT32����)*/
	WORD	spt;	/*ÿ��������*/
	WORD	nh;		/*��ͷ��*/
	DWORD	hs;		/*����������*/
	DWORD	ls;		/*��������*/
	DWORD	spf;	/*ÿFAT������(FAT32ר��)*/
	WORD	ef;		/*��չ��־(FAT32ר��)*/
	WORD	fv;		/*�ļ�ϵͳ�汾(FAT32ר��)*/
	DWORD	rcn;	/*��Ŀ¼�غ�(FAT32ר��)*/
	WORD	fsis;	/*�ļ�ϵͳ��Ϣ������(FAT32ר��)*/
	WORD	backup;	/*������������(FAT32ר��)*/
	DWORD	res1[3];/*����(FAT32ר��)*/

	BYTE	pdn;	/*������������*/
	BYTE	res2;	/*����*/
	BYTE	ebs;	/*��չ������ǩ*/
	DWORD	vsn;	/*�������*/
	BYTE	vl[11];	/*���*/
	BYTE	sid[8];	/*ϵͳID*/

	WORD	ldroff;	/*ulios���������������ƫ��*/
	DWORD	secoff;	/*�����ڴ����ϵ���ʼ����ƫ��*/
	BYTE	BootPath[32];/*�����б��ļ�·��*/
	BYTE	code[384];	/*��������*/
}BOOTSEC_FAT32;	/*FAT32������������79*/

typedef struct _BOOTSEC_ULIFS
{
	BYTE	jmpi[4];/*��תָ��*/
	BYTE	BS_OEMName[8];	/*OEM ID(tian&uli2k_X)*/
	BYTE	fsid[4];/*�ļ�ϵͳ��־"ULTN",�ļ�ϵͳ����:0x7C*/
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
	BYTE	code[384];	/*��������*/
}BOOTSEC_ULIFS;	/*ULIFS������������24*/

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
		if (RwSector(i + 0x80, 0, 0, 1, FAR2LINE((DWORD)((void far *)&mbr))) != NO_ERROR)
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
					if (RwSector(i + 0x80, 0, fst, 1, FAR2LINE((DWORD)((void far *)&ebr))) != NO_ERROR)
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

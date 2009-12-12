/*	fmtboot.c for ulios tools
	���ߣ�����
	���ܣ���ʽ��������Ϊ������ulios����
	����޸����ڣ�2009-12-11
	��ע��ʹ��Turbo C TCC�����������DOSƽ̨EXE�ļ�
*/

#include <stdio.h>

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

typedef struct _BPB_FAT32
{
	BYTE	DRV_num;	/*��������*/
	BYTE	DRV_count;	/*��������*/
	BYTE	DRV_res;	/*����*/
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
}BPB_FAT32;	/*FAT32������¼����*/

typedef struct _BPB_ULIFS
{
	BYTE	DRV_num;	/*��������*/
	BYTE	DRV_count;	/*��������*/
	BYTE	DRV_res[2];	/*����*/
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
}BPB_ULIFS;	/*ULIFS������¼����*/

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

#define NO_ERROR	0
#define ERROR_DISK	1

/*��ȡ����*/
int ReadSector(BYTE DrvNum,		/*���룺��������*/
				DWORD BlockAddr,	/*���룺��ʼ������*/
				WORD BlockCount,	/*���룺������*/
				DWORD BufferAddr)	/*���������������Ե�ַ*/
{
	DAP dap;

	dap.PacketSize = 0x10;
	dap.Reserved = 0;
	dap.BlockCount = BlockCount;
	dap.BufferAddr = LINE2FAR(BufferAddr);
	dap.BlockAddr[0] = BlockAddr;
	dap.BlockAddr[1] = 0;

	_AH = 0x42;
	_DL = DrvNum;
	_SI = (WORD)(&dap);
	asm int 13h;
	asm jc short ReadError;
	return NO_ERROR;
ReadError:
	return ERROR_DISK;
}

/*ȡ�÷�����Ϣ*/
void ReadPart(PART_INF *part)
{
	BYTE i, j;
	BOOT_SEC mbr, ebr;

	for (i = 0; i < 0x80; i++)	/*ÿ��Ӳ��*/
	{
		if (ReadSector(i + 0x80, 0, 1, FAR2LINE((DWORD)((void far *)&mbr))) != NO_ERROR)
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
					if (ReadSector(i + 0x80, fst, 1, FAR2LINE((DWORD)((void far *)&ebr))) != NO_ERROR)
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
					if (ebr.part[1].cou == 0)
						break;	/*���һ���߼�������*/
					fst = CurPart->fst + ebr.part[1].fst;
				}
			}
			else	/*��ͨ��������*/
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

int main()
{
	PART_INF part[32], *partp;
	puts("Welcome to ulios file system setup program!\nI will setup ulios boot program to your hard disk.\nWrite f32boot/f32ldr/setup to partition when you select FAT32\nwrite uliboot/ulildr/setup when you select ULIFS.\n");
	ReadPart(part);
	printf("fsid\tboot\tdrv\tlev\tfst\tcou\n");
	for (partp = part; partp->fsid; partp++)
		printf("%X\t%X\t%X\t%u\t%lu\t%lu\n", partp->fsid, partp->boot, partp->drv, partp->lev, partp->fst, partp->cou);
	return NO_ERROR;
}

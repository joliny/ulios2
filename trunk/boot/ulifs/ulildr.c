/*	ulildr.c for ulios
	���ߣ�����
	���ܣ���ULIFS�ļ�ϵͳ�����������ں�
	����޸����ڣ�2009-10-30
	��ע��ʹ��Turbo C TCC�����������16λCOM�ļ�
*/
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

typedef struct _BPB
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
}BPB;	/*ULIFS������¼����*/

typedef struct _BLKID
{
	DWORD fst;	/*�״�*/
	DWORD cou;	/*����*/
}BLKID;	/*�������ڵ�*/

typedef struct _DIR
{
	BYTE name[80];	/*utf8�ַ���*/
	DWORD CreateTime;	/*����ʱ��1970-01-01����������*/
	DWORD ModifyTime;	/*�޸�ʱ��*/
	DWORD AccessTime;	/*����ʱ��*/
	DWORD attr;		/*����*/
	DWORD len[2];	/*�ļ�����,��FAT32��ͬĿ¼�ļ��ĳ�����Ч*/
	BLKID idx[3];	/*�����3���Դ���������,����һ���ļ��п����ò���������*/
}DIR;	/*ULIFSĿ¼��ṹ*/

#define BUF_COU		0x4000							/*���ݻ�������*/
#define IDX_COU		(BUF_COU / sizeof(BLKID))		/*������������*/

#define SETUP		((void (*)())0x0B00)			/*Setup����λ��*/
#define BIN_ADDR	((DWORD *)0x0080)				/*Bin�������������*/
#define VESA_MODE	((WORD *)0x00FC)				/*����VESAģʽ��*/

#define NO_ERROR	0
#define NOT_FOUND	1

void memcpy(BYTE *dst, BYTE *src, WORD size)
{
	while (size--)
		*dst++ = *src++;
}

WORD strcmp(BYTE *str1, BYTE *str2)
{
	while (*str1 == *str2)
	{
		if (*str1 == 0)
			return 0;
		str1++;
		str2++;
	}
	return 1;
}

WORD atol(BYTE *str)
{
	WORD i;
	BYTE c;

	for (i = 0; (c = *str) >= '0' && c <= '9'; str++)
		i = i * 10 + c - '0';
	return i;
}

void PutChar(BYTE c)
{
	_AL = c;
	_AH = 0x0E;
	_BX = 0x0007;
	asm int 10h;
}

void PutS(BYTE *str)
{
	while (*str)
	{
		_AL = *str;
		_AH = 0x0E;
		_BX = 0x0007;
		asm int 10h;
		str++;
	}
}

/*��ȡ����*/
void ReadSector(BYTE DrvNum,		/*���룺��������*/
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
	return;
ReadError:	/*ע�⣺��չINT13���ܲ��ᴦ��DMA�߽���󣬵��ñ�����ʱ��ע�����*/
	PutChar('!');
	for (;;);
}

/*��ȡ�����ļ�*/
DWORD ReadFile(	BPB *bpb,			/*���룺����BPB*/
				BLKID *idx,			/*�޸ģ���������*/
				DIR *SrcDir,		/*���룺�ļ�Ŀ¼��*/
				DWORD BufferAddr)	/*������������Զ��ַ*/
{
	WORD idxcou, idxi, idxblkcou;	/*ÿ�ζ�ȡ�������ڵ����������ڵ�������ÿ�ζ�ȡ������������*/
	DWORD idxfstblk, brd;	/*���������������Ѷ�ȡ�ֽ���*/

	idxcou = bpb->bps / sizeof(BLKID) * bpb->spc;
	if (idxcou > IDX_COU)	/*���峤��*/
		idxcou = IDX_COU;
	idxi = idxcou - 3;
	idxblkcou = idxcou * sizeof(BLKID) / bpb->bps;
	brd = 0;
	memcpy((BYTE *)(idx + idxi), (BYTE *)(SrcDir->idx), sizeof(BLKID) * 3);	/*����Ŀ¼���е�����*/
	for (;;)
	{
		for (;;)	/*����������*/
		{
			WORD blkcou, i;
			DWORD fstblk;
			if (idxi >= idxcou)	/*��ȡ��һ����*/
			{
				idxfstblk += idxblkcou;
				break;
			}
			else if ((blkcou = idx[idxi].cou) == 0)	/*��ȡ��һ������*/
			{
				idxfstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;
				break;
			}
			blkcou *= bpb->spc;	/*ȡ��Ҫ��ȡ��������*/
			fstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;	/*ȡ��Ҫ��ȡ����������*/
			for (i = 0; i < blkcou; i++)
			{
				ReadSector(bpb->DRV_num, fstblk, 1, BufferAddr);	/*��ȡ��������*/
				PutChar('#');
				brd += bpb->bps;
				fstblk++;
				BufferAddr += bpb->bps;
				if (brd >= SrcDir->len[0])	/*��ȡ���*/
					return BufferAddr;
			}
			idxi++;
		}
		ReadSector(bpb->DRV_num, idxfstblk, idxblkcou, FAR2LINE((DWORD)((void far *)idx)));
		idxi = 0;
	}
}

/*����Ŀ¼��*/
WORD SearchDir(	BPB *bpb,			/*���룺����BPB*/
				BLKID *idx,			/*�޸ģ���������*/
				BYTE *buf,			/*�޸ģ����ݻ���*/
				DIR *SrcDir,		/*���룺������Ŀ¼*/
				BYTE *FileName,		/*���룺��������Ŀ¼������*/
				DIR *DstDir)		/*������ҵ���Ŀ¼��*/
{
	WORD idxcou, idxi, idxblkcou;	/*ÿ�ζ�ȡ�������ڵ����������ڵ�������ÿ�ζ�ȡ������������*/
	DWORD idxfstblk, brd;	/*���������������Ѷ�ȡ�ֽ���*/

	idxcou = bpb->bps / sizeof(BLKID) * bpb->spc;
	if (idxcou > IDX_COU)	/*���峤��*/
		idxcou = IDX_COU;
	idxi = idxcou - 3;
	idxblkcou = idxcou * sizeof(BLKID) / bpb->bps;
	brd = 0;
	memcpy((BYTE *)(idx + idxi), (BYTE *)(SrcDir->idx), sizeof(BLKID) * 3);	/*����Ŀ¼���е�����*/
	for (;;)
	{
		for (;;)	/*����������*/
		{
			WORD blkcou, i;
			DWORD fstblk;

			if (idxi >= idxcou)	/*��ȡ��һ����*/
			{
				idxfstblk += idxblkcou;
				break;
			}
			else if ((blkcou = idx[idxi].cou) == 0)	/*��ȡ��һ������*/
			{
				idxfstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;
				break;
			}
			blkcou *= bpb->spc;	/*ȡ��Ҫ��ȡ��������*/
			fstblk = bpb->secoff + bpb->cluoff + bpb->spc * idx[idxi].fst;	/*ȡ��Ҫ��ȡ����������*/
			for (i = 0; i < blkcou; i += idxblkcou)
			{
				WORD j;

				ReadSector(bpb->DRV_num, fstblk, idxblkcou, FAR2LINE((DWORD)((void far *)buf)));	/*��ȡ��������*/
				for (j = 0; j < bpb->bps * idxblkcou / sizeof(DIR); j++)
				{
					if (strcmp(((DIR *)buf)[j].name, FileName) == 0)	/*�����ɹ�*/
					{
						*DstDir = ((DIR *)buf)[j];	/*�����ҵ���Ŀ¼��*/
						return NO_ERROR;
					}
				}
				brd += bpb->bps * idxblkcou;
				if (brd >= SrcDir->len[0])	/*��ȡ���*/
					return NOT_FOUND;
				fstblk += idxblkcou;
			}
			idxi++;
		}
		ReadSector(bpb->DRV_num, idxfstblk, idxblkcou, FAR2LINE((DWORD)((void far *)idx)));
		idxi = 0;
	}
}

/*��·����ȡ�����ļ�*/
DWORD ReadPath(	BPB *bpb,			/*���룺����BPB*/
				BLKID *idx,			/*�޸ģ���������*/
				BYTE *buf,			/*�޸ģ����ݻ���*/
				DIR *SrcDir,		/*�޸ģ��ļ�Ŀ¼��*/
				DIR *DstDir,		/*�޸ģ�Ŀ��Ŀ¼��*/
				BYTE *path,			/*���룺�ļ�·��*/
				DWORD BufferAddr)	/*������������Զ��ַ*/
{
	while (*path)
	{
		BYTE *cp = path;

		while (*cp != '/' && *cp != 0)
			cp++;
		if (*cp)
		{
			*cp++ = 0;
			PutS(path);
			if (SearchDir(bpb, idx, buf, SrcDir, path, DstDir) != NO_ERROR)	/*ȡ��Ŀ¼��*/
				return 0;
			if (DstDir->attr & 0x10 == 0)	/*����Ƿ���Ŀ¼*/
				return 0;
			*SrcDir = *DstDir;
			path = cp;
			PutChar('/');
		}
		else
		{
			PutS(path);
			if (SearchDir(bpb, idx, buf, SrcDir, path, DstDir) != NO_ERROR)	/*ȡ���ļ�Ŀ¼��*/
				return 0;
			if (DstDir->attr & 0x10 != 0)	/*����Ƿ����ļ�*/
				return 0;
			BufferAddr = ReadFile(bpb, idx, DstDir, BufferAddr);	/*��ȡ�ļ�*/
			PutChar(13);
			PutChar(10);
			return BufferAddr;
		}
	}
}

/*Loader����������,����Ϊ�Զ������*/
void main(BPB *bpb)
{
	BLKID idx[IDX_COU];	/*16K�ֽ���������*/
	BYTE buf[BUF_COU];	/*16K�ֽ����ݻ���*/
	DIR RootDir, SrcDir, DstDir;	/*Ŀ¼���*/
	BYTE BootList[4096], *cmd;	/*�����б�*/
	DWORD addr = 0x10000, *BinAddr = BIN_ADDR;	/*�ں˴��λ��, ���������ָ��*/
	WORD *VesaMode = VESA_MODE;

	if (bpb->bps > BUF_COU)	/*���ÿ�����ֽ����Ƿ�Ϸ�*/
		goto BootError;
	ReadSector(bpb->DRV_num, bpb->secoff + bpb->cluoff, 1, FAR2LINE((DWORD)((void far *)buf)));	/*��ȡ��Ŀ¼����������*/
	SrcDir = RootDir = *((DIR*)buf);
	if (ReadPath(bpb, idx, buf, &SrcDir, &DstDir, bpb->BootPath, FAR2LINE((DWORD)((void far *)BootList))) == 0)
		goto BootError;
	cmd = BootList;
	BootList[DstDir.len] = 0;
	*VesaMode = 0;
	for (;;)
	{
		BYTE *cp = cmd;
		DWORD end;

		while (*cp != '\n' && *cp != 0)
			cp++;
		if (*cp)
			*cp++ = 0;
		switch (*cmd++)
		{
		case 'F':	/*File*/
			SrcDir = RootDir;
			if ((end = ReadPath(bpb, idx, buf, &SrcDir, &DstDir, cmd, addr)) == 0)
				goto BootError;
			end = (end + 0x00000FFF) & 0xFFFFF000;		/*������4K�߽�*/
			*BinAddr++ = addr;
			*BinAddr++ = end - addr;
			addr = end;
			break;
		case 'V':	/*VesaMode*/
			*VesaMode = atol(cmd);
			break;
		}
		if (*cp)	/*�����ļ�*/
			cmd = cp;
		else
		{
			*BinAddr = 0;
			SETUP();
		}
	}
BootError:
	PutChar('!');
	for (;;);
}

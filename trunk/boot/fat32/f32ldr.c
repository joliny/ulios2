/*	f32ldr.c for ulios
	���ߣ�����
	���ܣ���FAT32�ļ�ϵͳ�����������ں�
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

typedef struct _BPB0
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
}BPB0;	/*FAT32������¼ԭʼ����*/

typedef struct _BPB
{
	BYTE	DRV_num;	/*��������*/
	BYTE	DRV_count;	/*��������*/
	WORD	bps;	/*ÿ�����ֽ���*/
	WORD	spc;	/*ÿ��������*/
	WORD	res;	/*����������,����������¼*/
	DWORD	secoff;	/*�����ڴ����ϵ���ʼ����ƫ��*/
	DWORD	cluoff;	/*�������״ؿ�ʼ����ƫ��*/
}BPB;	/*FAT32������¼����*/

typedef struct _DIR
{
	BYTE name[11];	/*�ļ���*/
	BYTE attr;		/*����*/
	BYTE reserved;	/*����*/
	BYTE crtmils;	/*����ʱ��10����λ*/
	WORD crttime;	/*����ʱ��*/
	WORD crtdate;	/*��������*/
	WORD acsdate;	/*��������*/
	WORD idxh;		/*�״ظ�16λ*/
	WORD chgtime;	/*�޸�ʱ��*/
	WORD chgdate;	/*�޸�����*/
	WORD idxl;		/*�״ص�16λ*/
	DWORD len;		/*����*/
}DIR;	/*FAT32Ŀ¼��ṹ*/

#define BUF_COU		0x4000							/*���ݻ�������*/
#define IDX_COU		(BUF_COU / sizeof(DWORD))		/*������������*/

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

WORD namecmp(BYTE *name, BYTE *str)
{
	BYTE newnam[11], *namep;

	for (namep = newnam; namep < newnam + 11; namep++)
		*namep = ' ';
	for (namep = newnam; *str != '.' && *str; namep++, str++)
	{
		if (namep >= newnam + 8)
			return 1;
		if (*str == 0xE5 && namep == newnam)
			*namep = 0x05;
		else if (*str >= 'a' && *str <= 'z')
			*namep = *str - 0x20;
		else
			*namep = *str;
	}
	if (*str == '.')
		str++;
	for (namep = newnam + 8; *str; namep++, str++)
	{
		if (namep >= newnam + 11)
			return 1;
		if (*str >= 'a' && *str <= 'z')
			*namep = *str - 0x20;
		else
			*namep = *str;
	}
	for (namep = newnam; namep < newnam + 11; namep++, name++)
		if (*namep != *name)
			return 1;
	return 0;
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
				DWORD *idx,			/*�޸ģ���������*/
				DIR *SrcDir,		/*���룺�ļ�Ŀ¼��*/
				DWORD BufferAddr)	/*������������Զ��ַ*/
{
	DWORD prei, clui;/*�غż���,��һ�غ�*/

	prei = 0xFFFFFFFF;
	clui = ((DWORD)(SrcDir->idxh) << 16) | SrcDir->idxl;
	for (;;)
	{
		DWORD i, fstblk;

		fstblk = bpb->secoff + bpb->cluoff + bpb->spc * clui;	/*ȡ��Ҫ��ȡ����������*/
		for (i = 0; i < bpb->spc; i++)
		{
			ReadSector(bpb->DRV_num, fstblk, 1, BufferAddr);	/*��ȡ��������*/
			PutChar('#');
			fstblk++;
			BufferAddr += bpb->bps;
		}
		i = clui / (bpb->bps >> 2);
		if (i != prei / (bpb->bps >> 2))	/*��һ�غŲ��ڻ�����*/
			ReadSector(bpb->DRV_num, bpb->secoff + bpb->res + i, 1, FAR2LINE((DWORD)((void far *)idx)));
		prei = clui;
		clui = idx[clui % (bpb->bps >> 2)];
		if (clui == 0x0FFFFFFF)	/*�ļ�����*/
			return BufferAddr;
	}
}

/*����Ŀ¼��*/
WORD SearchDir(	BPB *bpb,			/*���룺����BPB*/
				DWORD *idx,			/*�޸ģ���������*/
				BYTE *buf,			/*�޸ģ����ݻ���*/
				DIR *SrcDir,		/*���룺������Ŀ¼*/
				BYTE *FileName,		/*���룺��������Ŀ¼������*/
				DIR *DstDir)		/*������ҵ���Ŀ¼��*/
{
	DWORD prei, clui;/*�غż���,��һ�غ�*/

	prei = 0xFFFFFFFF;
	clui = ((DWORD)(SrcDir->idxh) << 16) | SrcDir->idxl;
	for (;;)
	{
		DWORD i;
		DWORD fstblk;

		fstblk = bpb->secoff + bpb->cluoff + bpb->spc * clui;	/*ȡ��Ҫ��ȡ����������*/
		for (i = 0; i < bpb->spc; i++)
		{
			WORD j;
			ReadSector(bpb->DRV_num, fstblk, 1, FAR2LINE((DWORD)((void far *)buf)));	/*��ȡ��������*/
			for (j = 0; j < bpb->bps / sizeof(DIR); j++)
				if (namecmp(((DIR *)buf)[j].name, FileName) == 0)	/*�����ɹ�*/
				{
					*DstDir = ((DIR *)buf)[j];	/*�����ҵ���Ŀ¼��*/
					return NO_ERROR;
				}
			fstblk++;
		}
		i = clui / (bpb->bps >> 2);
		if (i != prei / (bpb->bps >> 2))	/*��һ�غŲ��ڻ�����*/
			ReadSector(bpb->DRV_num, bpb->secoff + bpb->res + i, 1, FAR2LINE((DWORD)((void far *)idx)));
		prei = clui;
		clui = idx[clui % (bpb->bps >> 2)];
		if (clui == 0x0FFFFFFF)	/*�ļ�����*/
			return NOT_FOUND;
	}
}

/*��·����ȡ�����ļ�*/
DWORD ReadPath(	BPB *bpb,			/*���룺����BPB*/
				DWORD *idx,			/*�޸ģ���������*/
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
void main(BPB0 *bpb0)
{
	DWORD idx[IDX_COU];	/*16K�ֽ���������*/
	BYTE buf[BUF_COU];	/*16K�ֽ����ݻ���*/
	BPB bpb;
	DIR RootDir, SrcDir, DstDir;	/*Ŀ¼���*/
	BYTE BootList[4096], *cmd;	/*�����б�*/
	DWORD addr = 0x10000, *BinAddr = BIN_ADDR;	/*�ں˴��λ��, ���������ָ��*/
	WORD *VesaMode = VESA_MODE;

	if (bpb0->bps > BUF_COU)	/*���ÿ�����ֽ����Ƿ�Ϸ�*/
		goto BootError;
	bpb.DRV_num = bpb0->DRV_num;	/*��������*/
	bpb.bps = bpb0->bps;	/*ÿ�����ֽ���*/
	bpb.spc = bpb0->spc;	/*ÿ��������*/
	bpb.res = bpb0->res;	/*����������,����������¼*/
	bpb.secoff = bpb0->secoff;	/*�����ڴ����ϵ���ʼ����ƫ��*/
	bpb.cluoff = bpb0->res + (bpb0->spf * bpb0->nf) - (bpb0->spc << 1);	/*�������״ؿ�ʼ����ƫ��*/
	RootDir.attr = 0x10;
	RootDir.idxh = (bpb0->rcn >> 16);
	RootDir.idxl = (WORD)bpb0->rcn;
	RootDir.len = 0;
	SrcDir = RootDir;
	if (ReadPath(&bpb, idx, buf, &SrcDir, &DstDir, bpb0->BootPath, FAR2LINE((DWORD)((void far *)BootList))) == 0)
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
			if ((end = ReadPath(&bpb, idx, buf, &SrcDir, &DstDir, cmd, addr)) == 0)
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

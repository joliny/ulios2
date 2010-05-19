/*	test.c
	���ߣ�����
	���ܣ�uliosϵͳ�ں˲��Գ���
	����޸����ڣ�2009-05-28
*/

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "../fs/fsapi.h"

typedef struct
{
	BYTE	VBESignature[4];	// �ַ���"VESA"
	WORD	VBEVersion;			// VBE�汾��,BCD��
	BYTE*	OEMString;			// ����OEM�ַ���
	DWORD	Capabilities;		// ��ʾ������
	WORD*	VideoMod;			// ��֧����ʾģʽ�б�
	WORD	VRAMMemory;			// ��ʾ�ڴ��С,��λ64KB
	BYTE	reserved[236];		// ����
	BYTE	OEMData[256];		// VESA2.0�汾���϶���
}__attribute__((packed)) VBE_INFO;

typedef struct
{
	WORD	ModeAttr;			// ģʽ����
	BYTE	WinAAttr, WinBAttr;	// ����A,B����
	WORD	WinGran;			// ��������,��KBΪ��λ
	WORD	WinSize;			// ���ڴ�С,��KBΪ��λ
	WORD	WinASeg, WinBSeg;	// ����A,B��ʼ�ε�ַ
	BYTE*	BankFunc;			// ��ҳ�������
	WORD	BytesPerScanline;	// ÿ��ˮƽɨ������ռ���ֽ���
	WORD	XRes, YRes;			// ˮƽ,��ֱ����ֱ���
	BYTE	XCharSize, YCharSize;	// �ַ����,�߶�
	BYTE	NumberOfPlanes;		// λƽ��ĸ���
	BYTE	BitsPerPixel;		// ÿ���ص�λ��
	BYTE	NumberOfBanks;		// CGA�߼�ɨ���߷�����
	BYTE	MemoryModel;		// ��ʾ�ڴ�ģʽ
	BYTE	BankSize;			// CGAÿ��ɨ���ߵĴ�С
	BYTE	NumberOfImagePages;	// ��ͬʱ������������ͼ����
	BYTE	reserve1;			// Ϊҳ�湦�ܱ���
	BYTE	RedMaskSize;		// ��ɫ��ռλ��
	BYTE	RedFieldPosition;	// ��ɫ�������Чλλ��
	BYTE	GreenMaskSize;		// ��ɫ��ռλ��
	BYTE	GreenFieldPosition;	// ��ɫ�������Чλλ��
	BYTE	BlueMaskSize;		// ��ɫ��ռλ��
	BYTE	BlueFieldPosition;	// ��ɫ�������Чλλ��
	BYTE	RsvdMaskSize;		// ����ɫ��ռλ��
	BYTE	RsvdFieldPosition;	// ����ɫ�������Чλλ��
	BYTE	DirectColorModeInfo;// ֱ����ɫģʽ����
	// ����ΪVBE2.0�汾���϶���
	BYTE*	PhyBase;			// ��ʹ�ô��֡����ʱΪָ�����׵�ַ
	DWORD	OffScreenMemOffset;	// ֡�����׵�ַ��32λƫ����
	WORD	OffScreenMemSize;	// ��ʾ��������С,��KBΪ��λ
	// ����ΪVBE3.0�汾���϶���
	WORD	LinBytesPerScanLine;	// ���Ի�������ÿ��ɨ���ߵĳ���,���ֽ�Ϊ��λ
	BYTE	BnkNumberOfImagePages;	// ʹ�ô��ڹ���ʱ����ʾҳ����
	BYTE	LinNumberOfImagePages;	// ʹ�ô�����Ի�����ʱ����ʾҳ����
	BYTE	LinRedMaskSize;			// ʹ�ô�����Ի�����ʱ��ɫ��ռλ��
	BYTE	LinRedFieldPosition;	// ʹ�ô�����Ի�����ʱ��ɫ�������Чλλ��
	BYTE	LinGreenMaskSize;		// ʹ�ô�����Ի�����ʱ��ɫ��ռλ��
	BYTE	LinGreenFieldPosition;	// ʹ�ô�����Ի�����ʱ��ɫ�������Чλλ��
	BYTE	LinBlueMaskSize;		// ʹ�ô�����Ի�����ʱ��ɫ��ռλ��
	BYTE	LinBlueFieldPosition;	// ʹ�ô�����Ի�����ʱ��ɫ�������Чλλ��
	BYTE	LinRsvdMaskSize;		// ʹ�ô�����Ի�����ʱ����ɫ��ռλ��
	BYTE	LinRsvdFieldPosition;	// ʹ�ô�����Ի�����ʱ����ɫ�������Чλλ��
	BYTE	reserve2[194];			// ����
}__attribute__((packed)) MODE_INFO;

#define FAR2LINE(addr)	((WORD)(addr) + (((addr) & 0xFFFF0000) >> 12))
#define LINE2FAR(addr)	((WORD)(addr) | (((addr) & 0xFFFF0000) << 12))

THREAD_ID AthdPtid, FsPtid;

int main()
{
	long res;
	void *addr;
	VBE_INFO *vbe;
	MODE_INFO *mode;
	WORD *ml;

	KPrintf("map 0xB8000 test!\n", 0);
	res = KMapPhyAddr(&addr, 0xB8000, 0x1000);
	if (res != NO_ERROR)
		return res;
	KPrintf("addr:0x%X!\n", (DWORD)addr);
	for (res = 3200; res < 3360; res++)
		((char*)addr)[res] = 'X';
	res = KFreeAddr(addr);
	if (res != NO_ERROR)
		return res;

	KPrintf("\nmap VBE_INFO test!\n", 0);
	res = KMapPhyAddr(&addr, 0x90000, 0x70000);
	if (res != NO_ERROR)
		return res;
	vbe = (VBE_INFO*)(addr + 0x100);
	mode = (MODE_INFO*)(addr + 0x300);
	KPrintf("V_VBEsign:%s\n", (DWORD)vbe->VBESignature);
	KPrintf("V_VBEver:%X\n", vbe->VBEVersion);
	KPrintf("V_OEMstr:%s\nV_MList:", (DWORD)addr + FAR2LINE((DWORD)vbe->OEMString) - 0x90000);
	ml = (WORD*)((DWORD)addr + FAR2LINE((DWORD)vbe->VideoMod) - 0x90000);
	while (*ml != 0xFFFF)
		KPrintf(" %X", *ml++);
	KPrintf("\nV_MEMsize:%u KB\n", vbe->VRAMMemory * 64);
	res = KFreeAddr(addr);
	if (res != NO_ERROR)
		return res;

	KPrintf("\nKeyboard and athd driver test!\n", 0);
	if ((res = KRegIrq(1)) != NO_ERROR)
		return res;
	if ((res = KGetKpToThed(SRV_ATHD_PORT, &AthdPtid)) != NO_ERROR)
		KPrintf("GetErr:%d\n", res);
	if ((res = KGetKpToThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)
		KPrintf("FS ERR", 0);
	FSChDir(FsPtid, "/0/ulios");

	for (;;)
	{
		THREAD_ID ptid;
		BYTE buf[ATHD_BPS * 128];
		DWORD data[MSG_DATA_LEN];
		DWORD KbdCode;

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
		{
			KPrintf("Err:%d\n", res);
			continue;
		}
		if (data[0] != MSG_ATTR_IRQ || data[1] != 1)
			continue;
		KbdCode = inb(0x60);
		if (KbdCode < 0x80)
			KPrintf("down:%d\n", KbdCode);
		else
		{
			DWORD fh;
			THREAD_ID newpt;
//			TM tm;
			KPrintf("up:%d\n", KbdCode);

//			res = HDReadSector(AthdPtid, 0, sec, 128, buf);
//			sec += 128;
//			if (res != NO_ERROR)
//				KPrintf("ReadErr:%d\n", res);
//			for (res = 0; res < 64; res++)
//				KPrintf("%c", buf[res]);

			fh = FSGetExid(FsPtid, "pro.bin");
			KPrintf("fh %d ", fh);
			KCreateProcess(0, fh, INVALID, &newpt);
			KPrintf("pid %d ", newpt.ProcID);
		}
	}
	return NO_ERROR;
}

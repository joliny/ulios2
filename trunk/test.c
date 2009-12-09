/*	test.c
	���ߣ�����
	���ܣ�uliosϵͳ�ں˲��Գ���
	����޸����ڣ�2009-05-28
*/

#include "MkApi/ulimkapi.h"

typedef struct 
{
	BYTE	VBESignature[4];	// �ַ���"VESA"
	WORD	VBEVersion;			// VBE�汾��,BCD��
	BYTE*	OEMString;			// ����OEM�ַ���
	DWORD	Capabilities;		// ��ʾ������
	WORD*	VideoMod;			// ��֧����ʾģʽ�б�
	WORD	VRAMMemory;			// ��ʾ�ڴ��С,��λ64KB
	WORD	OemSoftwareRev;		// VBE�����OEM�޶��汾��,BCD��
	BYTE*	OemVendorName;		// ��ʾ�����쳧���ַ���
	BYTE*	OemProductName;		// ��ʾ����Ʒ�����ַ���
	BYTE*	OemProductRev;		// ��ʾ���޶��汾�Ż��Ʒ�ȼ��ַ���
	BYTE	reserved[222];		// ����
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

int main()
{
	long res;
	BYTE *addr;

	KPrintf("Keyboard driver!\n", 0);
//	res = KMapPhyAddr((DWORD*)&addr, 0xB8000, 0x1000);
//	if (res != NO_ERROR)
//		return res;
//	KPrintf("addr:0x%X!", (DWORD)addr);
//	for (res = 0; res < 100; res++)
//		addr[res] = 'X';
//	res = KFreeAddr((DWORD)addr);
//	if (res != NO_ERROR)
//		return res;
//	for (res = 100; res < 200; res++)
//		addr[res] = 'X';
	VBE_INFO *vbe;
	MODE_INFO *mode;
	WORD *ml;

	res = KMapPhyAddr((DWORD*)&addr, 0x90000, 0x70000);
	if (res != NO_ERROR)
		return res;
	vbe = (VBE_INFO*)(addr + 0x100);
	mode = (MODE_INFO*)(addr + 0x300);
	KPrintf("V_VBEsign:%s\n", vbe->VBESignature);
	KPrintf("V_VBEver:%X\n", vbe->VBEVersion);
	KPrintf("V_OEMstr:%s\nV_MList:", (DWORD)addr + FAR2LINE((DWORD)vbe->OEMString) - 0x90000);
	ml = (DWORD)addr + FAR2LINE((DWORD)vbe->VideoMod) - 0x90000;
	while (*ml != 0xFFFF)
		KPrintf(" %X", *ml++);
	KPrintf("\nV_MEMsize:0x%X\n", vbe->VRAMMemory);
	KPrintf("V_OEMVendor:%s\n", (DWORD)addr + FAR2LINE((DWORD)vbe->OemVendorName) - 0x90000);
	KPrintf("V_OEMProduct:%s\n", (DWORD)addr + FAR2LINE((DWORD)vbe->OemProductName) - 0x90000);
	KPrintf("V_OEMProdRev:%s\n", (DWORD)addr + FAR2LINE((DWORD)vbe->OemProductRev) - 0x90000);
	res = KFreeAddr((DWORD)addr);
	if (res != NO_ERROR)
		return res;

	res = KRegIrq(1);
	if (res != NO_ERROR)
		return res;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD c, d, src, dest;
		DWORD KbdCode;

		if ((res = KWaitMsg(0, &ptid, &c, &d, &src, &dest)) != NO_ERROR)
		{
			KPrintf("Err:%d\n", res);
			continue;
		}
		KPrintf("ptid:%d ", *((DWORD*)&ptid));
		if (c != (MSG_ATTR_IRQ | 1))
			continue;
		KbdCode = inb(0x60);
		if (KbdCode < 0x80)
			KPrintf("down:%d\n", KbdCode);
		else
			KPrintf("up:%d\n", KbdCode);
	}
	return NO_ERROR;
}

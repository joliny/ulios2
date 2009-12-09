/*	test.c
	作者：孙亮
	功能：ulios系统内核测试程序
	最后修改日期：2009-05-28
*/

#include "MkApi/ulimkapi.h"

typedef struct 
{
	BYTE	VBESignature[4];	// 字符串"VESA"
	WORD	VBEVersion;			// VBE版本号,BCD码
	BYTE*	OEMString;			// 厂商OEM字符串
	DWORD	Capabilities;		// 显示卡特性
	WORD*	VideoMod;			// 所支持显示模式列表
	WORD	VRAMMemory;			// 显示内存大小,单位64KB
	WORD	OemSoftwareRev;		// VBE软件的OEM修订版本号,BCD码
	BYTE*	OemVendorName;		// 显示卡制造厂商字符串
	BYTE*	OemProductName;		// 显示卡产品名称字符串
	BYTE*	OemProductRev;		// 显示卡修订版本号或产品等级字符串
	BYTE	reserved[222];		// 保留
	BYTE	OEMData[256];		// VESA2.0版本以上定义
}__attribute__((packed)) VBE_INFO;

typedef struct 
{
	WORD	ModeAttr;			// 模式属性
	BYTE	WinAAttr, WinBAttr;	// 窗口A,B属性
	WORD	WinGran;			// 窗口粒度,以KB为单位
	WORD	WinSize;			// 窗口大小,以KB为单位
	WORD	WinASeg, WinBSeg;	// 窗口A,B起始段地址
	BYTE*	BankFunc;			// 换页调用入口
	WORD	BytesPerScanline;	// 每条水平扫描线所占的字节数
	WORD	XRes, YRes;			// 水平,垂直方向分辨率
	BYTE	XCharSize, YCharSize;	// 字符宽度,高度
	BYTE	NumberOfPlanes;		// 位平面的个数
	BYTE	BitsPerPixel;		// 每像素的位数
	BYTE	NumberOfBanks;		// CGA逻辑扫描线分组数
	BYTE	MemoryModel;		// 显示内存模式
	BYTE	BankSize;			// CGA每组扫描线的大小
	BYTE	NumberOfImagePages;	// 可同时载入的最大满屏图像数
	BYTE	reserve1;			// 为页面功能保留
	BYTE	RedMaskSize;		// 红色所占位数
	BYTE	RedFieldPosition;	// 红色的最低有效位位置
	BYTE	GreenMaskSize;		// 绿色所占位数
	BYTE	GreenFieldPosition;	// 绿色的最低有效位位置
	BYTE	BlueMaskSize;		// 蓝色所占位数
	BYTE	BlueFieldPosition;	// 蓝色的最低有效位位置
	BYTE	RsvdMaskSize;		// 保留色所占位数
	BYTE	RsvdFieldPosition;	// 保留色的最低有效位位置
	BYTE	DirectColorModeInfo;// 直接颜色模式属性
	// 以下为VBE2.0版本以上定义
	BYTE*	PhyBase;			// 可使用大的帧缓存时为指向其首地址
	DWORD	OffScreenMemOffset;	// 帧缓存首地址的32位偏移量
	WORD	OffScreenMemSize;	// 显示缓冲区大小,以KB为单位
	// 以下为VBE3.0版本以上定义
	WORD	LinBytesPerScanLine;	// 线性缓冲区中每条扫描线的长度,以字节为单位
	BYTE	BnkNumberOfImagePages;	// 使用窗口功能时的显示页面数
	BYTE	LinNumberOfImagePages;	// 使用大的线性缓冲区时的显示页面数
	BYTE	LinRedMaskSize;			// 使用大的线性缓冲区时红色所占位数
	BYTE	LinRedFieldPosition;	// 使用大的线性缓冲区时红色的最低有效位位置
	BYTE	LinGreenMaskSize;		// 使用大的线性缓冲区时绿色所占位数
	BYTE	LinGreenFieldPosition;	// 使用大的线性缓冲区时绿色的最低有效位位置
	BYTE	LinBlueMaskSize;		// 使用大的线性缓冲区时蓝色所占位数
	BYTE	LinBlueFieldPosition;	// 使用大的线性缓冲区时蓝色的最低有效位位置
	BYTE	LinRsvdMaskSize;		// 使用大的线性缓冲区时保留色所占位数
	BYTE	LinRsvdFieldPosition;	// 使用大的线性缓冲区时保留色的最低有效位位置
	BYTE	reserve2[194];			// 保留
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

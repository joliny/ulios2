/*	vesa.c for ulios driver
	作者：孙亮
	功能：VESA 2.0显卡驱动服务程序
	最后修改日期：2010-05-19
*/

#include "basesrv.h"
#include "../fs/fsapi.h"

#define FAR2LINE(addr)	((WORD)(addr) + (((addr) & 0xFFFF0000) >> 12))
#define FONT_FILE	"font.bin"
#define FONT_SIZE	199344

int main()
{
	DWORD CurMode;	/*当前显示模式*/
	DWORD width, height;	/*屏幕大小*/
	DWORD PixBits;	/*每像素位数*/
	DWORD VmPhy;	/*显存物理地址*/
	DWORD VmSiz;	/*映射显存大小*/
	WORD *Modep;
	DWORD ModeCou;	/*模式数量*/
	void *vm;		/*显存映射地址*/
	THREAD_ID ptid;
	long res;	/*返回结果*/
	WORD ModeList[VESA_MAX_MODE];	/*显示模式列表*/
	BYTE font[FONT_SIZE];	/*字体*/

	if ((res = KRegKnlPort(SRV_VESA_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	if ((res = KMapPhyAddr(&vm, 0x90000, 0x70000)) != NO_ERROR)	/*取得显卡信息*/
		return res;
	CurMode = *((DWORD*)(vm + 0x2FC));
	if (CurMode)	/*图形模式*/
	{
		width = *((WORD*)(vm + 0x512));
		height = *((WORD*)(vm + 0x514));
		PixBits = *((BYTE*)(vm + 0x519));
		VmPhy = *((DWORD*)(vm + 0x528));
	}
	else	/*文本模式*/
	{
		width = 80;
		height = 25;
		PixBits = 16;	/*每个字符占2字节*/
		VmPhy = 0xB8000;
	}
	Modep = (WORD*)((DWORD)vm + FAR2LINE(*(DWORD*)(vm + 0x30E)) - 0x90000);	/*复制模式列表*/
	for (ModeCou = 0; *Modep != 0xFFFF; ModeCou++, Modep++)
		ModeList[ModeCou] = *Modep;
	if (CurMode)
	{
		if ((res = KGetKptThed(SRV_FS_PORT, &ptid)) != NO_ERROR)	/*取得文件系统线程ID*/
			return res;
		if ((res = FSChDir(ptid, (const char*)(vm + 0x280))) != NO_ERROR)	/*切换到系统目录*/
			return res;
		if ((res = FSopen(ptid, FONT_FILE, FS_OPEN_READ)) < 0)	/*打开字体文件*/
			return res;
		if (FSread(ptid, res, font, FONT_SIZE) <= 0)	/*读取字体文件*/
			return -1;
		FSclose(ptid, res);
	}
	KFreeAddr(vm);
	VmSiz = ((PixBits + 7) >> 3) * width * height;
	if ((res = KMapPhyAddr(&vm, VmPhy, VmSiz)) != NO_ERROR)	/*映射显存*/
		return res;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_VESA:
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case VESA_API_GETVMEM:
				data[MSG_RES_ID] = NO_ERROR;
				data[3] = width;	/*像素/字符分辨率*/
				data[4] = height;
				data[5] = PixBits;	/*色彩位数*/
				KReadProcAddr(vm, VmSiz, &ptid, data, 0);
				break;
			case VESA_API_GETFONT:
				if (CurMode)	/*图形模式*/
				{
					data[MSG_RES_ID] = NO_ERROR;
					data[3] = 6;	/*字符大小*/
					data[4] = 12;
					KWriteProcAddr(font, FONT_SIZE, &ptid, data, 0);
				}
				else	/*文本模式*/
				{
					data[MSG_RES_ID] = VESA_ERR_TEXTMODE;
					KSendMsg(&ptid, data, 0);
				}
				break;
			}
			break;
		case MSG_ATTR_ROMAP:
			data[MSG_RES_ID] = VESA_ERR_ARGS;
			KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
			break;
		case MSG_ATTR_RWMAP:
			if ((data[MSG_API_ID] & MSG_API_MASK) == VESA_API_GETMODE && data[MSG_SIZE_ID] >= (((ModeCou + 1) >> 1) << 2))
			{
				memcpy32((void*)data[MSG_ADDR_ID], ModeList, (ModeCou + 1) >> 1);
				data[MSG_RES_ID] = NO_ERROR;
				data[MSG_SIZE_ID] = ModeCou;
				data[3] = CurMode;
			}
			else
				data[MSG_RES_ID] = VESA_ERR_ARGS;
			KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
			break;
		}
	}
	KFreeAddr(vm);
	KUnregKnlPort(SRV_VESA_PORT);
	return NO_ERROR;
}

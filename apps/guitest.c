/*	guitest.c for ulios application
	作者：孙亮
	功能：GUI测试程序
	最后修改日期：2010-12-11
*/

#include "../driver/basesrv.h"
#include "../gui/guiapi.h"
#include "../fs/fsapi.h"

/*加载BMP图像文件*/
long LoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height)
{
	BYTE BmpHead[32];
	THREAD_ID FsPtid;
	long bmpw, bmph, file, res;
	
	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)	/*取得文件系统服务线程*/
		return res;
	if ((file = FSopen(FsPtid, path, FS_OPEN_READ)) < 0)	/*读取BMP文件*/
		return file;
	if (FSread(FsPtid, file, &BmpHead[2], 30) < 30 || *((WORD*)&BmpHead[2]) != 0x4D42 || *((WORD*)&BmpHead[30]) != 24)	/*保证32位对齐访问*/
	{
		FSclose(FsPtid, file);
		return -1;
	}
	bmpw = *((long*)&BmpHead[20]);
	bmph = *((long*)&BmpHead[24]);
	if (bmpw * bmph > len)
	{
		FSclose(FsPtid, file);
		return -1;
	}
	FSseek(FsPtid, file, 54, FS_SEEK_SET);
	len = ((DWORD)bmpw * 3 + 3) & 0xFFFFFFFC;
	for (res = bmph, buf += bmpw * (res - 1); res > 0; res--, buf -= bmpw)
	{
		BYTE *src, *dst;
		
		if (FSread(FsPtid, file, buf, len) < len)
		{
			FSclose(FsPtid, file);
			return -1;
		}
		src = (BYTE*)buf + bmpw * 3;
		dst = (BYTE*)buf + bmpw * 4;
		while (src > (BYTE*)buf)
		{
			*(--dst) = 0xFF;
			*(--dst) = *(--src);
			*(--dst) = *(--src);
			*(--dst) = *(--src);
		}
	}
	FSclose(FsPtid, file);
	if (width)
		*width = bmpw;
	if (height)
		*height = bmph;
	return NO_ERROR;
}

/*双字转化为数字*/
char *Itoa(char *buf, DWORD n, DWORD r)
{
	static const char num[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	char *p, *q;
	
	q = p = buf;
	do
	{
		*p++ = num[n % r];
		n /= r;
	}
	while (n);
	buf = p;	/*确定字符串尾部*/
	*p-- = '\0';
	while (p > q)	/*翻转字符串*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	return buf;
}

int main()
{
	DWORD vbuf[0x20000];
	char buf[12];
	THREAD_ID GuiPtid, CuiPtid, ptid;
	long width, height, gid[2], res;

	width = 256;
	height = 128;
	LoadBmp("laopo.bmp", vbuf, 0x20000, &width, &height);
	KGetKptThed(SRV_GUI_PORT, &GuiPtid);

	gid[0] = GUIcreate(GuiPtid, 0, 0, vbuf, 0, 0, width, height);
	gid[1] = GUIcreate(GuiPtid, 0, 0, vbuf, width, 0, width, height);
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		switch (data[MSG_API_ID] & MSG_API_MASK)
		{
		case GM_MOUSEMOVE:
			if (data[1] & MUS_STATE_LBUTTON)
				GUImove(GuiPtid, data[GUIMSG_GOBJ_ID], data[2] - 128, data[3] - 64);
			break;
		case GM_LBUTTONDOWN:
			GUISetFocus(GuiPtid, data[GUIMSG_GOBJ_ID]);
			break;
		case GM_MOUSEWHEEL:
			{
				DWORD clk;
				KGetClock(&clk);
				memset32(vbuf, clk, width * height);
				GUIpaint(GuiPtid, data[GUIMSG_GOBJ_ID], 10, 10, width - 20, height - 20);
			}
			break;
		case GM_LBUTTONDBCLK:
			return NO_ERROR;
		}
	}
/*		KGetKptThed(SRV_CUI_PORT, &CuiPtid);
		Itoa(buf, data[MSG_API_ID] & MSG_API_MASK, 16);
		CUIPutS(CuiPtid, buf);
		CUIPutC(CuiPtid, ' ');
*/	return NO_ERROR;
}

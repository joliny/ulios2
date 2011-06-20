/*	guitest.c for ulios application
	���ߣ�����
	���ܣ�GUI���Գ���
	����޸����ڣ�2010-12-11
*/

#include "../driver/basesrv.h"
#include "../gui/guiapi.h"
#include "../fs/fsapi.h"

/*����BMPͼ���ļ�*/
long LoadBmp(char *path, DWORD *buf, DWORD len, long *width, long *height)
{
	BYTE BmpHead[32];
	THREAD_ID FsPtid;
	long bmpw, bmph, file, res;
	
	if ((res = KGetKptThed(SRV_FS_PORT, &FsPtid)) != NO_ERROR)	/*ȡ�ü����������߳�*/
		return res;
	if ((file = FSopen(FsPtid, path, FS_OPEN_READ)) < 0)	/*��ȡBMP�ļ�*/
		return file;
	if (FSread(FsPtid, file, &BmpHead[2], 30) < 30 || *((WORD*)&BmpHead[2]) != 0x4D42 || *((WORD*)&BmpHead[30]) != 24)	/*��֤32λ�������*/
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

/*˫��ת��Ϊ����*/
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
	buf = p;	/*ȷ���ַ���β��*/
	*p-- = '\0';
	while (p > q)	/*��ת�ַ���*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	return buf;
}

int main()
{
	DWORD vbuf[300000];
	THREAD_ID GuiPtid;
	long width, height, gid;

	width = 200;
	height = 150;
	LoadBmp("laopo.bmp", vbuf, 30000, &width, &height);
	KGetKptThed(SRV_GUI_PORT, &GuiPtid);
	gid = GUIcreate(GuiPtid, 0, 0, vbuf, 0, 0, width, height);
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];
		THREAD_ID ptid;

		if (KRecvMsg(&ptid, data, INVALID))	/*�ȴ���Ϣ*/
			break;
		if ((data[MSG_API_ID] & MSG_API_MASK) == GM_MOUSEMOVE)
		{
			if (data[1] & MUS_STATE_LBUTTON)
				GUImove(GuiPtid, gid, data[2] - 100, data[3] - 75);
			else if (data[1] & MUS_STATE_RBUTTON)
				GUIsize(GuiPtid, gid, vbuf, 0, 0, data[2] + 1, data[3] + 1);
		}
	}
	return NO_ERROR;
}

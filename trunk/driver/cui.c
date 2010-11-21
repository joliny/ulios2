/*	cui.c for ulios driver
	���ߣ�����
	���ܣ��ַ��û���������
	����޸����ڣ�2010-06-10
*/

#include "basesrv.h"

#define MAX_LINE	200	/*ÿ������ַ�����*/
#define CURS_WIDTH	2	/*���߶�*/

DWORD width, height;	/*��ʾ�ַ�����*/
DWORD CursX, CursY;		/*���λ��*/
DWORD BgColor, CharColor;	/*��ǰ����,ǰ��ɫ*/

/*����*/
void ClearScr()
{
	CursY = CursX = 0;
	GDIFillRect(0, 0, GDIwidth, GDIheight, BgColor);
	GDIFillRect(0, GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
}

/*����ַ���*/
void PutStr(const char *str)
{
	DWORD BufX, BufY;	/*�ַ���д��λ��*/
	char LineBuf[MAX_LINE], *bufp;	/*�л���*/

	bufp = LineBuf;
	BufX = GDICharWidth * CursX;
	BufY = GDICharHeight * CursY;
	while (*str)
	{
		switch (*str)
		{
		case '\n':
			GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY, GDICharWidth, GDICharHeight, BgColor);	/*�������*/
			CursX = width;
			break;
		case '\t':
			do
				*bufp++ = ' ';
			while (++CursX & 7);
			break;
		default:
			*bufp++ = *str;
			CursX++;
			break;
		}
		if (CursX >= width)	/*��������һ�У��ص�ǰ��*/
		{
			CursX = 0;
			CursY++;
			if (bufp != LineBuf)
			{
				*bufp = '\0';
				GDIFillRect(BufX, BufY, GDICharWidth * (bufp - LineBuf), GDICharHeight, BgColor);
				GDIDrawStr(BufX, BufY, LineBuf, CharColor);
				bufp = LineBuf;
			}
			BufX = GDICharWidth * CursX;
			BufY = GDICharHeight * CursY;
		}
		if (CursY >= height)	/*��������һ�У����Ϲ���*/
		{
			CursY--;
			GDIMoveUp(GDICharHeight);
			GDIFillRect(0, GDICharHeight * (height - 1), GDIwidth, GDICharHeight, BgColor);
			BufY = GDICharHeight * CursY;
		}
		str++;
	}
	if (bufp != LineBuf)	/*������һ��*/
	{
		*bufp = '\0';
		GDIFillRect(BufX, BufY, GDICharWidth * (bufp - LineBuf), GDICharHeight, BgColor);
		GDIDrawStr(BufX, BufY, LineBuf, CharColor);
	}
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
}

/*�˸���*/
void BackSp()
{
	if (CursX == 0 && CursY == 0)
		return;
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*������*/
	if (CursX)
		CursX--;
	else
	{
		CursX = width - 1;
		CursY--;
	}
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY, GDICharWidth, GDICharHeight, BgColor);	/*�������*/
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
}

int main()
{
	long res;

	if ((res = KRegKnlPort(SRV_CUI_PORT)) != NO_ERROR)	/*ע�����˿ں�*/
		return res;
	if ((res = GDIinit()) != NO_ERROR)
		return res;
	width = GDIwidth / GDICharWidth;	/*������ʾ�ַ�����*/
	height = GDIheight / GDICharHeight;
	CursY = CursX = 0;
	BgColor = 0;
	CharColor = 0xFFFFFFFF;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if ((data[0] & 0xFFFF0000) == MSG_ATTR_USER)	/*��ͨ������Ϣ*/
		{
			switch (data[3])
			{
			case CUI_API_GETCOL:	/*ȡ���ַ�������ɫ*/
				data[1] = CharColor;
				data[2] = BgColor;
				KSendMsg(&ptid, data, 0);
				break;
			case CUI_API_SETCOL:	/*�����ַ�������ɫ*/
				CharColor = data[1];
				BgColor = data[2];
				break;
			case CUI_API_GETCUR:	/*ȡ�ù��λ�ù��ܺ�*/
				data[1] = CursX;
				data[2] = CursY;
				KSendMsg(&ptid, data, 0);
				break;
			case CUI_API_SETCUR:	/*���ù��λ��*/
				GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*������*/
				if (data[1] < width)
					CursX = data[1];
				if (data[2] < height)
					CursY = data[2];
				GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
				break;
			case CUI_API_CLRSCR:	/*����*/
				ClearScr();
				break;
			case CUI_API_PUTC:		/*����ַ�*/
				if (data[1] == '\b')
					BackSp();
				else
					PutStr((const char*)&data[1]);
				break;
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_MAP)	/*ӳ����Ϣ*/
		{
			switch (data[3])
			{
			case CUI_API_PUTS:	/*����ַ���*/
				if (((const char*)data[2])[data[1] - 1])
					data[0] = CUI_ERR_ARGS;
				else
				{
					PutStr((const char*)data[2]);
					data[0] = NO_ERROR;
				}
				break;
			}
			KUnmapProcAddr((void*)data[2], data);
		}
	}
	GDIrelease();
	KUnregKnlPort(SRV_CUI_PORT);
	return NO_ERROR;
}

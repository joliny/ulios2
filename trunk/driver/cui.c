/*	cui.c for ulios driver
	���ߣ�����
	���ܣ��ַ��û���������
	����޸����ڣ�2010-06-10
*/

#include "basesrv.h"
#include "../lib/gdi.h"

#define MAX_LINE	200	/*ÿ������ַ�����*/
#define CURS_WIDTH	2	/*���߶�*/

BOOL isTextMode;	/*�Ƿ��ı�ģʽ*/
DWORD width, height;	/*��ʾ�ַ�����*/
DWORD CursX, CursY;		/*���λ��*/
DWORD BgColor, CharColor;	/*��ǰ����,ǰ��ɫ*/

#define TEXT_MODE_COLOR	((CharColor & 0xF) | ((BgColor & 0xF) << 4))

/*�ַ�ģʽ���ù��*/
void SetTextCurs()
{
	DWORD PortData;

	PortData = CursX + CursY * width;
	outb(0x3D4, 0xF);
	outb(0x3D5, PortData & 0xFF);
	outb(0x3D4, 0xE);
	outb(0x3D5, (PortData >> 8));
}

/*����*/
void ClearScr()
{
	CursY = CursX = 0;
	if (isTextMode)
	{
		memset32(GDIvm, (TEXT_MODE_COLOR << 8) | (TEXT_MODE_COLOR << 24), width * height / 2);
		SetTextCurs();
	}
	else
	{
		GDIFillRect(0, 0, GDIwidth, GDIheight, BgColor);
		GDIFillRect(0, GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
	}
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
			if (!isTextMode)
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
				if (isTextMode)
				{
					WORD *CurVm;

					CurVm = ((WORD*)GDIvm) + BufX + BufY * width;
					for (bufp = LineBuf; *bufp; bufp++, CurVm++)
						*CurVm = *(BYTE*)bufp | (TEXT_MODE_COLOR << 8);
				}
				else
				{
					GDIFillRect(BufX, BufY, GDICharWidth * (bufp - LineBuf), GDICharHeight, BgColor);
					GDIDrawStr(BufX, BufY, LineBuf, CharColor);
				}
				bufp = LineBuf;
			}
			BufX = GDICharWidth * CursX;
			BufY = GDICharHeight * CursY;
		}
		if (CursY >= height)	/*��������һ�У����Ϲ���*/
		{
			CursY--;
			if (isTextMode)
			{
				memcpy32(GDIvm, ((WORD*)GDIvm) + width, width * (height - 1) / 2);
				memset32(((WORD*)GDIvm) + width * (height - 1), (TEXT_MODE_COLOR << 8) | (TEXT_MODE_COLOR << 24), width / 2);
			}
			else
			{
				GDIMoveUp(GDICharHeight);
				GDIFillRect(0, GDICharHeight * (height - 1), GDIwidth, GDICharHeight, BgColor);
			}
			BufY = GDICharHeight * CursY;
		}
		str++;
	}
	if (bufp != LineBuf)	/*������һ��*/
	{
		*bufp = '\0';
		if (isTextMode)
		{
			WORD *CurVm;

			CurVm = ((WORD*)GDIvm) + BufX + BufY * width;
			for (bufp = LineBuf; *bufp; bufp++, CurVm++)
				*CurVm = *(BYTE*)bufp | (TEXT_MODE_COLOR << 8);
		}
		else
		{
			GDIFillRect(BufX, BufY, GDICharWidth * (bufp - LineBuf), GDICharHeight, BgColor);
			GDIDrawStr(BufX, BufY, LineBuf, CharColor);
		}
	}
	if (isTextMode)
		SetTextCurs();
	else
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
}

/*�˸���*/
void BackSp()
{
	if (CursX == 0 && CursY == 0)
		return;
	if (!isTextMode)
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*������*/
	if (CursX)
		CursX--;
	else
	{
		CursX = width - 1;
		CursY--;
	}
	if (isTextMode)
	{
		((WORD*)GDIvm)[CursX + CursY * width] = (TEXT_MODE_COLOR << 8);
		SetTextCurs();
	}
	else
	{
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY, GDICharWidth, GDICharHeight, BgColor);	/*�������*/
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
	}
}

int main()
{
	long res;

	if ((res = KRegKnlPort(SRV_CUI_PORT)) != NO_ERROR)	/*ע�����˿ں�*/
		return res;
	res = GDIinit();
	if (res != NO_ERROR)
	{
		if (res == GDI_ERR_TEXTMODE)
		{
			GDICharHeight = GDICharWidth = 1;
			isTextMode = TRUE;
			width = GDIwidth;	/*������ʾ�ַ�����*/
			height = GDIheight;
			BgColor = 0;
			CharColor = 0x7;
		}
		else
			return res;
	}
	else
	{
		width = GDIwidth / GDICharWidth;	/*������ʾ�ַ�����*/
		height = GDIheight / GDICharHeight;
		BgColor = 0;
		CharColor = 0xFFFFFFFF;
	}
	ClearScr();
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_CUI:	/*��ͨ������Ϣ*/
			switch (data[MSG_API_ID] & MSG_API_MASK)
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
				if (isTextMode)
				{
					if (data[1] < width)
						CursX = data[1];
					if (data[2] < height)
						CursY = data[2];
					SetTextCurs();
				}
				else
				{
					GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*������*/
					if (data[1] < width)
						CursX = data[1];
					if (data[2] < height)
						CursY = data[2];
					GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*�����*/
				}
				break;
			case CUI_API_CLRSCR:	/*����*/
				ClearScr();
				break;
			case CUI_API_PUTC:		/*����ַ�*/
				if (data[1] == '\b')
					BackSp();
				else
				{
					data[1] &= 0xFF;
					PutStr((const char*)&data[1]);
				}
				break;
			}
			break;
		case MSG_ATTR_ROMAP:	/*ӳ����Ϣ*/
		case MSG_ATTR_RWMAP:
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case CUI_API_PUTS:	/*����ַ���*/
				if (((const char*)data[MSG_ADDR_ID])[data[MSG_SIZE_ID] - 1])
					data[MSG_RES_ID] = CUI_ERR_ARGS;
				else
				{
					PutStr((const char*)data[MSG_ADDR_ID]);
					data[MSG_RES_ID] = NO_ERROR;
				}
				break;
			}
			KUnmapProcAddr((void*)data[MSG_ADDR_ID], data);
		}
	}
	GDIrelease();
	KUnregKnlPort(SRV_CUI_PORT);
	return NO_ERROR;
}

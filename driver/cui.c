/*	cui.c for ulios driver
	作者：孙亮
	功能：字符用户界面驱动
	最后修改日期：2010-06-10
*/

#include "basesrv.h"
#include "../lib/gdi.h"

#define MAX_LINE	200	/*每行最多字符数量*/
#define CURS_WIDTH	2	/*光标高度*/

BOOL isTextMode;	/*是否文本模式*/
DWORD width, height;	/*显示字符数量*/
DWORD CursX, CursY;		/*光标位置*/
DWORD BgColor, CharColor;	/*当前背景,前景色*/

#define TEXT_MODE_COLOR	((CharColor & 0xF) | ((BgColor & 0xF) << 4))

/*字符模式设置光标*/
void SetTextCurs()
{
	DWORD PortData;

	PortData = CursX + CursY * width;
	outb(0x3D4, 0xF);
	outb(0x3D5, PortData & 0xFF);
	outb(0x3D4, 0xE);
	outb(0x3D5, (PortData >> 8));
}

/*清屏*/
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
		GDIFillRect(0, GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
	}
}

/*输出字符串*/
void PutStr(const char *str)
{
	DWORD BufX, BufY;	/*字符串写入位置*/
	char LineBuf[MAX_LINE], *bufp;	/*行缓冲*/

	bufp = LineBuf;
	BufX = GDICharWidth * CursX;
	BufY = GDICharHeight * CursY;
	while (*str)
	{
		switch (*str)
		{
		case '\n':
			if (!isTextMode)
				GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY, GDICharWidth, GDICharHeight, BgColor);	/*清除背景*/
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
		if (CursX >= width)	/*光标在最后一列，回到前面*/
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
		if (CursY >= height)	/*光标在最后一行，向上滚屏*/
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
	if (bufp != LineBuf)	/*输出最后一行*/
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
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
}

/*退格处理*/
void BackSp()
{
	if (CursX == 0 && CursY == 0)
		return;
	if (!isTextMode)
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
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
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY, GDICharWidth, GDICharHeight, BgColor);	/*清除背景*/
		GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
	}
}

int main()
{
	long res;

	if ((res = KRegKnlPort(SRV_CUI_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	res = GDIinit();
	if (res != NO_ERROR)
	{
		if (res == GDI_ERR_TEXTMODE)
		{
			GDICharHeight = GDICharWidth = 1;
			isTextMode = TRUE;
			width = GDIwidth;	/*计算显示字符数量*/
			height = GDIheight;
			BgColor = 0;
			CharColor = 0x7;
		}
		else
			return res;
	}
	else
	{
		width = GDIwidth / GDICharWidth;	/*计算显示字符数量*/
		height = GDIheight / GDICharHeight;
		BgColor = 0;
		CharColor = 0xFFFFFFFF;
	}
	ClearScr();
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		switch (data[MSG_ATTR_ID] & MSG_ATTR_MASK)
		{
		case MSG_ATTR_CUI:	/*普通服务消息*/
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case CUI_API_GETCOL:	/*取得字符界面颜色*/
				data[1] = CharColor;
				data[2] = BgColor;
				KSendMsg(&ptid, data, 0);
				break;
			case CUI_API_SETCOL:	/*设置字符界面颜色*/
				CharColor = data[1];
				BgColor = data[2];
				break;
			case CUI_API_GETCUR:	/*取得光标位置功能号*/
				data[1] = CursX;
				data[2] = CursY;
				KSendMsg(&ptid, data, 0);
				break;
			case CUI_API_SETCUR:	/*设置光标位置*/
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
					GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
					if (data[1] < width)
						CursX = data[1];
					if (data[2] < height)
						CursY = data[2];
					GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
				}
				break;
			case CUI_API_CLRSCR:	/*清屏*/
				ClearScr();
				break;
			case CUI_API_PUTC:		/*输出字符*/
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
		case MSG_ATTR_ROMAP:	/*映射消息*/
		case MSG_ATTR_RWMAP:
			switch (data[MSG_API_ID] & MSG_API_MASK)
			{
			case CUI_API_PUTS:	/*输出字符串*/
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

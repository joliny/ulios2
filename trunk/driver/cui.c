/*	cui.c for ulios driver
	作者：孙亮
	功能：字符用户界面驱动
	最后修改日期：2010-06-10
*/

#include "basesrv.h"

#define MAX_LINE	200	/*每行最多字符数量*/
#define CURS_WIDTH	2	/*光标高度*/

DWORD width, height;	/*显示字符数量*/
DWORD CursX, CursY;		/*光标位置*/
DWORD BgColor, CharColor;	/*当前背景,前景色*/

/*清屏*/
void ClearScr()
{
	CursY = CursX = 0;
	GDIFillRect(0, 0, GDIwidth, GDIheight, BgColor);
	GDIFillRect(0, GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
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
				GDIFillRect(BufX, BufY, GDICharWidth * (bufp - LineBuf), GDICharHeight, BgColor);
				GDIDrawStr(BufX, BufY, LineBuf, CharColor);
				bufp = LineBuf;
			}
			BufX = GDICharWidth * CursX;
			BufY = GDICharHeight * CursY;
		}
		if (CursY >= height)	/*光标在最后一行，向上滚屏*/
		{
			CursY--;
			GDIMoveUp(GDICharHeight);
			GDIFillRect(0, GDICharHeight * (height - 1), GDIwidth, GDICharHeight, BgColor);
			BufY = GDICharHeight * CursY;
		}
		str++;
	}
	if (bufp != LineBuf)	/*输出最后一行*/
	{
		*bufp = '\0';
		GDIFillRect(BufX, BufY, GDICharWidth * (bufp - LineBuf), GDICharHeight, BgColor);
		GDIDrawStr(BufX, BufY, LineBuf, CharColor);
	}
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
}

/*退格处理*/
void BackSp()
{
	if (CursX == 0 && CursY == 0)
		return;
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
	if (CursX)
		CursX--;
	else
	{
		CursX = width - 1;
		CursY--;
	}
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY, GDICharWidth, GDICharHeight, BgColor);	/*清除背景*/
	GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
}

int main()
{
	long res;

	if ((res = KRegKnlPort(SRV_CUI_PORT)) != NO_ERROR)	/*注册服务端口号*/
		return res;
	if ((res = GDIinit()) != NO_ERROR)
		return res;
	width = GDIwidth / GDICharWidth;	/*计算显示字符数量*/
	height = GDIheight / GDICharHeight;
	CursY = CursX = 0;
	BgColor = 0;
	CharColor = 0xFFFFFFFF;
	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if ((data[0] & 0xFFFF0000) == MSG_ATTR_USER)	/*普通服务消息*/
		{
			switch (data[3])
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
				GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, BgColor);	/*清除光标*/
				if (data[1] < width)
					CursX = data[1];
				if (data[2] < height)
					CursY = data[2];
				GDIFillRect(GDICharWidth * CursX, GDICharHeight * CursY + GDICharHeight - CURS_WIDTH, GDICharWidth, CURS_WIDTH, CharColor);	/*画光标*/
				break;
			case CUI_API_CLRSCR:	/*清屏*/
				ClearScr();
				break;
			case CUI_API_PUTC:		/*输出字符*/
				if (data[1] == '\b')
					BackSp();
				else
					PutStr((const char*)&data[1]);
				break;
			}
		}
		else if ((data[0] & 0xFFFF0000) == MSG_ATTR_MAP)	/*映射消息*/
		{
			switch (data[3])
			{
			case CUI_API_PUTS:	/*输出字符串*/
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

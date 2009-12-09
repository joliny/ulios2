/*	buffer.c for ulios
	作者：孙亮
	功能：高速磁盘缓冲管理，属于文件系统的底层功能
	最后修改日期：2009-06-09
*/

#include"../include/kernel.h"

#define DEVBLKSIZ	512		/*磁盘每块字节数*/
#define BUFBLKSIZ	4096	/*缓冲每块字节数*/
#define BMTlen	(BUFlen/blksiz)/*缓冲表长度*/

/*初始化缓冲区管理表*/
void InitBMT()
{
	DWORD i;
	wbfid = 0xffff;	/*表中全1表示结束*/
	rbfid=BMTlen-1;
	for (i=0;i<BMTlen;i++)	/*反向链接*/
	{
		bmt[i].sec=0xffffffff;
		bmt[i].flg=0;
		bmt[i].nxt=i-1;
	}
	bmt[0].nxt = INVALID;
}

/*缓冲读写,设备块大小必须为512字节*/
long RwBuf(DWORD dev, BOOL isWrite, DWORD fst, DWORD cou, BYTE *buf)
{
	DWORD bid;
	DWORD end;

	LOCK(bmt_l);
	for (bid = (fst >> 3), end = ((fst + cou) >> 3); bid < end; bid++, buf += BUFBLKSIZ)	/*fst当作计数器cou当作结束标志,逐个块处理*/
	{
		DWORD i, pre, pre2;	/*循环,前一项,更前一项*/
		BOOL isSwap = TRUE;	/*是否替换*/

		for (i = wbid; i != INVALID; pre2 = pre, pre = i, i = bmt[i].nxt)	/*检查脏链表*/
		if (bmt[i].BlkID == bid && bmt[i].DevID == dev)	/*缓冲区中存在*/
		{
			if (isWrite)	/*写入*/
				memcpy32(fsbuf + (i << 12), buf, BUFBLKSIZ / sizeof(DWORD));
			else	/*读出*/
				memcpy32(buf, fsbuf + (i << 12), BUFBLKSIZ / sizeof(DWORD));
			if (i != wbid)	/*不是第一项,需要调整*/
			{
				bmt[pre].nxt = bmt[i].nxt;	/*放到脏链开头*/
				bmt[i].nxt = wbid;
				wbid = i;
			}
			goto NextBlk;
		}
		for (i = rbid; i != INVALID; pre2 = pre, pre = i, i = bmt[i].nxt, isSwap = FALSE)	/*检查读链表*/
		if (bmt[i].BlkID == bid && bmt[i].DevID == dev)	/*缓冲区中存在*/
		{
			if (isWrite)	/*写入*/
			{
				memcpy32(fsbuf + (i << 12), buf, BUFBLKSIZ / sizeof(DWORD));
				if (i != rbid)
					bmt[pre].nxt = bmt[i].nxt;
				else
					rbid = bmt[i].nxt;
				bmt[i].flag = BUF_FLAG_DIRTY;	/*标识为脏*/
				bmt[i].nxt = wbid;	/*放到脏链开头*/
				wbid = i;
			}
			else	/*读出*/
			{
				memcpy32(buf, fsbuf + (i << 12), BUFBLKSIZ / sizeof(DWORD));
				if (i != rbid)	/*不是第一项,需要调整*/
				{
					bmt[pre].nxt = bmt[i].nxt;	/*放到读链开头*/
					bmt[i].nxt = rbid;
					rbid = i;
				}
			}
			goto NextBlk;
		}
		/*如果缓冲区中不存在就要处理设备了*/
		if (isSwap)	/*需要替换,不可丢弃*/
			RwDev(bmt[pre].DevID, TRUE, bmt[pre].BlkID, 8, fsbuf + (pre << 12));
		if (isWrite)	/*写入*/
		{
			memcpy32(fsbuf + (pre << 12), buf, BUFBLKSIZ / sizeof(DWORD));	/*写入缓冲*/
			bmt[pre].flag = BUF_FLAG_DIRTY;	/*标识为脏*/
			if (pre != rbid)	/*pre不是读区首项*/
				bmt[pre2].nxt = INVALID;
			else	/*f是首项,删除读区*/
				rbid = INVALID;
			bmt[pre].nxt = wbid;	/*描述符放到脏区开头*/
			wbid = pre;
		}
		else	/*读出*/
		{
			DWORD j;	/*提前读扇区计数*/
			/*最多提前读32块,遇到脏块退出,表尾退出,设备尾部退出*/
			for (j = 1; j < 32 && !bmt[pre + j].flag && pre + j < BmtLen && bid + j < hd[dev].cou; j++)
			{
				bmt[pre + j].BlkID = bid + j;
				bmt[pre + j].DevID = dev;
			}
			RwDev(dev, FALSE, bid, (j << 3), fsbuf + (pre << 12));	/*读取设备*/
			memcpy32(buf, fsbuf + (f << 12), BUFBLKSIZ / sizeof(DWORD));	/*复制*/
			bmt[pre].flag = 0;	/*去掉脏标志*/
			if (pre != rbid)
			{
				bmt[pre2].nxt = INVALID;
				bmt[pre].nxt = rbid;	/*描述符放到读区开头*/
				rbid = pre;
			}
		}
		bmt[pre].BlkID = bid;	/*修改描述符*/
		bmt[pre].DevID = dev;
NextBlk:
		continue;
	}
	ULOCK(bmt_l);
}

/*保存脏数据*/
long SaveBuf()
{
	cli();
	if (bmt_l)	/*此处不等待*/
	{
		sti();
		return 1;
	}
	bmt_l = 1;
	sti();
	if (wbid != INVALID)
	{
		DWORD i, j;

		for (i = 0; i < BMTlen;)
			if (bmt[i].flag)
			{
				for (j = 1; i + j < BMTlen && j <= 128; j++)	/*连续缓冲写,不脏,扇区不对应,设备号不对应都停止*/
				{
					if (!bmt[i + j].flag || bmt[i + j].BlkID != bmt[i].BlkID + j || bmt[i + j].DevID != bmt[i].DevID)
						break;
					bmt[i + j].flag = 0;	/*恢复不脏*/
				}
				RwDev(bmt[i].DevID, TRUE, bmt[i].BlkID, (j << 3), fsbuf + (i << 12));
				i += j;
			}
			else
				i++;
		i = wbid;
		do	/*取得脏链表尾部指针*/
			i = bmt[j = i].nxt;
		while (i != INVALID);
		bmt[j].nxt = rbid;	/*读链表接在写链表后面*/
		rbid = wbid;
		wbid = INVALID;	/*写链表清空*/
	}
	ULOCK(bmt_l);
	return 0;
}

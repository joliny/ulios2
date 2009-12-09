/*	buffer.c for ulios
	���ߣ�����
	���ܣ����ٴ��̻�����������ļ�ϵͳ�ĵײ㹦��
	����޸����ڣ�2009-06-09
*/

#include"../include/kernel.h"

#define DEVBLKSIZ	512		/*����ÿ���ֽ���*/
#define BUFBLKSIZ	4096	/*����ÿ���ֽ���*/
#define BMTlen	(BUFlen/blksiz)/*�������*/

/*��ʼ�������������*/
void InitBMT()
{
	DWORD i;
	wbfid = 0xffff;	/*����ȫ1��ʾ����*/
	rbfid=BMTlen-1;
	for (i=0;i<BMTlen;i++)	/*��������*/
	{
		bmt[i].sec=0xffffffff;
		bmt[i].flg=0;
		bmt[i].nxt=i-1;
	}
	bmt[0].nxt = INVALID;
}

/*�����д,�豸���С����Ϊ512�ֽ�*/
long RwBuf(DWORD dev, BOOL isWrite, DWORD fst, DWORD cou, BYTE *buf)
{
	DWORD bid;
	DWORD end;

	LOCK(bmt_l);
	for (bid = (fst >> 3), end = ((fst + cou) >> 3); bid < end; bid++, buf += BUFBLKSIZ)	/*fst����������cou����������־,����鴦��*/
	{
		DWORD i, pre, pre2;	/*ѭ��,ǰһ��,��ǰһ��*/
		BOOL isSwap = TRUE;	/*�Ƿ��滻*/

		for (i = wbid; i != INVALID; pre2 = pre, pre = i, i = bmt[i].nxt)	/*���������*/
		if (bmt[i].BlkID == bid && bmt[i].DevID == dev)	/*�������д���*/
		{
			if (isWrite)	/*д��*/
				memcpy32(fsbuf + (i << 12), buf, BUFBLKSIZ / sizeof(DWORD));
			else	/*����*/
				memcpy32(buf, fsbuf + (i << 12), BUFBLKSIZ / sizeof(DWORD));
			if (i != wbid)	/*���ǵ�һ��,��Ҫ����*/
			{
				bmt[pre].nxt = bmt[i].nxt;	/*�ŵ�������ͷ*/
				bmt[i].nxt = wbid;
				wbid = i;
			}
			goto NextBlk;
		}
		for (i = rbid; i != INVALID; pre2 = pre, pre = i, i = bmt[i].nxt, isSwap = FALSE)	/*��������*/
		if (bmt[i].BlkID == bid && bmt[i].DevID == dev)	/*�������д���*/
		{
			if (isWrite)	/*д��*/
			{
				memcpy32(fsbuf + (i << 12), buf, BUFBLKSIZ / sizeof(DWORD));
				if (i != rbid)
					bmt[pre].nxt = bmt[i].nxt;
				else
					rbid = bmt[i].nxt;
				bmt[i].flag = BUF_FLAG_DIRTY;	/*��ʶΪ��*/
				bmt[i].nxt = wbid;	/*�ŵ�������ͷ*/
				wbid = i;
			}
			else	/*����*/
			{
				memcpy32(buf, fsbuf + (i << 12), BUFBLKSIZ / sizeof(DWORD));
				if (i != rbid)	/*���ǵ�һ��,��Ҫ����*/
				{
					bmt[pre].nxt = bmt[i].nxt;	/*�ŵ�������ͷ*/
					bmt[i].nxt = rbid;
					rbid = i;
				}
			}
			goto NextBlk;
		}
		/*����������в����ھ�Ҫ�����豸��*/
		if (isSwap)	/*��Ҫ�滻,���ɶ���*/
			RwDev(bmt[pre].DevID, TRUE, bmt[pre].BlkID, 8, fsbuf + (pre << 12));
		if (isWrite)	/*д��*/
		{
			memcpy32(fsbuf + (pre << 12), buf, BUFBLKSIZ / sizeof(DWORD));	/*д�뻺��*/
			bmt[pre].flag = BUF_FLAG_DIRTY;	/*��ʶΪ��*/
			if (pre != rbid)	/*pre���Ƕ�������*/
				bmt[pre2].nxt = INVALID;
			else	/*f������,ɾ������*/
				rbid = INVALID;
			bmt[pre].nxt = wbid;	/*�������ŵ�������ͷ*/
			wbid = pre;
		}
		else	/*����*/
		{
			DWORD j;	/*��ǰ����������*/
			/*�����ǰ��32��,��������˳�,��β�˳�,�豸β���˳�*/
			for (j = 1; j < 32 && !bmt[pre + j].flag && pre + j < BmtLen && bid + j < hd[dev].cou; j++)
			{
				bmt[pre + j].BlkID = bid + j;
				bmt[pre + j].DevID = dev;
			}
			RwDev(dev, FALSE, bid, (j << 3), fsbuf + (pre << 12));	/*��ȡ�豸*/
			memcpy32(buf, fsbuf + (f << 12), BUFBLKSIZ / sizeof(DWORD));	/*����*/
			bmt[pre].flag = 0;	/*ȥ�����־*/
			if (pre != rbid)
			{
				bmt[pre2].nxt = INVALID;
				bmt[pre].nxt = rbid;	/*�������ŵ�������ͷ*/
				rbid = pre;
			}
		}
		bmt[pre].BlkID = bid;	/*�޸�������*/
		bmt[pre].DevID = dev;
NextBlk:
		continue;
	}
	ULOCK(bmt_l);
}

/*����������*/
long SaveBuf()
{
	cli();
	if (bmt_l)	/*�˴����ȴ�*/
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
				for (j = 1; i + j < BMTlen && j <= 128; j++)	/*��������д,����,��������Ӧ,�豸�Ų���Ӧ��ֹͣ*/
				{
					if (!bmt[i + j].flag || bmt[i + j].BlkID != bmt[i].BlkID + j || bmt[i + j].DevID != bmt[i].DevID)
						break;
					bmt[i + j].flag = 0;	/*�ָ�����*/
				}
				RwDev(bmt[i].DevID, TRUE, bmt[i].BlkID, (j << 3), fsbuf + (i << 12));
				i += j;
			}
			else
				i++;
		i = wbid;
		do	/*ȡ��������β��ָ��*/
			i = bmt[j = i].nxt;
		while (i != INVALID);
		bmt[j].nxt = rbid;	/*���������д�������*/
		rbid = wbid;
		wbid = INVALID;	/*д�������*/
	}
	ULOCK(bmt_l);
	return 0;
}

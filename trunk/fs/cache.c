/*	cache.c for ulios file system
	���ߣ�����
	���ܣ����ٻ�����������ļ�ϵͳ�ĵײ㹦��
	����޸����ڣ�2009-06-09
*/

#include "fs.h"

CACHE_DESC bmt[BMT_LEN];
void *cache;
DWORD cahl;	/*���������*/

/*�����д*/
long RwCache(DWORD drv, BOOL isWrite, DWORD sec, DWORD cou, void *buf)
{
	lock(&cahl);
	for (cou += sec; sec < cou; sec++, buf += BLK_SIZ)	/*cou����������ID*/
	{
		CACHE_DESC *CurBmd;
		void *cachep;

		CurBmd = &bmt[sec & BLKATTR_MASK];
		cachep = cache + ((sec & BLKATTR_MASK) << BLK_SHIFT);
		if ((CurBmd->BlkID >> BLKID_SHIFT) == (sec >> BLKID_SHIFT) && CurBmd->DrvID == drv)	/*�������д���*/
		{
			if (isWrite)	/*д��*/
			{
				memcpy32(cachep, buf, BLK_SIZ / sizeof(DWORD));
				CurBmd->BlkID |= CACHE_ATTR_DIRTY;	/*���Ϊ��*/
			}
			else	/*����*/
				memcpy32(buf, cachep, BLK_SIZ / sizeof(DWORD));
			if ((CurBmd->BlkID & CACHE_ATTR_TIMES) < CACHE_ATTR_TIMES)	/*���ʴ�������*/
				CurBmd->BlkID++;
		}
		else	/*�������в����ھ�Ҫ�����豸*/
		{
			long res;
			CACHE_DESC *PreBmd;	/*Ԥ�ȴ����*/

			if (CurBmd->BlkID & CACHE_ATTR_DIRTY)	/*��ǰ�������,���ɶ���,ɨ���Ԥ�ȱ�������*/
			{
				for (PreBmd = CurBmd + 1; PreBmd - CurBmd < MAXPROC_COU && PreBmd < &bmt[BMT_LEN] && (PreBmd->BlkID & CACHE_ATTR_DIRTY) && (PreBmd->BlkID >> BLKID_SHIFT) == (CurBmd->BlkID >> BLKID_SHIFT) && PreBmd->DrvID == CurBmd->DrvID; PreBmd++)
					PreBmd->BlkID &= ~CACHE_ATTR_DIRTY;
				if ((res = HDWriteSector(CurBmd->DrvID, (CurBmd->BlkID & BLKID_MASK) | (sec & BLKATTR_MASK), PreBmd - CurBmd, cachep)) != NO_ERROR)	/*�������*/
				{
					ulock(&cahl);
					return res;
				}
			}
			if (isWrite)	/*д����,��д�뻺��*/
			{
				memcpy32(cachep, buf, BLK_SIZ / sizeof(DWORD));
				CurBmd->BlkID = (sec & BLKID_MASK) | CACHE_ATTR_DIRTY | 1;	/*���Ϊ���ʹ�1�ε����*/
			}
			else	/*������,���뻺�沢����*/
			{
				for (PreBmd = CurBmd + 1; PreBmd - CurBmd < MAXPROC_COU && PreBmd < &bmt[BMT_LEN] && !(PreBmd->BlkID & CACHE_ATTR_DIRTY) && (PreBmd->BlkID & CACHE_ATTR_TIMES) <= 1; PreBmd++)
				{
					PreBmd->DrvID = drv;
					PreBmd->BlkID = sec & BLKID_MASK;
				}
				if ((res = HDReadSector(drv, (sec & BLKID_MASK) | (sec & BLKATTR_MASK), PreBmd - CurBmd, cachep)) != NO_ERROR)	/*Ԥ��ȡ*/
				{
					ulock(&cahl);
					return res;
				}
				memcpy32(buf, cachep, BLK_SIZ / sizeof(DWORD));
				CurBmd->BlkID = (sec & BLKID_MASK) | 1;
			}
			CurBmd->DrvID = drv;
		}
	}
	ulock(&cahl);
	return NO_ERROR;
}

/*����������*/
long SaveCache()
{
	long res;
	CACHE_DESC *CurBmd, *PreBmd;

	lock(&cahl);
	for (CurBmd = bmt; CurBmd < &bmt[BMT_LEN];)
	{
		if (CurBmd->BlkID & CACHE_ATTR_DIRTY)
		{
			for (PreBmd = CurBmd + 1; PreBmd - CurBmd < MAXPROC_COU && PreBmd < &bmt[BMT_LEN] && (PreBmd->BlkID & CACHE_ATTR_DIRTY) && (PreBmd->BlkID >> BLKID_SHIFT) == (CurBmd->BlkID >> BLKID_SHIFT) && PreBmd->DrvID == CurBmd->DrvID; PreBmd++)
				PreBmd->BlkID &= ~CACHE_ATTR_DIRTY;
			if ((res = HDWriteSector(CurBmd->DrvID, (CurBmd->BlkID & BLKID_MASK) | (CurBmd - bmt), PreBmd - CurBmd, cache + ((CurBmd - bmt) << BLK_SHIFT))) != NO_ERROR)	/*�������*/
			{
				ulock(&cahl);
				return res;
			}
			CurBmd->BlkID &= ~CACHE_ATTR_DIRTY;
			CurBmd = PreBmd;
		}
		else
			CurBmd++;
	}
	ulock(&cahl);
	return NO_ERROR;
}

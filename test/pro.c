/*	pro.c
	���ߣ�����
	���ܣ���ִ���ļ����Գ���
	����޸����ڣ�2010-05-14
*/

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"

int main()
{
	THREAD_ID VesaPtid;
	long res;

	if ((res = KGetKpToThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	KSleep(1000);
	VSDrawStr(VesaPtid, 100, 100, (const BYTE*)"���������Ұ��㣬�������󰮴���", 0xFFFFFF);
	return NO_ERROR;
}

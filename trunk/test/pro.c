/*	pro.c
	���ߣ�����
	���ܣ���ִ���ļ����Գ���
	����޸����ڣ�2010-05-14
*/

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"

int main(char *args)
{
	THREAD_ID VesaPtid;
	long res;

	KSleep(500);
	if ((res = KGetKptThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	VSDrawStr(VesaPtid, 200, 180, "hello:", 0xFFFF00);
	VSDrawStr(VesaPtid, 200, 200, args, 0xFFFF00);
	return NO_ERROR;
}

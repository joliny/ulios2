/*	pro.c
	���ߣ�����
	���ܣ���ִ���ļ����Գ���
	����޸����ڣ�2010-05-14
*/

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"

int main(int argc, char *argv[])
{
	THREAD_ID VesaPtid;
	long res;

	KSleep(300);
	if ((res = KGetKptThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;
	for (res = 1; res < argc; res++)
		VSDrawStr(VesaPtid, 200, res * 20, argv[res], 0xFFFF00);
	return NO_ERROR;
}

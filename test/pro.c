/*	pro.c
	���ߣ�����
	���ܣ���ִ���ļ����Գ���
	����޸����ڣ�2010-05-14
*/

#include "../driver/basesrv.h"

int main(int argc, char *argv[])
{
	THREAD_ID ptid;
	long res;

	KSleep(300);
	if ((res = KGetKptThed(SRV_CUI_PORT, &ptid)) != NO_ERROR)
		return res;
	CUISetCur(ptid, 10, 24);
	CUIPutS(ptid, "This is a args test pro!");
	for (res = 0; res < 10; res++)
		CUIPutS(ptid, "���ţ�CUI�㶨�� ����\n");
	return NO_ERROR;
}

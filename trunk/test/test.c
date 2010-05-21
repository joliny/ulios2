/*	test.c
	作者：孙亮
	功能：ulios系统内核测试程序
	最后修改日期：2009-05-28
*/

#include "../driver/basesrv.h"

int main()
{
	THREAD_ID VesaPtid, ptid;
	long res;

	if ((res = KGetKpToThed(SRV_KBDMUS_PORT, &ptid)) != NO_ERROR)
		return res;
	if ((res = KMSetRecv(ptid)) != NO_ERROR)
		return res;
	if ((res = KGetKpToThed(SRV_VESA_PORT, &VesaPtid)) != NO_ERROR)
		return res;

	for (;;)
	{
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)
			break;
		if (data[0] == MSG_ATTR_KBD)
		{
/*			if (data[1] & KBD_STATE_LSHIFT)
				KPrintf("LSHIFT\t", 0);
			if (data[1] & KBD_STATE_RSHIFT)
				KPrintf("RSHIFT\t", 0);
			if (data[1] & KBD_STATE_LCTRL)
				KPrintf("LCTRL\t", 0);
			if (data[1] & KBD_STATE_RCTRL)
				KPrintf("RCTRL\t", 0);
			if (data[1] & KBD_STATE_LALT)
				KPrintf("LALT\t", 0);
			if (data[1] & KBD_STATE_RALT)
				KPrintf("RALT\t", 0);
			if (data[1] & KBD_STATE_PAUSE)
				KPrintf("PAUSE\t", 0);
			if (data[1] & KBD_STATE_PRTSC)
				KPrintf("PRTSC\t", 0);
			if (data[1] & KBD_STATE_SCRLOCK)
				KPrintf("SCRLOCK\t", 0);
			if (data[1] & KBD_STATE_NUMLOCK)
				KPrintf("NUMLOCK\t", 0);
			if (data[1] & KBD_STATE_CAPSLOCK)
				KPrintf("CAPSLOCK\t", 0);
			if (data[1] & KBD_STATE_INSERT)
				KPrintf("INSERT\t", 0);
			KPrintf("SCAN:%d\t", (data[1] >> 8) & 0xFF);
			KPrintf("CHAR:%c\n", data[1] & 0xFF);
*/		}
		if (data[0] == MSG_ATTR_MUS)
		{
			DWORD col;
/*			if (data[1] & MUS_STATE_LBUTTON)
				KPrintf("LBTN\t", 0);
			if (data[1] & MUS_STATE_RBUTTON)
				KPrintf("RBTN\t", 0);
			if (data[1] & MUS_STATE_MBUTTON)
				KPrintf("MBTN\t", 0);
			KPrintf("X:%d\t", data[2]);
			KPrintf("Y:%d\t", data[3]);
			KPrintf("Z:%d\n", data[4]);
*/			if (data[1] & MUS_STATE_LBUTTON)
				col = 0xFF;
			else if (data[1] & MUS_STATE_RBUTTON)
				col = 0xFF00;
			else if (data[1] & MUS_STATE_MBUTTON)
				col = 0xFF0000;
			else
				col = 0xFFFFFF;
			VSDrawStr(VesaPtid, data[2], data[3], "老婆老婆我爱你，就像老鼠爱大米", col);
		}
	}
	return NO_ERROR;
}

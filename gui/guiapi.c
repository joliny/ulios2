/*	guiapi.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面接口，响应应用程序的请求，执行服务
	最后修改日期：2010-10-05
*/

#include "gui.h"

THREAD_ID KbdMusPtid;	/*键盘鼠标服务ID*/

/*初始化GUI,如果不成功必须退出*/
long InitGUI()
{
	long res;
	void *addr;

	if ((res = KRegKnlPort(SRV_GUI_PORT)) != NO_ERROR)	/*注册服务端口*/
		return res;
	if ((res = GDIinit()) != NO_ERROR)	/*初始化GDI*/
		return res;
	if ((res = KGetKptThed(SRV_KBDMUS_PORT, &KbdMusPtid)) != NO_ERROR)	/*取得键盘鼠标服务线程*/
		return res;
	if ((res = KMapUserAddr(&addr, FDAT_SIZ + VDAT_SIZ)) != NO_ERROR)	/*申请自由内存和可视内存空间*/
		return res;
	InitFbt(fmt, FMT_LEN, addr, FDAT_SIZ);
	fmtl = FALSE;
	InitFbt(vmt, VMT_LEN, addr + FDAT_SIZ, VDAT_SIZ);
	vmtl = FALSE;
	return NO_ERROR;
}

int main()
{
	long res;

	if ((res = InitGUI()) != NO_ERROR)
		return res;
	for (;;)
	{
		DWORD data[MSG_DATA_LEN];
		THREAD_ID ptid;

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
	}

	KUnregKnlPort(SRV_GUI_PORT);
	return NO_ERROR;
}

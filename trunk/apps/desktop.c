/*	desktop.c for ulios application
	作者：孙亮
	功能：桌面应用进程
	最后修改日期：2011-05-21
*/

#include "../fs/fsapi.h"
#include "../lib/malloc.h"
#include "../lib/gclient.h"

void BtnProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	KCreateProcess(0, "guitest.bin", NULL, &ptid);
}

/*桌面消息处理函数*/
long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	CTRL_DSK *dsk = (CTRL_DSK*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		GCDskDefMsgProc(ptid, data);
		GCLoadBmp("desktop.bmp", dsk->obj.uda.vbuf, dsk->obj.uda.width * dsk->obj.uda.height, NULL, NULL);
		{
			CTRL_ARGS args;
			args.width = 80;
			args.height = 20;
			args.x = 16;
			args.y = 16;
			args.style = 0;
			args.MsgProc = NULL;
			GCBtnCreate(NULL, &args, dsk->obj.gid, &dsk->obj, "启动guitest", BtnProc);
		}
		return NO_ERROR;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_DSK *dsk;
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = GCwidth;
	args.height = GCheight;
	args.x = 0;
	args.y = 0;
	args.style = 0;
	args.MsgProc = MainMsgProc;
	GCDskCreate(&dsk, &args, 0, NULL);

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == GC_ERR_INVALID_GUIMSG)	/*非GUI消息另行处理*/
			;
		else if ((data[MSG_ATTR_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)dsk)	/*销毁主窗体时重建桌面*/
			GCDskCreate(&dsk, &args, 0, NULL);
	}
	return NO_ERROR;
}

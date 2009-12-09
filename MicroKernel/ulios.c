/*	ulios.c for ulios
	作者：孙亮
	功能：ulios系统的c入口代码，调用微内核初始化函数，启动核心服务
	最后修改日期：2009-05-28
*/

#include "knldef.h"

int main()
{
	long res;

	InitKnlVal();		/*内核变量*/
	InitKFMT();			/*内核内存管理*/
	res = InitPMM();	/*物理内存管理*/
	if (res != NO_ERROR)
		return res;
	res = InitMsg();	/*消息管理*/
	if (res != NO_ERROR)
		return res;
	InitEXMT();			/*可执行体管理*/
	InitPMT();			/*进程管理*/
	InitKnlProc();		/*内核进程*/
	InitINTR();			/*中断处理*/
	res = InitBaseSrv();/*基础服务*/
	if (res != NO_ERROR)
		return res;
	return NO_ERROR;	/*内核初始化完成*/
}

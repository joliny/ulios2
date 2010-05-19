/*	exec.h for ulios
	作者：孙亮
	功能：可执行体管理，进程线程初始化定义
	最后修改日期：2009-09-01
*/

#ifndef _EXEC_H_
#define _EXEC_H_

#include "ulidef.h"

#define EXEC_ARGV_BASESRV	0x00000001	/*0:文件进程1:基础服务*/
#define EXEC_ARGV_DRIVER	0x00000002	/*0:应用进程1:驱动进程*/
#define FS_KPORT			0			/*文件系统线程端口*/
#define FS_API_GETEXEC		0			/*请求文件系统取得可执行文件信息功能号*/
#define FS_API_READPAGE		1			/*请求文件系统读取可执行文件页功能号*/
#define FS_OUT_TIME			6000		/*文件系统超时时间*/

typedef struct _EXEC_DESC
{
	WORD id, cou;	/*可执行体ID,使用计数*/
	void *CodeOff;	/*代码段开始地址*/
	void *CodeEnd;	/*代码段结束地址*/
	DWORD CodeSeek;	/*代码段文件偏移*/
	void *DataOff;	/*数据段开始地址*/
	void *DataEnd;	/*数据段结束地址*/
	DWORD DataSeek;	/*数据段文件偏移*/
	void *entry;	/*入口点*/
	volatile DWORD Page_l;	/*分页管理锁*/
}EXEC_DESC;	/*可执行体结构*/

/*初始化可执行体管理表*/
void InitEXMT();

/*分配空可执行体ID*/
long AllocExid(EXEC_DESC *exec);

/*释放空可执行体ID*/
void FreeExid(EXEC_DESC **exmd);

/*线程起点*/
void ThedStart();

/*进程起点*/
void ProcStart();

/*删除线程资源并退出*/
void ThedExit();

#endif

/*	global.c for ulios
	作者：孙亮
	功能：全局变量及公用函数定义
	最后修改日期：2009-05-26
*/

#include "knldef.h"

/**********内核数据表**********/

/*保护模式相关表*/
SEG_GATE_DESC gdt[GDT_LEN];	/*全局描述符表2KB*/
SEG_GATE_DESC idt[IDT_LEN];	/*中断描述符表2KB*/
PAGE_DESC kpdt[PDT_LEN];	/*内核页目录表4KB*/
PAGE_DESC pddt[PDT_LEN];	/*页目录表的目录表4KB*/
PAGE_DESC pddt0[PDT_LEN];	/*页目录表副本的目录表4KB*/
/*内核管理表*/
FREE_BLK_DESC kmmt[FMT_LEN];/*内核自由数据管理表12KB(0项不用)*/
PROCESS_DESC* pmt[PMT_LEN];	/*进程管理表4KB*/
EXEC_DESC* exmt[EXMT_LEN];	/*可执行体管理表4KB*/
THREAD_ID kpt[KPT_LEN];		/*内核端口注册表4KB*/

/**********内核零散变量**********/

BYTE KnlValue[KVAL_LEN];	/*内核零散变量区4KB*/

/**********实模式设置的变量**********/

PHYBLK_DESC BaseSrv[16];	/*8 * 16字节基础服务程序段表*/
DWORD VesaMode;			/*启动VESA模式号*/
DWORD VesaInfo[64];		/*VESA模式信息*/
DWORD HdInfo[8];		/*两个硬盘参数*/
DWORD MemEnd;			/*内存上限*/
MEM_ARDS ards[1];		/*20 * N字节内存结构体*/

/**********内核大块数据区**********/

BYTE gramem[0x20000];	/*图形显存128K*/
BYTE txtmem[0x8000];	/*文本显存32K*/
BYTE kdat[KDAT_SIZ];	/*内核自由数据*/

/**********分页机制相关表**********/

/*占用24M虚拟地址空间*/
PAGE_DESC pdt[PT_LEN];		/*所有进程页目录表4MB*/
PAGE_DESC pdt0[PT_LEN];		/*所有进程副本页目录表4MB*/
PAGE_DESC pt[PT_LEN];		/*当前进程页表4MB*/
PAGE_DESC pt0[PT_LEN];		/*当前进程副本页表4MB*/
PAGE_DESC pt2[PT_LEN];		/*关系进程页表4MB*/
BYTE pg0[PG_LEN];			/*当前页副本4MB*/

/*初始化内核变量*/
void InitKnlVal()
{
	memset32(kpt, INVALID, sizeof(kpt) / sizeof(DWORD));	/*初始化内核端口注册表*/
	memset32(KnlValue, 0, sizeof(KnlValue) / sizeof(DWORD));	/*清空零散变量*/
}

/*初始化基础服务*/
long InitBaseSrv()
{
	long res;
	PHYBLK_DESC *CurSeg;
	DWORD argv[3];

	argv[0] = EXEC_ARGV_BASESRV | EXEC_ARGV_DRIVER;
	for (CurSeg = &BaseSrv[1]; CurSeg->addr; CurSeg++)
	{
		argv[1] = CurSeg->addr;
		argv[2] = CurSeg->siz;
		if ((res = CreateProc(argv, (THREAD_ID*)&argv[1])) != NO_ERROR)
			return res;
	}
	return NO_ERROR;
}

/*注册内核端口对应线程*/
long RegKnlPort(DWORD PortN)
{
	if (PortN >= KPT_LEN)
		return ERROR_WRONG_KPTN;
	cli();
	if (*(DWORD*)(&kpt[PortN]) != INVALID)
	{
		sti();
		return ERROR_KPT_ISENABLED;
	}
	kpt[PortN] = CurPmd->CurTmd->id;
	sti();
	return NO_ERROR;
}

/*注销内核端口对应线程*/
long UnregKnlPort(DWORD PortN)
{
	if (PortN >= KPT_LEN)
		return ERROR_WRONG_KPTN;
	cli();
	if (*(DWORD*)(&kpt[PortN]) == INVALID)
	{
		sti();
		return ERROR_KPT_ISDISABLED;
	}
	if (kpt[PortN].ProcID != CurPmd->CurTmd->id.ProcID)
	{
		sti();
		return ERROR_KPT_WRONG_CURPROC;
	}
	*(DWORD*)(&kpt[PortN]) = INVALID;
	sti();
	return NO_ERROR;
}

/*取得内核端口对应线程*/
long GetKpToThed(DWORD PortN, THREAD_ID *ptid)
{
	if (PortN >= KPT_LEN)
		return ERROR_WRONG_KPTN;
	cli();
	if (*(DWORD*)(&kpt[PortN]) == INVALID)
	{
		sti();
		return ERROR_KPT_ISDISABLED;
	}
	*ptid = kpt[PortN];
	sti();
	return NO_ERROR;
}

/*注销线程的所有内核端口*/
long UnregAllKnlPort()
{
	THREAD_ID ptid;
	DWORD i;

	ptid = CurPmd->CurTmd->id;
	cli();
	for (i = 0; i < KPT_LEN; i++)
		if (*(DWORD*)(&kpt[i]) == *(DWORD*)(&ptid))
			*(DWORD*)(&kpt[i]) = INVALID;
	sti();
	return NO_ERROR;
}

/*	debug.h for ulios
	作者：孙亮
	功能：内核调试相关定义
	最后修改日期：2009-05-29
*/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "ulidef.h"

#define DebugMsg	Print		/*调试信息*/

/*输出字符*/
void PutChar(char c);

/*输出数值*/
void PutNum(DWORD n, DWORD r);

/*输出字符窗*/
void PutS(const char *str);

/*格式化输出*/
void Print(const char *fmtstr, ...);

/*格式化输出到指定位置*/
void XYPrint(DWORD x, DWORD y, const char *fmtstr, ...);

#endif

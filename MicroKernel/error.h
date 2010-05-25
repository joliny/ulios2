/*	error.h for ulios
	作者：孙亮
	功能：定义错误代码
	最后修改日期：2009-05-29
*/

#ifndef	_ERROR_H_
#define	_ERROR_H_

#define NO_ERROR					0	/*无错*/

#define ERROR_WRONG_APIN			-1	/*错误的系统调用号*/
#define ERROR_WRONG_THEDID			-2	/*错误的线程ID*/
#define ERROR_WRONG_PROCID			-3	/*错误的进程ID*/
#define ERROR_WRONG_KPTN			-4	/*错误的内核端口号*/
#define ERROR_WRONG_IRQN			-5	/*错误的IRQ号*/
#define ERROR_WRONG_APPMSG			-6	/*错误的应用程序消息*/

#define ERROR_HAVENO_KMEM			-7	/*没有内核内存*/
#define ERROR_HAVENO_PMEM			-8	/*没有物理内存*/
#define ERROR_HAVENO_THEDID			-9	/*没有线程管理节点*/
#define ERROR_HAVENO_PROCID			-10	/*没有进程管理节点*/
#define ERROR_HAVENO_EXECID			-11	/*没有可执行体描述符*/
#define ERROR_HAVENO_MSGDESC		-12	/*没有消息描述符*/
#define ERROR_HAVENO_LINEADDR		-13	/*没有线性地址空间*/

#define ERROR_KPT_ISENABLED			-14	/*内核端口已经被注册*/
#define ERROR_KPT_ISDISABLED		-15	/*内核端口没有被注册*/
#define ERROR_KPT_WRONG_CURPROC		-16	/*当前进程无法改动内核端口*/
#define ERROR_IRQ_ISENABLED			-17	/*IRQ已经开启*/
#define ERROR_IRQ_ISDISABLED		-18	/*IRQ已经关闭*/
#define ERROR_IRQ_WRONG_CURPROC		-19	/*当前线程无法改动IRQ*/

#define ERROR_NOT_DRIVER			-20	/*非法执行驱动API*/
#define ERROR_INVALID_ADDR			-21	/*无效的地址*/
#define ERROR_INVALID_MAPADDR		-22	/*非法的映射地址*/
#define ERROR_INVALID_MAPSIZE		-23	/*非法的映射大小*/

#define ERROR_OUT_OF_TIME			-24	/*超时错误*/
#define ERROR_PROC_EXCEP			-25	/*程序异常*/
#define ERROR_THED_KILLED			-26	/*线程被杀死*/

#endif

/*	error.h for ulios
	作者：孙亮
	功能：定义错误代码
	最后修改日期：2009-05-29
*/

#ifndef	_ERROR_H_
#define	_ERROR_H_

#define NO_ERROR					0	/*无错*/

/*内核资源错误*/
#define KERR_OUT_OF_KNLMEM			-1	/*内核内存不足*/
#define KERR_OUT_OF_PHYMEM			-2	/*物理内存不足*/
#define KERR_OUT_OF_LINEADDR		-3	/*线性地址空间不足*/

/*进程线程错误*/
#define KERR_INVALID_PROCID			-4	/*非法进程ID*/
#define KERR_PROC_NOT_EXIST			-5	/*进程不存在*/
#define KERR_PROC_NOT_ENOUGH		-6	/*进程管理结构不足*/
#define KERR_INVALID_THEDID			-7	/*非法线程ID*/
#define KERR_THED_NOT_EXIST			-8	/*线程不存在*/
#define KERR_THED_NOT_ENOUGH		-9	/*线程管理结构不足*/

/*内核注册表错误*/
#define KERR_INVALID_KPTNUN			-10	/*非法内核端口号*/
#define KERR_KPT_ALREADY_REGISTERED	-11	/*内核端口已被注册*/
#define KERR_KPT_NOT_REGISTERED		-12	/*内核端口未被注册*/
#define KERR_INVALID_IRQNUM			-13	/*非法IRQ号*/
#define KERR_IRQ_ALREADY_REGISTERED	-14	/*IRQ已被注册*/
#define KERR_IRQ_NOT_REGISTERED		-15	/*IRQ未被注册*/
#define KERR_CURPROC_NOT_REGISTRANT	-16	/*当前进程不是注册者*/

/*消息错误*/
#define KERR_INVALID_USERMSG_ATTR	-17	/*非法用户消息属性*/
#define KERR_MSG_NOT_ENOUGH			-18	/*消息结构不足*/
#define KERR_MSG_QUEUE_FULL			-19	/*消息队列满*/
#define KERR_MSG_QUEUE_EMPTY		-20	/*消息队列空*/

/*地址映射错误*/
#define KERR_MAPSIZE_IS_ZERO		-21	/*映射长度为0*/
#define KERR_MAPSIZE_TOO_LONG		-22	/*映射长度太大*/
#define KERR_PROC_SELF_MAPED		-23	/*映射进程自身*/
#define KERR_ILLEGAL_PHYADDR_MAPED	-24	/*映射不允许的物理地址*/
#define KERR_ADDRARGS_NOT_FOUND		-25	/*地址参数未找到*/

/*程序执行错误*/
#define KERR_OUT_OF_TIME			-26	/*超时错误*/
#define KERR_ACCESS_ILLEGAL_ADDR	-27	/*访问非法地址*/
#define KERR_WRITE_RDONLY_ADDR		-28	/*写只读地址*/
#define KERR_THED_EXCEPTION			-29	/*线程执行异常*/
#define KERR_THED_KILLED			-30	/*线程被杀死*/

/*调用错误*/
#define KERR_INVALID_APINUM			-31	/*非法系统调用号*/
#define KERR_ARGS_TOO_LONG			-32	/*参数字串超长*/
#define KERR_INVALID_MEMARGS_ADDR	-33	/*非法内存参数地址*/
#define KERR_NO_DRIVER_PRIVILEGE	-34	/*没有执行驱动功能的特权*/

#endif

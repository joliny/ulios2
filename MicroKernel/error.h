/*	error.h for ulios
	���ߣ�����
	���ܣ�����������
	����޸����ڣ�2009-05-29
*/

#ifndef	_ERROR_H_
#define	_ERROR_H_

#define NO_ERROR					0	/*�޴�*/

/*�ں���Դ����*/
#define KERR_OUT_OF_KNLMEM			-1	/*�ں��ڴ治��*/
#define KERR_OUT_OF_PHYMEM			-2	/*�����ڴ治��*/
#define KERR_OUT_OF_LINEADDR		-3	/*���Ե�ַ�ռ䲻��*/

/*�����̴߳���*/
#define KERR_INVALID_PROCID			-4	/*�Ƿ�����ID*/
#define KERR_PROC_NOT_EXIST			-5	/*���̲�����*/
#define KERR_PROC_NOT_ENOUGH		-6	/*���̹���ṹ����*/
#define KERR_PROC_KILL_SELF			-7	/*ɱ����������*/
#define KERR_INVALID_THEDID			-8	/*�Ƿ��߳�ID*/
#define KERR_THED_NOT_EXIST			-9	/*�̲߳�����*/
#define KERR_THED_NOT_ENOUGH		-10	/*�̹߳���ṹ����*/
#define KERR_THED_KILL_SELF			-11	/*ɱ���߳�����*/

/*�ں�ע������*/
#define KERR_INVALID_KPTNUN			-12	/*�Ƿ��ں˶˿ں�*/
#define KERR_KPT_ALREADY_REGISTERED	-13	/*�ں˶˿��ѱ�ע��*/
#define KERR_KPT_NOT_REGISTERED		-14	/*�ں˶˿�δ��ע��*/
#define KERR_INVALID_IRQNUM			-15	/*�Ƿ�IRQ��*/
#define KERR_IRQ_ALREADY_REGISTERED	-16	/*IRQ�ѱ�ע��*/
#define KERR_IRQ_NOT_REGISTERED		-17	/*IRQδ��ע��*/
#define KERR_CURPROC_NOT_REGISTRANT	-18	/*��ǰ���̲���ע����*/

/*��Ϣ����*/
#define KERR_INVALID_USERMSG_ATTR	-19	/*�Ƿ��û���Ϣ����*/
#define KERR_MSG_NOT_ENOUGH			-20	/*��Ϣ�ṹ����*/
#define KERR_MSG_QUEUE_FULL			-21	/*��Ϣ������*/
#define KERR_MSG_QUEUE_EMPTY		-22	/*��Ϣ���п�*/

/*��ַӳ�����*/
#define KERR_MAPSIZE_IS_ZERO		-23	/*ӳ�䳤��Ϊ0*/
#define KERR_MAPSIZE_TOO_LONG		-24	/*ӳ�䳤��̫��*/
#define KERR_PROC_SELF_MAPED		-25	/*ӳ���������*/
#define KERR_PAGE_ALREADY_MAPED		-26	/*Ŀ�����ҳ���Ѿ���ӳ��*/
#define KERR_ILLEGAL_PHYADDR_MAPED	-27	/*ӳ�䲻����������ַ*/
#define KERR_ADDRARGS_NOT_FOUND		-28	/*��ַ����δ�ҵ�*/

/*����ִ�д���*/
#define KERR_OUT_OF_TIME			-29	/*��ʱ����*/
#define KERR_ACCESS_ILLEGAL_ADDR	-30	/*���ʷǷ���ַ*/
#define KERR_WRITE_RDONLY_ADDR		-31	/*дֻ����ַ*/
#define KERR_THED_EXCEPTION			-32	/*�߳�ִ���쳣*/
#define KERR_THED_KILLED			-33	/*�̱߳�ɱ��*/

/*���ô���*/
#define KERR_INVALID_APINUM			-34	/*�Ƿ�ϵͳ���ú�*/
#define KERR_ARGS_TOO_LONG			-35	/*�����ִ�����*/
#define KERR_INVALID_MEMARGS_ADDR	-36	/*�Ƿ��ڴ������ַ*/
#define KERR_NO_DRIVER_PRIVILEGE	-37	/*û��ִ���������ܵ���Ȩ*/

#endif

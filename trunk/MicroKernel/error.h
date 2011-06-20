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
#define KERR_INVALID_THEDID			-7	/*�Ƿ��߳�ID*/
#define KERR_THED_NOT_EXIST			-8	/*�̲߳�����*/
#define KERR_THED_NOT_ENOUGH		-9	/*�̹߳���ṹ����*/

/*�ں�ע������*/
#define KERR_INVALID_KPTNUN			-10	/*�Ƿ��ں˶˿ں�*/
#define KERR_KPT_ALREADY_REGISTERED	-11	/*�ں˶˿��ѱ�ע��*/
#define KERR_KPT_NOT_REGISTERED		-12	/*�ں˶˿�δ��ע��*/
#define KERR_INVALID_IRQNUM			-13	/*�Ƿ�IRQ��*/
#define KERR_IRQ_ALREADY_REGISTERED	-14	/*IRQ�ѱ�ע��*/
#define KERR_IRQ_NOT_REGISTERED		-15	/*IRQδ��ע��*/
#define KERR_CURPROC_NOT_REGISTRANT	-16	/*��ǰ���̲���ע����*/

/*��Ϣ����*/
#define KERR_INVALID_USERMSG_ATTR	-17	/*�Ƿ��û���Ϣ����*/
#define KERR_MSG_NOT_ENOUGH			-18	/*��Ϣ�ṹ����*/
#define KERR_MSG_QUEUE_FULL			-19	/*��Ϣ������*/
#define KERR_MSG_QUEUE_EMPTY		-20	/*��Ϣ���п�*/

/*��ַӳ�����*/
#define KERR_MAPSIZE_IS_ZERO		-21	/*ӳ�䳤��Ϊ0*/
#define KERR_MAPSIZE_TOO_LONG		-22	/*ӳ�䳤��̫��*/
#define KERR_PROC_SELF_MAPED		-23	/*ӳ���������*/
#define KERR_ILLEGAL_PHYADDR_MAPED	-24	/*ӳ�䲻����������ַ*/
#define KERR_ADDRARGS_NOT_FOUND		-25	/*��ַ����δ�ҵ�*/

/*����ִ�д���*/
#define KERR_OUT_OF_TIME			-26	/*��ʱ����*/
#define KERR_ACCESS_ILLEGAL_ADDR	-27	/*���ʷǷ���ַ*/
#define KERR_WRITE_RDONLY_ADDR		-28	/*дֻ����ַ*/
#define KERR_THED_EXCEPTION			-29	/*�߳�ִ���쳣*/
#define KERR_THED_KILLED			-30	/*�̱߳�ɱ��*/

/*���ô���*/
#define KERR_INVALID_APINUM			-31	/*�Ƿ�ϵͳ���ú�*/
#define KERR_ARGS_TOO_LONG			-32	/*�����ִ�����*/
#define KERR_INVALID_MEMARGS_ADDR	-33	/*�Ƿ��ڴ������ַ*/
#define KERR_NO_DRIVER_PRIVILEGE	-34	/*û��ִ���������ܵ���Ȩ*/

#endif

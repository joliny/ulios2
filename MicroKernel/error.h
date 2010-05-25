/*	error.h for ulios
	���ߣ�����
	���ܣ�����������
	����޸����ڣ�2009-05-29
*/

#ifndef	_ERROR_H_
#define	_ERROR_H_

#define NO_ERROR					0	/*�޴�*/

#define ERROR_WRONG_APIN			-1	/*�����ϵͳ���ú�*/
#define ERROR_WRONG_THEDID			-2	/*������߳�ID*/
#define ERROR_WRONG_PROCID			-3	/*����Ľ���ID*/
#define ERROR_WRONG_KPTN			-4	/*������ں˶˿ں�*/
#define ERROR_WRONG_IRQN			-5	/*�����IRQ��*/
#define ERROR_WRONG_APPMSG			-6	/*�����Ӧ�ó�����Ϣ*/

#define ERROR_HAVENO_KMEM			-7	/*û���ں��ڴ�*/
#define ERROR_HAVENO_PMEM			-8	/*û�������ڴ�*/
#define ERROR_HAVENO_THEDID			-9	/*û���̹߳���ڵ�*/
#define ERROR_HAVENO_PROCID			-10	/*û�н��̹���ڵ�*/
#define ERROR_HAVENO_EXECID			-11	/*û�п�ִ����������*/
#define ERROR_HAVENO_MSGDESC		-12	/*û����Ϣ������*/
#define ERROR_HAVENO_LINEADDR		-13	/*û�����Ե�ַ�ռ�*/

#define ERROR_KPT_ISENABLED			-14	/*�ں˶˿��Ѿ���ע��*/
#define ERROR_KPT_ISDISABLED		-15	/*�ں˶˿�û�б�ע��*/
#define ERROR_KPT_WRONG_CURPROC		-16	/*��ǰ�����޷��Ķ��ں˶˿�*/
#define ERROR_IRQ_ISENABLED			-17	/*IRQ�Ѿ�����*/
#define ERROR_IRQ_ISDISABLED		-18	/*IRQ�Ѿ��ر�*/
#define ERROR_IRQ_WRONG_CURPROC		-19	/*��ǰ�߳��޷��Ķ�IRQ*/

#define ERROR_NOT_DRIVER			-20	/*�Ƿ�ִ������API*/
#define ERROR_INVALID_ADDR			-21	/*��Ч�ĵ�ַ*/
#define ERROR_INVALID_MAPADDR		-22	/*�Ƿ���ӳ���ַ*/
#define ERROR_INVALID_MAPSIZE		-23	/*�Ƿ���ӳ���С*/

#define ERROR_OUT_OF_TIME			-24	/*��ʱ����*/
#define ERROR_PROC_EXCEP			-25	/*�����쳣*/
#define ERROR_THED_KILLED			-26	/*�̱߳�ɱ��*/

#endif

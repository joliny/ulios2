/*	ulios.c for ulios
	���ߣ�����
	���ܣ�uliosϵͳ��c��ڴ��룬����΢�ں˳�ʼ���������������ķ���
	����޸����ڣ�2009-05-28
*/

#include "ulios.h"

int main()
{
	long res;

	InitKnlVal();		/*�ں˱���*/
	InitKFMT();			/*�ں��ڴ����*/
	if ((res = InitPMM()) != NO_ERROR)	/*�����ڴ����*/
		return res;
	if ((res = InitMsg()) != NO_ERROR)	/*��Ϣ����*/
		return res;
	if ((res = InitMap()) != NO_ERROR)	/*��ַӳ�����*/
		return res;
	InitPMT();			/*���̹���*/
	InitKnlProc();		/*�ں˽���*/
	InitINTR();			/*�жϴ���*/
	return InitBaseSrv();/*��������*/
}

/*	ulios.c for ulios
	���ߣ�����
	���ܣ�uliosϵͳ��c��ڴ��룬����΢�ں˳�ʼ���������������ķ���
	����޸����ڣ�2009-05-28
*/

#include "knldef.h"

int main()
{
	long res;

	InitKnlVal();		/*�ں˱���*/
	InitKFMT();			/*�ں��ڴ����*/
	res = InitPMM();	/*�����ڴ����*/
	if (res != NO_ERROR)
		return res;
	res = InitMsg();	/*��Ϣ����*/
	if (res != NO_ERROR)
		return res;
	InitEXMT();			/*��ִ�������*/
	InitPMT();			/*���̹���*/
	InitKnlProc();		/*�ں˽���*/
	InitINTR();			/*�жϴ���*/
	res = InitBaseSrv();/*��������*/
	if (res != NO_ERROR)
		return res;
	return NO_ERROR;	/*�ں˳�ʼ�����*/
}

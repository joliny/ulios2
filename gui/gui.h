/*	gui.h for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�������ؽṹ����������
	����޸����ڣ�2010-10-03
*/

#ifndef	_GUI_H_
#define	_GUI_H_

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "guiapi.h"

/**********��̬�ڴ�������**********/

typedef struct _FREE_BLK_DESC
{
	void *addr;						/*��ʼ��ַ*/
	DWORD siz;						/*�ֽ���,0��ʾ����*/
	struct _FREE_BLK_DESC *nxt;		/*��һ��*/
}FREE_BLK_DESC;	/*���ɿ�������*/

#define FDAT_SIZ		0x03F00000	/*��̬�ڴ��С*/
#define FMT_LEN			0x100		/*��̬�ڴ�������*/
extern FREE_BLK_DESC fmt[];			/*��̬�ڴ�����*/
extern DWORD fmtl;					/*��̬�ڴ������*/

/**********ͼ���û�����ṹ����**********/

typedef struct _GUIOBJ_DESC
{
	WORD id, type;					/*����ID/����*/
	THREAD_ID ptid;					/*�����߳�ID*/
	long xpos, ypos;				/*��Ը������λ��*/
	long xend, yend;				/*������/�±�Եλ��*/
	struct _GUIOBJ_DESC *pre, *nxt;	/*ǰ�����ָ��*/
	struct _GUIOBJ_DESC *par, *chl;	/*������/�Ӷ�����ָ��*/
	DWORD *buf;						/*ͼ�λ���*/
	DWORD attr;						/*����*/
}GUIOBJ_DESC;	/*GUI����������*/

#define GOBJT_LEN	0x1000		/*4k��GUI����������*/
extern GUIOBJ_DESC *gobjt[];	/*GUI����������ָ���*/

/*���ɿ����*/
void *malloc(DWORD siz);

/*���ɿ����*/
void free(void *addr, DWORD siz);

#endif

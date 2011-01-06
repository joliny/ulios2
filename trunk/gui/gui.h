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
	DWORD siz;						/*�ֽ���*/
	struct _FREE_BLK_DESC *nxt;		/*��һ��*/
}FREE_BLK_DESC;	/*���ɿ�������*/

#define FDAT_SIZ		0x00300000	/*��̬�ڴ��С*/
#define FMT_LEN			0x100		/*��̬�ڴ�������*/
extern FREE_BLK_DESC fmt[];			/*��̬�ڴ�����*/
extern DWORD fmtl;					/*��̬�ڴ������*/
#define VPAGE_SIZE		0x00001000	/*�����ڴ�ҳ��С*/
#define VDAT_SIZ		0x03C00000	/*�����ڴ��С*/
#define VMT_LEN			0x100		/*�����ڴ�������*/
extern FREE_BLK_DESC vmt[];			/*�����ڴ�����*/
extern DWORD vmtl;					/*�����ڴ������*/

/**********ͼ���û�����ṹ����**********/

typedef struct _RECT
{
	long xpos, ypos, xend, yend;
}RECT;	/*���νṹ*/

typedef struct _GUIOBJ_DESC
{
	WORD id, type;					/*����ID/����*/
	THREAD_ID ptid;					/*�����߳�ID*/
	RECT rect;						/*��Ը������λ��*/
	struct _GUIOBJ_DESC *pre, *nxt;	/*ǰ�����ָ��*/
	struct _GUIOBJ_DESC *par, *chl;	/*������/�Ӷ�����ָ��*/
	DWORD *vbuf;					/*�����ڴ滺��*/
	DWORD attr;						/*����*/
}GUIOBJ_DESC;	/*GUI����������*/

#define GOBJT_LEN	0x1000		/*4k��GUI����������*/
extern GUIOBJ_DESC *gobjt[];	/*GUI����������ָ���*/

/*��ʼ�����ɿ�����*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz);

/*���ɿ����*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz);

/*���ɿ����*/
void free(FREE_BLK_DESC *fbt, void *addr, DWORD siz);

/*��̬�ڴ����*/
static inline void *falloc(DWORD siz)
{
	return alloc(fmt, siz);
}

/*��̬�ڴ����*/
static inline void ffree(void *addr, DWORD siz)
{
	free(fmt, addr, siz);
}

/*�����ڴ����*/
static inline void *valloc(DWORD siz)
{
	return alloc(vmt, ((siz + (VPAGE_SIZE - 1)) & VPAGE_SIZE));
}

/*�����ڴ����*/
static inline void vfree(void *addr, DWORD siz)
{
	free(vmt, addr, ((siz + (VPAGE_SIZE - 1)) & VPAGE_SIZE));
}

#endif

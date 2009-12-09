/*	kernel.h for ulios
	���ߣ�����
	���ܣ��ں���ؽṹ�塢�����ֲ�����
	����޸����ڣ�2009-05-26
*/
#ifndef	_FS_H_
#define	_FS_H_

#include "ulidef.h"

#define BUF_FLAG_DIRTY		0x00000001	/*��������־*/

typedef struct
{
	DWORD BlkID;/*���*/
	DWORD DevID;/*�豸��*/
	DWORD flag;	/*��־*/
	DWORD nxt;	/*��һ��ָ��*/
}BUF_DESC;	/*������������*/

#define LABEL_SIZE	64
#define PART_ATTR_RDONLY	0x00000001	/*ֻ��*/

typedef struct 
{
	BYTE label[LABEL_SIZE];	/*�����*/
	QWORD size;				/*�����ֽ���*/
	QWORD remain;			/*ʣ���ֽ���*/
	DWORD attr;				/*����*/
}PART_ATTR;	/*��������*/

typedef struct
{
	DWORD FsID;		/*�ļ�ϵͳ�ӿڱ�ID*/
	DWORD DevID;	/*���ڴ���ID*/
	DWORD SecID;	/*��ʼ������*/
	DWORD SeCou;	/*������*/
	PART_ATTR attr;	/*��������*/
	/*�ļ�ϵͳ˽��*/
	void *data;		/*�ļ�ϵͳ˽������*/
}PART_DESC;	/*����������*/

#define FILE_NAME_SIZE		256
#define FILE_ATTR_RDONLY	0x00000001	/*ֻ��*/
#define FILE_ATTR_HIDDEN	0x00000002	/*����*/
#define FILE_ATTR_SYSTEM	0x00000004	/*ϵͳ*/
#define FILE_ATTR_LABEL		0x00000008	/*���*/
#define FILE_ATTR_DIREC		0x00000010	/*Ŀ¼(ֻ��)*/
#define FILE_ATTR_ARCH		0x00000020	/*�鵵*/
#define FILE_ATTR_EXEC		0x00000040	/*��ִ��*/
#define FILE_ATTR_UNMDFY	0x80000000	/*���Բ����޸�*/
#define FILE_FLAG_WRITE		0x00000001	/*д��ʽ��*/

typedef struct
{
	BYTE name[FILE_NAME_SIZE];	/*�ļ���*/
	DWORD CreateTime;			/*����ʱ��1970-01-01����������*/
	DWORD ModifyTime;			/*�޸�ʱ��*/
	DWORD AccessTime;			/*����ʱ��*/
	DWORD attr;					/*����*/
	QWORD size;					/*�ļ��ֽ���*/
}FILE_ATTR;	/*�ļ�����*/

typedef struct
{
	DWORD PartID;	/*������*/
	DWORD flag;		/*��־*/
	DWORD cou;		/*���ʼ���*/
	DWORD par;		/*��Ŀ¼ID*/
	FILE_ATTR attr;	/*�ļ�����*/
	DWORD avl;		/*ʵ������*/
	/*�ļ�ϵͳ˽��*/
	void *data;		/*�ļ�ϵͳ˽������*/
}FILE_DESC;	/*�ļ�������*/

typedef struct
{
	BYTE name[8];			/*�ļ�ϵͳ��ʾ*/
	long (*MntPart)(DWORD ptid);					/*���ط���*/
	void (*UmntPart)(PART_DESC *ptd);				/*ж�ط���*/
	long (*SetPart)(PART_DESC *ptd, PART_ATTR *pa);	/*���÷�����Ϣ*/
	long (*FmtPart)(PART_DESC *ptd);				/*��ʽ������*/
	BOOL (*CmpFile)(FILE_DESC *fd, BYTE *path);		/*�Ƚ�·���ַ������ļ����Ƿ�ƥ��*/
	long (*SchFile)(FILE_DESC *fd, BYTE *path);		/*�����������ļ���*/
	long (*NewFile)(FILE_DESC *fd, BYTE *path);		/*�����������ļ���*/
	long (*DelFile)(FILE_DESC *fd);					/*ɾ���ļ���*/
	long (*SetFile)(FILE_DESC *fd, FILE_ATTR *fa);	/*�����ļ�����Ϣ*/
	long (*SetSize)(FILE_DESC *fd, QWORD siz);		/*�����ļ����*/
	long (*RwFile)(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, BYTE *buf);	/*��д�ļ���*/
	long (*ReadDir)(FILE_DESC *fd, QWORD *seek, FILE_ATTR *fa);	/*��ȡĿ¼�б�*/
	void (*FreeData)(FILE_DESC *fd);				/*�ͷ��ļ��������е�˽������*/
}FSUI;	/*�ļ�ϵͳ�ϲ�ӿ�*/

/*lseek����*/
#define FS_SEEK_SET	0
#define FS_SEEK_CUR	1
#define FS_SEEK_END	2

typedef struct
{
	DWORD fid;		/*�ļ�������ID*/
	QWORD seek;		/*��дָ��*/
}FILE_HANDLE;	/*���ļ����*/

/*����FILE_DESC*/
#define FDLOCK(flag) \
({\
	cli();\
	while((flag) & FILE_FLAG_LOCKED)\
		schedul();\
	(flag) |= FILE_FLAG_LOCKED;\
	sti();\
})

/*����FILE_DESC*/
#define FDULOCK(flag) \
({\
	cli();\
	(flag) &= ~FILE_FLAG_LOCKED;\
	sti();\
})

#endif

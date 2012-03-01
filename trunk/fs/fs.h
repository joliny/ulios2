/*	fs.h for ulios file system
	���ߣ�����
	���ܣ��ļ�ϵͳ��ؽṹ����������
	����޸����ڣ�2009-05-26
*/

#ifndef	_FS_H_
#define	_FS_H_

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "fsapi.h"

/**********��̬�ڴ�������**********/

typedef struct _FREE_BLK_DESC
{
	void *addr;						/*��ʼ��ַ*/
	DWORD siz;						/*�ֽ���*/
	struct _FREE_BLK_DESC *nxt;		/*��һ��*/
}FREE_BLK_DESC;	/*���ɿ�������*/

#define FDAT_SIZ		0x00700000	/*��̬�ڴ��С*/
#define FMT_LEN			0x100		/*��̬�ڴ�������*/
extern FREE_BLK_DESC fmt[];			/*��̬�ڴ�����*/
extern DWORD fmtl;					/*��̬�ڴ������*/

/**********���ٻ������**********/

#define CACHE_ATTR_TIMES	0x00000003	/*0:�����û�б����ʹ�,1:����鱻������,2:����鱻�ٴη���,3:����鱻Ƶ������*/
#define CACHE_ATTR_DIRTY	0x00000004	/*0:�����ֻ����,1:�������*/
typedef struct _CACHE_DESC
{
	DWORD DrvID;	/*�豸��*/
	DWORD BlkID;	/*���(��ϣֵ)*/
}CACHE_DESC;	/*�����������*/

#define BLK_SHIFT		9							/*�����λ��(Ӳ��ÿ����2��9�η��ֽ�)*/
#define BLK_SIZ			(1ul << BLK_SHIFT)			/*�豸ÿ���ֽ���*/
#define BLKID_SHIFT		14							/*���λ��*/
#define BLKID_MASK		(INVALID << BLKID_SHIFT)	/*����ɰ�*/
#define BLKATTR_MASK	(~BLKID_MASK)				/*�������ɰ�*/
#define BMT_LEN			(1ul << BLKID_SHIFT)		/*���������*/
#define MAXPROC_COU		0x80						/*��ദ�������*/
#define PROC_INTERVAL	0x400						/*�������ݱ���ʱ����*/
extern CACHE_DESC bmt[];	/*���ٻ�������*/
extern void *cache;			/*���ٻ���*/
extern DWORD cahl;			/*���������*/

/**********�ļ�ϵͳ�ϲ㹦�����**********/

typedef struct _PART_DESC
{
	DWORD FsID;				/*�ļ�ϵͳ�ӿ�ID*/
	DWORD DrvID;			/*���ڴ���ID*/
	DWORD SecID;			/*��ʼ������*/
	DWORD SeCou;			/*������*/
	PART_INFO part;			/*�����ɼ���Ϣ*/
	void *data;				/*�ļ�ϵͳ˽������*/
}PART_DESC;	/*����������*/

#define FILE_FLAG_WRITE		0x0001	/*0:ֻ����ʽ1:��д��ʽ*/
typedef struct _FILE_DESC
{
	PART_DESC *part;		/*���ڷ���*/
	WORD id, flag;			/*��־*/
	DWORD cou;				/*���ʼ���*/
	struct _FILE_DESC *par;	/*��Ŀ¼*/
	FILE_INFO file;			/*�ļ��ɼ���Ϣ*/
	DWORD idx;				/*Ŀ¼�еı��*/
	void *data;				/*�ļ�ϵͳ˽������*/
}FILE_DESC;	/*�ļ�������*/

typedef struct _FILE_HANDLE
{
	FILE_DESC *fd;			/*�ļ�������*/
	DWORD avl;				/*�ļ�ϵͳ�Զ�*/
	QWORD seek;				/*��дָ��*/
}FILE_HANDLE;	/*���ļ����*/

#define EXEC_DFTENTRY		0x08000000	/*��ִ���ļ�Ĭ�����*/
#define FHT_LEN				0x100
typedef struct _PROCRES_DESC
{
	FILE_DESC *exec;		/*��ִ���ļ�*/
	DWORD CodeOff;			/*����ο�ʼ��ַ*/
	DWORD CodeEnd;			/*����ν�����ַ*/
	DWORD CodeSeek;			/*������ļ�ƫ��*/
	DWORD DataOff;			/*���ݶο�ʼ��ַ*/
	DWORD DataEnd;			/*���ݶν�����ַ*/
	DWORD DataSeek;			/*���ݶ��ļ�ƫ��*/
	DWORD entry;			/*��ڵ�*/
	FILE_DESC *CurDir;		/*���̵�ǰĿ¼*/
	FILE_HANDLE fht[FHT_LEN];	/*���ļ��б�*/
}PROCRES_DESC;	/*������Դ������*/

#define PART_LEN	0x40	/*���֧��64������*/
extern PART_DESC part[];	/*������Ϣ��*/
#define FILT_LEN	0x1000	/*����4k��Ŀ¼��*/
extern FILE_DESC* filt[];	/*���ļ�ָ���*/
extern FILE_DESC** FstFd;	/*��һ�����ļ�������ָ��*/
extern FILE_DESC** EndFd;	/*���ǿ��ļ���������һ��ָ��*/
#define PRET_LEN	0x0400	/*������Դ1k��,���ں������������*/
extern PROCRES_DESC* pret[];/*������Դ��*/
extern DWORD prel;			/*������Դ�������*/
extern DWORD SubthCou;		/*���̼߳���*/

typedef struct _FSUI
{
	char name[8];			/*�ļ�ϵͳ��ʾ*/
	long (*MntPart)(PART_DESC *pd);					/*���ط���*/
	void (*UmntPart)(PART_DESC *pd);				/*ж�ط���*/
	long (*SetPart)(PART_DESC *pd, PART_INFO *pi);	/*���÷�����Ϣ*/
	BOOL (*CmpFile)(FILE_DESC *fd, const char *path);	/*�Ƚ�·���ַ������ļ����Ƿ�ƥ��*/
	long (*SchFile)(FILE_DESC *fd, const char *path);	/*�����������ļ���*/
	long (*NewFile)(FILE_DESC *fd, const char *path);	/*�����������ļ���*/
	long (*DelFile)(FILE_DESC *fd);					/*ɾ���ļ���*/
	long (*SetFile)(FILE_DESC *fd, FILE_INFO *fi);	/*�����ļ�����Ϣ*/
	long (*SetSize)(FILE_DESC *fd, QWORD siz);		/*�����ļ����*/
	long (*RwFile)(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, void *buf, DWORD *avl);	/*��д�ļ���*/
	long (*ReadDir)(FILE_DESC *fd, QWORD *seek, FILE_INFO *fi, DWORD *avl);	/*��ȡĿ¼�б�*/
	void (*FreeData)(FILE_DESC *fd);				/*�ͷ��ļ��������е�˽������*/
}FSUI;	/*�ļ�ϵͳ�ϲ�ӿ�*/

/*���ɿ����*/
void *malloc(DWORD siz);

/*���ɿ����*/
void free(void *addr, DWORD siz);

/*�����д*/
long RwCache(DWORD drv, BOOL isWrite, DWORD sec, DWORD cou, void *buf);

/*��д����*/
static inline long RwHd(DWORD drv, BOOL isWrite, DWORD sec, DWORD cou, void *buf)
{
	return RwCache(drv, isWrite, sec, cou, buf);
}

/*��д����*/
static inline long RwPart(PART_DESC *part, BOOL isWrite, DWORD sec, DWORD cou, void *buf)
{
	return RwCache(part->DrvID, isWrite, part->SecID + sec, cou, buf);
}

/*����������*/
long SaveCache();

#endif

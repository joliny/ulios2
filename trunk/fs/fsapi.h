/*	fsapi.h for ulios file system
	���ߣ�����
	���ܣ�ulios�ļ�ϵͳ����API�ӿڶ��壬�����ļ�ϵͳ������Ҫ�������ļ�
	����޸����ڣ�2010-04-30
*/

#ifndef	_FSAPI_H_
#define	_FSAPI_H_

#include "../MkApi/ulimkapi.h"

typedef unsigned long long	QWORD;	/*64λ*/
typedef long long			SQWORD;	/*�з���64λ*/

#define LABEL_SIZE			64
#define PART_ATTR_RDONLY	0x00000001	/*ֻ��*/
typedef struct _PART_INFO
{
	char label[LABEL_SIZE];	/*�����*/
	QWORD size;				/*�����ֽ���*/
	QWORD remain;			/*ʣ���ֽ���*/
	DWORD attr;				/*����*/
}PART_INFO;	/*������Ϣ*/

#define FILE_NAME_SIZE		256
#define FILE_ATTR_RDONLY	0x00000001	/*ֻ��*/
#define FILE_ATTR_HIDDEN	0x00000002	/*����*/
#define FILE_ATTR_SYSTEM	0x00000004	/*ϵͳ*/
#define FILE_ATTR_LABEL		0x00000008	/*���*/
#define FILE_ATTR_DIREC		0x00000010	/*Ŀ¼(ֻ��)*/
#define FILE_ATTR_ARCH		0x00000020	/*�鵵*/
#define FILE_ATTR_EXEC		0x00000040	/*��ִ��*/
#define FILE_ATTR_DIRTY		0x40000000	/*���ݲ�һ��*/
#define FILE_ATTR_UNMDFY	0x80000000	/*���Բ����޸�*/
typedef struct _FILE_INFO
{
	char name[FILE_NAME_SIZE];	/*�ļ���*/
	DWORD CreateTime;			/*����ʱ��1970-01-01����������*/
	DWORD ModifyTime;			/*�޸�ʱ��*/
	DWORD AccessTime;			/*����ʱ��*/
	DWORD attr;					/*����*/
	QWORD size;					/*�ļ��ֽ���*/
}FILE_INFO;	/*�ļ���Ϣ*/

/*seek����*/
#define FS_SEEK_SET		0	/*���ļ�ͷ��λ*/
#define FS_SEEK_CUR		1	/*�ӵ�ǰλ�ö�λ*/
#define FS_SEEK_END		2	/*���ļ�β��λ*/

/*settime����*/
#define FS_SETIM_CREATE	1	/*���ô���ʱ��*/
#define FS_SETIM_MODIFY	2	/*�����޸�ʱ��*/
#define FS_SETIM_ACCESS	4	/*���÷���ʱ��*/

#define MAX_PATH	1024	/*·������ֽ���*/

#define SRV_FS_PORT		0	/*�ļ�ϵͳ����˿�*/

#define FS_API_GETEXID	0	/*�ں�ר��*/
#define FS_API_GETEXEC	1	/*�ں�ר��*/
#define FS_API_READPAGE	2	/*�ں�ר��*/
#define FS_API_PROCEXT	3	/*�ں�ר��*/
#define FS_API_ENUMPART	4
#define FS_API_GETPART	5
#define FS_API_CREAT	6
#define FS_API_OPEN		7
#define FS_API_CLOSE	8
#define FS_API_READ		9
#define FS_API_WRITE	10
#define FS_API_SEEK		11
#define FS_API_SETSIZE	12
#define FS_API_OPENDIR	13
#define FS_API_READDIR	14
#define FS_API_CHDIR	15
#define FS_API_MKDIR	16
#define FS_API_REMOVE	17
#define FS_API_RENAME	18
#define FS_API_GETATTR	19
#define FS_API_SETATTR	20
#define FS_API_SETTIME	21

/*������*/
#define FS_ERR_HAVENO_FILD		-256	/*�ļ�����������*/
#define FS_ERR_HAVENO_HANDLE	-257	/*�������*/
#define FS_ERR_HAVENO_MEMORY	-258	/*�ڴ治��*/
#define FS_ERR_HAVENO_SPACE		-259	/*���̿ռ䲻��*/
#define FS_ERR_HAVENO_PART		-260	/*ö�ٲ�������*/
#define FS_ERR_WRONG_ARGS		-261	/*��������*/
#define FS_ERR_WRONG_HANDLE		-262	/*�������*/
#define FS_ERR_WRONG_CURDIR		-263	/*��ǰĿ¼����*/
#define FS_ERR_WRONG_PARTID		-264	/*����ķ���ID*/
#define FS_ERR_NAME_FORMAT		-265	/*���Ƹ�ʽ��*/
#define FS_ERR_NAME_TOOLONG		-266	/*���Ƴ���*/
#define FS_ERR_ARGS_TOOLONG		-267	/*��������*/
#define FS_ERR_PATH_FORMAT		-268	/*·����ʽ��*/
#define FS_ERR_PATH_EXISTS		-269	/*·���Ѵ���*/
#define FS_ERR_PATH_NOT_FOUND	-270	/*·��δ�ҵ�*/
#define FS_ERR_PATH_NOT_DIR		-271	/*·������Ŀ¼*/
#define FS_ERR_PATH_NOT_FILE	-272	/*·�������ļ�*/
#define FS_ERR_PATH_READED		-273	/*·���ѱ��򿪶�*/
#define FS_ERR_PATH_WRITTEN		-274	/*·���ѱ���д*/
#define FS_ERR_ATTR_RDONLY		-275	/*����ֻ����ϵͳ*/
#define FS_ERR_ATTR_UNMDFY		-276	/*���Բ����޸�*/
#define FS_ERR_DIR_NOT_EMPTY	-277	/*Ŀ¼�ǿ�*/
#define FS_ERR_END_OF_FILE		-278	/*�����ļ���β*/
#define FS_ERR_SIZE_LIMIT		-279	/*�����ռ�����*/

/*ö�ٷ���*/
static inline long FSEnumPart(THREAD_ID ptid, DWORD pid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = FS_API_ENUMPART;
	data[4] = pid;
	if ((data[0] = KSendMsg(ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[1] != NO_ERROR)
		return data[1];
	return data[2];
}

/*ȡ�÷�����Ϣ*/
static inline long FSGetPart(THREAD_ID ptid, DWORD pid, PART_INFO *pi)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_GETPART;
	if ((data[0] = KReadProcAddr(pi, sizeof(PART_INFO) + 8, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return NO_ERROR;
}

/*�����ļ�*/
static inline long FScreat(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_CREAT;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*���ļ�*/
static inline long FSopen(THREAD_ID ptid, const char *path, DWORD op)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_OPEN;
	data[1] = op;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*�ر��ļ�*/
static inline long FSclose(THREAD_ID ptid, DWORD handle)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = FS_API_CLOSE;
	data[4] = handle;
	if ((data[0] = KSendMsg(ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[1];
}

/*��ȡ�ļ�*/
static inline long FSread(THREAD_ID ptid, DWORD handle, void *buf, DWORD siz)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_READ;
	data[1] = handle;
	if ((data[0] = KReadProcAddr(buf, siz, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*д���ļ�*/
static inline long FSwrite(THREAD_ID ptid, DWORD handle, void *buf, DWORD siz)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_WRITE;
	data[1] = handle;
	if ((data[0] = KWriteProcAddr(buf, siz, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*���ö�дָ��*/
static inline long FSseek(THREAD_ID ptid, DWORD handle, SQWORD seek, DWORD from)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = FS_API_SEEK;
	data[4] = handle;
	*((SQWORD*)&data[5]) = seek;
	data[7] = from;
	if ((data[0] = KSendMsg(ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[1];
}

/*�����ļ���С*/
static inline long FSSetSize(THREAD_ID ptid, DWORD handle, QWORD siz)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = FS_API_SETSIZE;
	data[4] = handle;
	*((QWORD*)&data[5]) = siz;
	if ((data[0] = KSendMsg(ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[1];
}

/*��Ŀ¼*/
static inline long FSOpenDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_OPENDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*��ȡĿ¼*/
static inline long FSReadDir(THREAD_ID ptid, DWORD handle, FILE_INFO *fi)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_READDIR;
	data[1] = handle;
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*�л���ǰĿ¼*/
static inline long FSChDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_CHDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*����Ŀ¼*/
static inline long FSMkDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_MKDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*ɾ���ļ����Ŀ¼*/
static inline long FSremove(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_REMOVE;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*�������ļ���Ŀ¼*/
static inline long FSrename(THREAD_ID ptid, const char *path, const char *name)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH * 2];
	data[0] = FS_API_RENAME;
	if ((data[0] = KWriteProcAddr(buf, strcpy(strcpy(buf, path) + 1, name) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*ȡ���ļ���Ŀ¼��������Ϣ*/
static inline long FSGetAttr(THREAD_ID ptid, const char *path, FILE_INFO *fi)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_GETATTR;
	if ((data[0] = KReadProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	memcpy32(fi, buf, sizeof(FILE_INFO) / sizeof(DWORD));
	return NO_ERROR;
}

/*�����ļ���Ŀ¼������*/
static inline long FSSetAttr(THREAD_ID ptid, const char *path, DWORD attr)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_SETATTR;
	data[1] = attr;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*�����ļ���Ŀ¼��ʱ��*/
static inline long FSSetTime(THREAD_ID ptid, const char *path, DWORD time, DWORD cma)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_SETTIME;
	data[1] = time;
	data[2] = cma;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) + 1 - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

#endif

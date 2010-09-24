/*	fsapi.h for ulios file system
	���ߣ�����
	���ܣ�ulios�ļ�ϵͳ����API�ӿڶ��壬�����ļ�ϵͳ������Ҫ�������ļ�
	����޸����ڣ�2010-04-30
*/

#ifndef	_FSAPI_H_
#define	_FSAPI_H_

#include "../MkApi/ulimkapi.h"

#define LABEL_SIZE			64
#define PART_ATTR_RDONLY	0x00000001	/*ֻ��*/
typedef struct _PART_INFO
{
	char label[LABEL_SIZE];	/*������*/
	QWORD size;				/*�����ֽ���*/
	QWORD remain;			/*ʣ���ֽ���*/
	DWORD attr;				/*����*/
}PART_INFO;	/*������Ϣ*/

#define FILE_NAME_SIZE		256
#define FILE_ATTR_RDONLY	0x00000001	/*ֻ��*/
#define FILE_ATTR_HIDDEN	0x00000002	/*����*/
#define FILE_ATTR_SYSTEM	0x00000004	/*ϵͳ*/
#define FILE_ATTR_LABEL		0x00000008	/*����(ֻ��)*/
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

#define FS_API_GETEXEC	0	/*�ں�ר��*/
#define FS_API_READPAGE	1	/*�ں�ר��*/
#define FS_API_PROCEXIT	2	/*�ں�ר��*/
#define FS_API_ENUMPART	3	/*ö�ٷ���*/
#define FS_API_GETPART	4	/*ȡ�÷�����Ϣ*/
#define FS_API_SETPART	5	/*���÷�����Ϣ*/
#define FS_API_CREAT	6	/*�����ļ�*/
#define FS_API_OPEN		7	/*���ļ�*/
#define FS_API_CLOSE	8	/*�ر��ļ�*/
#define FS_API_READ		9	/*��ȡ�ļ�*/
#define FS_API_WRITE	10	/*д���ļ�*/
#define FS_API_SEEK		11	/*���ö�дָ��*/
#define FS_API_SETSIZE	12	/*�����ļ���С*/
#define FS_API_OPENDIR	13	/*��Ŀ¼*/
#define FS_API_READDIR	14	/*��ȡĿ¼*/
#define FS_API_CHDIR	15	/*�л���ǰĿ¼*/
#define FS_API_MKDIR	16	/*����Ŀ¼*/
#define FS_API_REMOVE	17	/*ɾ���ļ����Ŀ¼*/
#define FS_API_RENAME	18	/*�������ļ���Ŀ¼*/
#define FS_API_GETATTR	19	/*ȡ���ļ���Ŀ¼��������Ϣ*/
#define FS_API_SETATTR	20	/*�����ļ���Ŀ¼������*/
#define FS_API_SETTIME	21	/*�����ļ���Ŀ¼��ʱ��*/
#define FS_API_PROCINFO	22	/*ȡ�ý�����Ϣ*/

/*������*/
#define FS_ERR_HAVENO_FILD		-256	/*�ļ�����������*/
#define FS_ERR_HAVENO_HANDLE	-257	/*�������*/
#define FS_ERR_HAVENO_MEMORY	-258	/*�ڴ治��*/
#define FS_ERR_HAVENO_SPACE		-259	/*���̿ռ䲻��*/
#define FS_ERR_WRONG_ARGS		-260	/*��������*/
#define FS_ERR_WRONG_HANDLE		-261	/*�������*/
#define FS_ERR_WRONG_CURDIR		-262	/*��ǰĿ¼����*/
#define FS_ERR_WRONG_PARTID		-263	/*����ķ���ID*/
#define FS_ERR_NAME_FORMAT		-264	/*���Ƹ�ʽ��*/
#define FS_ERR_NAME_TOOLONG		-265	/*���Ƴ���*/
#define FS_ERR_ARGS_TOOLONG		-266	/*��������*/
#define FS_ERR_PATH_FORMAT		-267	/*·����ʽ��*/
#define FS_ERR_PATH_EXISTS		-268	/*·���Ѵ���*/
#define FS_ERR_PATH_NOT_FOUND	-269	/*·��δ�ҵ�*/
#define FS_ERR_PATH_NOT_DIR		-270	/*·������Ŀ¼*/
#define FS_ERR_PATH_NOT_FILE	-271	/*·�������ļ�*/
#define FS_ERR_PATH_READED		-272	/*·���ѱ��򿪶�*/
#define FS_ERR_PATH_WRITTEN		-273	/*·���ѱ���д*/
#define FS_ERR_ATTR_RDONLY		-274	/*����ֻ����ϵͳ*/
#define FS_ERR_ATTR_UNMDFY		-275	/*���Բ����޸�*/
#define FS_ERR_DIR_NOT_EMPTY	-276	/*Ŀ¼�ǿ�*/
#define FS_ERR_END_OF_FILE		-277	/*�����ļ�,Ŀ¼,���̱���β*/
#define FS_ERR_SIZE_LIMIT		-278	/*�����ռ�����*/
#define FS_ERR_FILE_EMPTY		-279	/*���ļ�*/

/*ö�ٷ���*/
static inline long FSEnumPart(THREAD_ID ptid, DWORD *pid)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = MSG_ATTR_USER;
	data[3] = FS_API_ENUMPART;
	data[4] = *pid;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	*pid = data[2];
	return data[1];
}

/*ȡ�÷�����Ϣ*/
static inline long FSGetPart(THREAD_ID ptid, DWORD pid, PART_INFO *pi)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_GETPART;
	data[1] = pid;
	if ((data[0] = KReadProcAddr(pi, sizeof(PART_INFO) + 8, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*���÷�����Ϣ*/
static inline long FSSetPart(THREAD_ID ptid, DWORD pid, PART_INFO *pi)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_SETPART;
	data[1] = pid;
	if ((data[0] = KWriteProcAddr(pi, sizeof(PART_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*�����ļ�*/
static inline long FScreat(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_CREAT;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[1];
}

/*��ȡ�ļ�*/
static inline long FSread(THREAD_ID ptid, DWORD handle, void *buf, DWORD siz)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_READ;
	data[1] = handle;
	if ((data[0] = KReadProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KWriteProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[1];
}

/*��Ŀ¼*/
static inline long FSOpenDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_OPENDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*�л���ǰĿ¼*/
static inline long FSChDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_CHDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*����Ŀ¼*/
static inline long FSMkDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_MKDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*ɾ���ļ����Ŀ¼*/
static inline long FSremove(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_REMOVE;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*�������ļ���Ŀ¼*/
static inline long FSrename(THREAD_ID ptid, const char *path, const char *name)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_RENAME;
	if ((data[0] = KWriteProcAddr(buf, strcpy(strcpy(buf, path), name) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*ȡ���ļ���Ŀ¼��������Ϣ*/
static inline long FSGetAttr(THREAD_ID ptid, const char *path, FILE_INFO *fi)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_GETATTR;
	if ((data[0] = KReadProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
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
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*ȡ�ý�����Ϣ*/
static inline long FSProcInfo(THREAD_ID ptid, DWORD *pid, FILE_INFO *fi)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_PROCINFO;
	data[1] = *pid;
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	*pid = data[3];
	return data[2];
}

#endif
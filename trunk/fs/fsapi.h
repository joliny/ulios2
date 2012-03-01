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
	char label[LABEL_SIZE];	/*�����*/
	QWORD size;				/*�����ֽ���*/
	QWORD remain;			/*ʣ���ֽ���*/
	DWORD attr;				/*����*/
}PART_INFO;	/*������Ϣ*/

#define FILE_NAME_SIZE		256
#define FILE_ATTR_RDONLY	0x00000001	/*ֻ��*/
#define FILE_ATTR_HIDDEN	0x00000002	/*����*/
#define FILE_ATTR_SYSTEM	0x00000004	/*ϵͳ*/
#define FILE_ATTR_LABEL		0x00000008	/*���(ֻ��)*/
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

/*open����*/
#define FS_OPEN_READ	0	/*���ļ���*/
#define FS_OPEN_WRITE	1	/*���ļ�д*/

/*seek����*/
#define FS_SEEK_SET		0	/*���ļ�ͷ��λ*/
#define FS_SEEK_CUR		1	/*�ӵ�ǰλ�ö�λ*/
#define FS_SEEK_END		2	/*���ļ�β��λ*/

/*settime����*/
#define FS_SETIM_CREATE	1	/*���ô���ʱ��*/
#define FS_SETIM_MODIFY	2	/*�����޸�ʱ��*/
#define FS_SETIM_ACCESS	4	/*���÷���ʱ��*/

#define MAX_PATH	1024	/*·������ֽ���*/

#define SRV_FS_PORT		1	/*�ļ�ϵͳ����˿�*/

#define MSG_ATTR_FS		0x01010000	/*�ļ�ϵͳ��Ϣ*/

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
#define FS_API_GETCWD	16	/*ȡ�õ�ǰĿ¼*/
#define FS_API_MKDIR	17	/*����Ŀ¼*/
#define FS_API_REMOVE	18	/*ɾ���ļ����Ŀ¼*/
#define FS_API_RENAME	19	/*�������ļ���Ŀ¼*/
#define FS_API_GETATTR	20	/*ȡ���ļ���Ŀ¼��������Ϣ*/
#define FS_API_SETATTR	21	/*�����ļ���Ŀ¼������*/
#define FS_API_SETTIME	22	/*�����ļ���Ŀ¼��ʱ��*/
#define FS_API_PROCINFO	23	/*ȡ�ý�����Ϣ*/

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
#define FS_ERR_END_OF_FILE		-277	/*�����ļ�,Ŀ¼,���̱��β*/
#define FS_ERR_SIZE_LIMIT		-278	/*�����ռ�����*/
#define FS_ERR_FILE_EMPTY		-279	/*���ļ�*/

/*ö�ٷ���*/
static inline long FSEnumPart(DWORD *pid)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_ENUMPART;
	data[1] = *pid;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	*pid = data[1];
	return data[MSG_RES_ID];
}

/*ȡ�÷�����Ϣ*/
static inline long FSGetPart(DWORD pid, PART_INFO *pi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_GETPART;
	data[3] = pid;
	if ((data[0] = KReadProcAddr(pi, sizeof(PART_INFO) + 8, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*���÷�����Ϣ*/
static inline long FSSetPart(DWORD pid, PART_INFO *pi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_SETPART;
	data[3] = pid;
	if ((data[0] = KWriteProcAddr(pi, sizeof(PART_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*�����ļ�*/
static inline long FScreat(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_CREAT;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[2];
}

/*���ļ�*/
static inline long FSopen(const char *path, DWORD op)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_OPEN;
	data[3] = op;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[2];
}

/*�ر��ļ�*/
static inline long FSclose(DWORD handle)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_CLOSE;
	data[1] = handle;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*��ȡ�ļ�*/
static inline long FSread(DWORD handle, void *buf, DWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_READ;
	data[3] = handle;
	if ((data[0] = KReadProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[MSG_SIZE_ID];
}

/*д���ļ�*/
static inline long FSwrite(DWORD handle, void *buf, DWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_WRITE;
	data[3] = handle;
	if ((data[0] = KWriteProcAddr(buf, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[MSG_SIZE_ID];
}

/*���ö�дָ��*/
static inline long FSseek(DWORD handle, SQWORD seek, DWORD from)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_SEEK;
	data[1] = handle;
	*((SQWORD*)&data[2]) = seek;
	data[4] = from;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*�����ļ���С*/
static inline long FSSetSize(DWORD handle, QWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_FS | FS_API_SETSIZE;
	data[1] = handle;
	*((QWORD*)&data[2]) = siz;
	if ((data[0] = KSendMsg(&ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*��Ŀ¼*/
static inline long FSOpenDir(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_OPENDIR;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[2];
}

/*��ȡĿ¼*/
static inline long FSReadDir(DWORD handle, FILE_INFO *fi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_READDIR;
	data[3] = handle;
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*�л���ǰĿ¼*/
static inline long FSChDir(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_CHDIR;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*ȡ�õ�ǰĿ¼*/
static inline long FSGetCwd(char *path, DWORD siz)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_GETCWD;
	if ((data[0] = KReadProcAddr(path, siz, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	return data[MSG_SIZE_ID];
}

/*����Ŀ¼*/
static inline long FSMkDir(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_MKDIR;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*ɾ���ļ����Ŀ¼*/
static inline long FSremove(const char *path)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_REMOVE;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*�������ļ���Ŀ¼*/
static inline long FSrename(const char *path, const char *name)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_RENAME;
	if ((data[0] = KWriteProcAddr(buf, strcpy(strcpy(buf, path), name) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*ȡ���ļ���Ŀ¼��������Ϣ*/
static inline long FSGetAttr(const char *path, FILE_INFO *fi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_GETATTR;
	if ((data[0] = KReadProcAddr(buf, strcpy(buf, path) - buf, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[MSG_RES_ID] != NO_ERROR)
		return data[MSG_RES_ID];
	memcpy32(fi, buf, sizeof(FILE_INFO) / sizeof(DWORD));
	return NO_ERROR;
}

/*�����ļ���Ŀ¼������*/
static inline long FSSetAttr(const char *path, DWORD attr)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_SETATTR;
	data[3] = attr;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*�����ļ���Ŀ¼��ʱ��*/
static inline long FSSetTime(const char *path, DWORD time, DWORD cma)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_SETTIME;
	data[3] = time;
	data[4] = cma;
	if ((data[0] = KWriteProcAddr(path, strlen(path) + 1, &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[MSG_RES_ID];
}

/*ȡ�ý�����Ϣ*/
static inline long FSProcInfo(DWORD *pid, FILE_INFO *fi)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];
	ptid.ProcID = SRV_FS_PORT;
	ptid.ThedID = INVALID;
	data[MSG_API_ID] = FS_API_PROCINFO;
	data[3] = *pid;
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), &ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	*pid = data[3];
	return data[MSG_RES_ID];
}

#endif

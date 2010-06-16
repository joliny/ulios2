/*	fsapi.h for ulios file system
	作者：孙亮
	功能：ulios文件系统服务API接口定义，调用文件系统服务需要包含此文件
	最后修改日期：2010-04-30
*/

#ifndef	_FSAPI_H_
#define	_FSAPI_H_

#include "../MkApi/ulimkapi.h"

typedef unsigned long long	QWORD;	/*64位*/
typedef long long			SQWORD;	/*有符号64位*/

#define LABEL_SIZE			64
#define PART_ATTR_RDONLY	0x00000001	/*只读*/
typedef struct _PART_INFO
{
	char label[LABEL_SIZE];	/*卷标名*/
	QWORD size;				/*分区字节数*/
	QWORD remain;			/*剩余字节数*/
	DWORD attr;				/*属性*/
}PART_INFO;	/*分区信息*/

#define FILE_NAME_SIZE		256
#define FILE_ATTR_RDONLY	0x00000001	/*只读*/
#define FILE_ATTR_HIDDEN	0x00000002	/*隐藏*/
#define FILE_ATTR_SYSTEM	0x00000004	/*系统*/
#define FILE_ATTR_LABEL		0x00000008	/*卷标*/
#define FILE_ATTR_DIREC		0x00000010	/*目录(只读)*/
#define FILE_ATTR_ARCH		0x00000020	/*归档*/
#define FILE_ATTR_EXEC		0x00000040	/*可执行*/
#define FILE_ATTR_DIRTY		0x40000000	/*数据不一致*/
#define FILE_ATTR_UNMDFY	0x80000000	/*属性不可修改*/
typedef struct _FILE_INFO
{
	char name[FILE_NAME_SIZE];	/*文件名*/
	DWORD CreateTime;			/*创建时间1970-01-01经过的秒数*/
	DWORD ModifyTime;			/*修改时间*/
	DWORD AccessTime;			/*访问时间*/
	DWORD attr;					/*属性*/
	QWORD size;					/*文件字节数*/
}FILE_INFO;	/*文件信息*/

/*seek参数*/
#define FS_SEEK_SET		0	/*从文件头定位*/
#define FS_SEEK_CUR		1	/*从当前位置定位*/
#define FS_SEEK_END		2	/*从文件尾定位*/

/*settime参数*/
#define FS_SETIM_CREATE	1	/*设置创建时间*/
#define FS_SETIM_MODIFY	2	/*设置修改时间*/
#define FS_SETIM_ACCESS	4	/*设置访问时间*/

#define MAX_PATH	1024	/*路径最多字节数*/

#define SRV_FS_PORT		0	/*文件系统服务端口*/

#define FS_API_GETEXID	0	/*内核专用*/
#define FS_API_GETEXEC	1	/*内核专用*/
#define FS_API_READPAGE	2	/*内核专用*/
#define FS_API_PROCEXT	3	/*内核专用*/
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

/*错误定义*/
#define FS_ERR_HAVENO_FILD		-256	/*文件描述符不足*/
#define FS_ERR_HAVENO_HANDLE	-257	/*句柄不足*/
#define FS_ERR_HAVENO_MEMORY	-258	/*内存不足*/
#define FS_ERR_HAVENO_SPACE		-259	/*磁盘空间不足*/
#define FS_ERR_HAVENO_PART		-260	/*枚举不到分区*/
#define FS_ERR_WRONG_ARGS		-261	/*参数错误*/
#define FS_ERR_WRONG_HANDLE		-262	/*句柄错误*/
#define FS_ERR_WRONG_CURDIR		-263	/*当前目录错误*/
#define FS_ERR_WRONG_PARTID		-264	/*错误的分区ID*/
#define FS_ERR_NAME_FORMAT		-265	/*名称格式错*/
#define FS_ERR_NAME_TOOLONG		-266	/*名称超长*/
#define FS_ERR_ARGS_TOOLONG		-267	/*参数超长*/
#define FS_ERR_PATH_FORMAT		-268	/*路径格式错*/
#define FS_ERR_PATH_EXISTS		-269	/*路径已存在*/
#define FS_ERR_PATH_NOT_FOUND	-270	/*路径未找到*/
#define FS_ERR_PATH_NOT_DIR		-271	/*路径不是目录*/
#define FS_ERR_PATH_NOT_FILE	-272	/*路径不是文件*/
#define FS_ERR_PATH_READED		-273	/*路径已被打开读*/
#define FS_ERR_PATH_WRITTEN		-274	/*路径已被打开写*/
#define FS_ERR_ATTR_RDONLY		-275	/*属性只读或系统*/
#define FS_ERR_ATTR_UNMDFY		-276	/*属性不可修改*/
#define FS_ERR_DIR_NOT_EMPTY	-277	/*目录非空*/
#define FS_ERR_END_OF_FILE		-278	/*到达文件结尾*/
#define FS_ERR_SIZE_LIMIT		-279	/*数量空间限制*/

/*枚举分区*/
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

/*取得分区信息*/
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

/*创建文件*/
static inline long FScreat(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_CREAT;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*打开文件*/
static inline long FSopen(THREAD_ID ptid, const char *path, DWORD op)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_OPEN;
	data[1] = op;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*关闭文件*/
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

/*读取文件*/
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

/*写入文件*/
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

/*设置读写指针*/
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

/*设置文件大小*/
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

/*打开目录*/
static inline long FSOpenDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_OPENDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	return data[3];
}

/*读取目录*/
static inline long FSReadDir(THREAD_ID ptid, DWORD handle, FILE_INFO *fi)
{
	DWORD data[MSG_DATA_LEN];
	data[0] = FS_API_READDIR;
	data[1] = handle;
	if ((data[0] = KReadProcAddr(fi, sizeof(FILE_INFO), ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*切换当前目录*/
static inline long FSChDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_CHDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*创建目录*/
static inline long FSMkDir(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_MKDIR;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*删除文件或空目录*/
static inline long FSremove(THREAD_ID ptid, const char *path)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_REMOVE;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*重命名文件或目录*/
static inline long FSrename(THREAD_ID ptid, const char *path, const char *name)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH * 2];
	data[0] = FS_API_RENAME;
	if ((data[0] = KWriteProcAddr(buf, strcpy(strcpy(buf, path), name) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*取得文件或目录的属性信息*/
static inline long FSGetAttr(THREAD_ID ptid, const char *path, FILE_INFO *fi)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_GETATTR;
	if ((data[0] = KReadProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	if (data[2] != NO_ERROR)
		return data[2];
	memcpy32(fi, buf, sizeof(FILE_INFO) / sizeof(DWORD));
	return NO_ERROR;
}

/*设置文件或目录的属性*/
static inline long FSSetAttr(THREAD_ID ptid, const char *path, DWORD attr)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_SETATTR;
	data[1] = attr;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

/*设置文件或目录的时间*/
static inline long FSSetTime(THREAD_ID ptid, const char *path, DWORD time, DWORD cma)
{
	DWORD data[MSG_DATA_LEN];
	char buf[MAX_PATH];
	data[0] = FS_API_SETTIME;
	data[1] = time;
	data[2] = cma;
	if ((data[0] = KWriteProcAddr(buf, strcpy(buf, path) - buf, ptid, data, INVALID)) != NO_ERROR)
		return data[0];
	return data[2];
}

#endif

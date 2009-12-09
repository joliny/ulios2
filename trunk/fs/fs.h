/*	kernel.h for ulios
	作者：孙亮
	功能：内核相关结构体、变量分布定义
	最后修改日期：2009-05-26
*/
#ifndef	_FS_H_
#define	_FS_H_

#include "ulidef.h"

#define BUF_FLAG_DIRTY		0x00000001	/*缓冲块脏标志*/

typedef struct
{
	DWORD BlkID;/*块号*/
	DWORD DevID;/*设备号*/
	DWORD flag;	/*标志*/
	DWORD nxt;	/*下一项指针*/
}BUF_DESC;	/*缓冲区描述符*/

#define LABEL_SIZE	64
#define PART_ATTR_RDONLY	0x00000001	/*只读*/

typedef struct 
{
	BYTE label[LABEL_SIZE];	/*卷标名*/
	QWORD size;				/*分区字节数*/
	QWORD remain;			/*剩余字节数*/
	DWORD attr;				/*属性*/
}PART_ATTR;	/*分区属性*/

typedef struct
{
	DWORD FsID;		/*文件系统接口表ID*/
	DWORD DevID;	/*所在磁盘ID*/
	DWORD SecID;	/*起始扇区号*/
	DWORD SeCou;	/*扇区数*/
	PART_ATTR attr;	/*分区属性*/
	/*文件系统私有*/
	void *data;		/*文件系统私有数据*/
}PART_DESC;	/*分区描述符*/

#define FILE_NAME_SIZE		256
#define FILE_ATTR_RDONLY	0x00000001	/*只读*/
#define FILE_ATTR_HIDDEN	0x00000002	/*隐藏*/
#define FILE_ATTR_SYSTEM	0x00000004	/*系统*/
#define FILE_ATTR_LABEL		0x00000008	/*卷标*/
#define FILE_ATTR_DIREC		0x00000010	/*目录(只读)*/
#define FILE_ATTR_ARCH		0x00000020	/*归档*/
#define FILE_ATTR_EXEC		0x00000040	/*可执行*/
#define FILE_ATTR_UNMDFY	0x80000000	/*属性不可修改*/
#define FILE_FLAG_WRITE		0x00000001	/*写方式打开*/

typedef struct
{
	BYTE name[FILE_NAME_SIZE];	/*文件名*/
	DWORD CreateTime;			/*创建时间1970-01-01经过的秒数*/
	DWORD ModifyTime;			/*修改时间*/
	DWORD AccessTime;			/*访问时间*/
	DWORD attr;					/*属性*/
	QWORD size;					/*文件字节数*/
}FILE_ATTR;	/*文件属性*/

typedef struct
{
	DWORD PartID;	/*分区号*/
	DWORD flag;		/*标志*/
	DWORD cou;		/*访问计数*/
	DWORD par;		/*父目录ID*/
	FILE_ATTR attr;	/*文件属性*/
	DWORD avl;		/*实例自用*/
	/*文件系统私有*/
	void *data;		/*文件系统私有数据*/
}FILE_DESC;	/*文件描述符*/

typedef struct
{
	BYTE name[8];			/*文件系统标示*/
	long (*MntPart)(DWORD ptid);					/*挂载分区*/
	void (*UmntPart)(PART_DESC *ptd);				/*卸载分区*/
	long (*SetPart)(PART_DESC *ptd, PART_ATTR *pa);	/*设置分区信息*/
	long (*FmtPart)(PART_DESC *ptd);				/*格式化分区*/
	BOOL (*CmpFile)(FILE_DESC *fd, BYTE *path);		/*比较路径字符串与文件名是否匹配*/
	long (*SchFile)(FILE_DESC *fd, BYTE *path);		/*搜索并设置文件项*/
	long (*NewFile)(FILE_DESC *fd, BYTE *path);		/*创建并设置文件项*/
	long (*DelFile)(FILE_DESC *fd);					/*删除文件项*/
	long (*SetFile)(FILE_DESC *fd, FILE_ATTR *fa);	/*设置文件项信息*/
	long (*SetSize)(FILE_DESC *fd, QWORD siz);		/*设置文件项长度*/
	long (*RwFile)(FILE_DESC *fd, BOOL isWrite, QWORD seek, DWORD siz, BYTE *buf);	/*读写文件项*/
	long (*ReadDir)(FILE_DESC *fd, QWORD *seek, FILE_ATTR *fa);	/*获取目录列表*/
	void (*FreeData)(FILE_DESC *fd);				/*释放文件描述符中的私有数据*/
}FSUI;	/*文件系统上层接口*/

/*lseek参数*/
#define FS_SEEK_SET	0
#define FS_SEEK_CUR	1
#define FS_SEEK_END	2

typedef struct
{
	DWORD fid;		/*文件描述符ID*/
	QWORD seek;		/*读写指针*/
}FILE_HANDLE;	/*打开文件句柄*/

/*锁定FILE_DESC*/
#define FDLOCK(flag) \
({\
	cli();\
	while((flag) & FILE_FLAG_LOCKED)\
		schedul();\
	(flag) |= FILE_FLAG_LOCKED;\
	sti();\
})

/*解锁FILE_DESC*/
#define FDULOCK(flag) \
({\
	cli();\
	(flag) &= ~FILE_FLAG_LOCKED;\
	sti();\
})

#endif

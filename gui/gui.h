/*	gui.h for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面相关结构、常量定义
	最后修改日期：2010-10-03
*/

#ifndef	_GUI_H_
#define	_GUI_H_

#include "../MkApi/ulimkapi.h"
#include "../driver/basesrv.h"
#include "guiapi.h"

/**********动态内存管理相关**********/

typedef struct _FREE_BLK_DESC
{
	void *addr;						/*起始地址*/
	DWORD siz;						/*字节数*/
	struct _FREE_BLK_DESC *nxt;		/*后一项*/
}FREE_BLK_DESC;	/*自由块描述符*/

#define FDAT_SIZ		0x00300000	/*动态内存大小*/
#define FMT_LEN			0x100		/*动态内存管理表长度*/
extern FREE_BLK_DESC fmt[];			/*动态内存管理表*/
extern DWORD fmtl;					/*动态内存管理锁*/
#define VPAGE_SIZE		0x00001000	/*可视内存页大小*/
#define VDAT_SIZ		0x03C00000	/*可视内存大小*/
#define VMT_LEN			0x100		/*可视内存管理表长度*/
extern FREE_BLK_DESC vmt[];			/*可视内存管理表*/
extern DWORD vmtl;					/*可视内存管理锁*/

/**********图形用户界面结构定义**********/

typedef struct _RECT
{
	long xpos, ypos, xend, yend;
}RECT;	/*矩形结构*/

typedef struct _GUIOBJ_DESC
{
	WORD id, type;					/*对象ID/类型*/
	THREAD_ID ptid;					/*所属线程ID*/
	RECT rect;						/*相对父窗体的位置*/
	struct _GUIOBJ_DESC *pre, *nxt;	/*前后对象指针*/
	struct _GUIOBJ_DESC *par, *chl;	/*父对象/子对象链指针*/
	DWORD *vbuf;					/*可视内存缓冲*/
	DWORD attr;						/*属性*/
}GUIOBJ_DESC;	/*GUI对象描述符*/

#define GOBJT_LEN	0x1000		/*4k个GUI对象描述符*/
extern GUIOBJ_DESC *gobjt[];	/*GUI对象描述符指针表*/

/*初始化自由块管理表*/
void InitFbt(FREE_BLK_DESC *fbt, DWORD FbtLen, void *addr, DWORD siz);

/*自由块分配*/
void *alloc(FREE_BLK_DESC *fbt, DWORD siz);

/*自由块回收*/
void free(FREE_BLK_DESC *fbt, void *addr, DWORD siz);

/*动态内存分配*/
static inline void *falloc(DWORD siz)
{
	return alloc(fmt, siz);
}

/*动态内存回收*/
static inline void ffree(void *addr, DWORD siz)
{
	free(fmt, addr, siz);
}

/*可视内存分配*/
static inline void *valloc(DWORD siz)
{
	return alloc(vmt, ((siz + (VPAGE_SIZE - 1)) & VPAGE_SIZE));
}

/*可视内存回收*/
static inline void vfree(void *addr, DWORD siz)
{
	free(vmt, addr, ((siz + (VPAGE_SIZE - 1)) & VPAGE_SIZE));
}

#endif

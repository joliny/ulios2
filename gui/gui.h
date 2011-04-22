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
#define VPAGE_SIZE		0x00001000	/*可视内存页大小*/
#define VDAT_SIZ		0x03C00000	/*可视内存大小*/
#define VMT_LEN			0x100		/*可视内存管理表长度*/
extern FREE_BLK_DESC vmt[];			/*可视内存管理表*/

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

/**********图形用户界面结构定义**********/

typedef struct _RECT
{
	long xpos, ypos, xend, yend;
}RECT;	/*矩形结构*/

#define CLIPRECTT_LEN	0x1000		/*剪切矩形管理表长度*/

typedef struct _CLIPRECT
{
	RECT rect;						/*剪切矩形*/
	struct _CLIPRECT *nxt;			/*后续指针*/
}CLIPRECT;	/*可视剪切矩形节点,用于窗体的重叠控制*/

#define GOBJT_LEN		0x1000		/*窗体描述符表长度*/

typedef struct _GOBJ_DESC
{
	WORD id, type;					/*对象ID/类型*/
	THREAD_ID ptid;					/*所属线程ID*/
	RECT rect;						/*相对父窗体的位置*/
	struct _GOBJ_DESC *pre, *nxt;	/*兄/弟对象链指针*/
	struct _GOBJ_DESC *par, *chl;	/*父/子对象链指针*/
	CLIPRECT *ClipList;				/*自身剪切矩形列表*/
	DWORD *vbuf;					/*可视内存缓冲*/
	DWORD attr;						/*属性*/
}GOBJ_DESC;	/*GUI对象(窗体)描述符*/

typedef struct _GCTRLI
{
	DWORD ObjSize;		/*控件结构大小*/
	long (*InitCtrl)(GOBJ_DESC *gobj);	/*初始化控件*/
	long (*ReleaseCtrl)(GOBJ_DESC *gobj);	/*撤销控件*/
}GCTRLI;	/*控件接口*/

/**********图形用户界面上层接口**********/

long CreateDesktop(long width, long height);

long CreateGobj(THREAD_ID ptid, WORD pid, WORD type, long xpos, long ypos, long width, long height, DWORD attr, WORD *id);

#endif

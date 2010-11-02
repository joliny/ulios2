/*	control.h for ulios graphical user interface
	作者：孙亮
	功能：控件相关结构、常量定义
	最后修改日期：2010-10-05
*/

#ifndef	_CONTROL_H_
#define	_CONTROL_H_

#include "gui.h"

typedef struct _GUIDSK_WNDBLOCK
{
	long xpos, xend;				/*块起始/结束X坐标*/
	GUIOBJ_DESC *obj;				/*窗口对象指针*/
	struct _GUIDSK_WNDBLOCK *nxt;	/*后续指针*/
}GUIDSK_WNDBLOCK;	/*窗口块节点,用于桌面内窗口的重叠控制*/

typedef struct _GUIOBJ_DESKTOP
{
	GUIOBJ_DESC obj;
	GUIDSK_WNDBLOCK **blk;
}GUIOBJ_DESKTOP;		/*桌面结构*/

typedef struct _GUIOBJ_WINDOW
{
	GUIOBJ_DESC obj;
}GUIOBJ_WINDOW;		/*窗口结构*/

#define BUTTON_CAPTION_LEN 1024
typedef struct _GUIOBJ_BUTTON
{
	GUIOBJ_DESC obj;
	char caption[BUTTON_CAPTION_LEN];
}GUIOBJ_BUTTON;		/*按钮结构*/

#endif

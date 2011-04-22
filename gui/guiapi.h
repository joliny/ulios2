/*	guiapi.h for ulios graphical user interface
	作者：孙亮
	功能：ulios GUI服务API接口定义，调用GUI服务需要包含此文件
	最后修改日期：2010-10-05
*/

#ifndef	_GUIAPI_H_
#define	_GUIAPI_H_

#include "../MkApi/ulimkapi.h"

#define GUI_TYPE_DESKTOP	0	/*桌面类型*/
#define GUI_TYPE_WINDOW		1	/*窗口类型*/
#define GUI_TYPE_BUTTON		2	/*按钮类型*/

/*错误定义*/
#define GUI_ERR_HAVENO_GOBJ		-2560	/*窗体描述符不足*/
#define GUI_ERR_HAVENO_MEMORY	-2561	/*内存不足*/
#define GUI_ERR_HAVENO_CLIPRECT	-2562	/*剪切矩形不足,窗口系统过于复杂*/
#define GUI_ERR_WRONG_GOBJTYPE	-2563	/*错误的窗体类型*/
#define GUI_ERR_WRONG_WNDSIZE	-2564	/*错误的窗口大小*/
#define GUI_ERR_WRONG_GOBJID	-2565	/*错误的窗体ID*/
#define GUI_ERR_NOT_OVERLAP		-2566	/*矩形不重叠*/
#define GUI_ERR_NOCHG_CLIPRECT	-2567	/*剪切矩形没有变化*/
#define GUI_ERR_WRONG_HANDLE	-2568	/*错误的窗体句柄*/

#define SRV_GUI_PORT		10	/*GUI服务端口*/

#endif

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
#define GUI_ERR_HAVENO_GOBJ		-2560	/*GUI对象描述符不足*/

#define SRV_GUI_PORT		10	/*GUI服务端口*/

#endif

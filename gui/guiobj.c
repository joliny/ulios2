/*	guiobj.c for ulios graphical user interface
	作者：孙亮
	功能：图形用户界面GUI对象基本操作
	最后修改日期：2010-10-05
*/

#include "gui.h"
#include "control.h"

GUIOBJ_DESC *gobjt[GOBJT_LEN];	/*GUI对象描述符指针表*/
GUIOBJ_DESC **FstGobj;			/*第一个空对象描述符指针*/

/*分配空对象描述符ID*/
long AllocGobj(GUIOBJ_DESC *gobj)
{
	if (FstGobj >= &gobjt[GOBJT_LEN])
		return GUI_ERR_HAVENO_GOBJ;
	gobj->id = FstGobj - gobjt;
	*FstGobj = gobj;
	do
		FstGobj++;
	while (FstGobj < &gobjt[GOBJT_LEN] && *FstGobj);
	return NO_ERROR;
}

/*释放空对象描述符ID*/
void FreeGobj(WORD gid)
{
	GUIOBJ_DESC **gobj;

	gobj = &gobjt[gid];
	*gobj = NULL;
	if (FstGobj > gobj)
		FstGobj = gobj;
}

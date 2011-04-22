/*	control.c for ulios graphical user interface
	作者：孙亮
	功能：控件功能实现
	最后修改日期：2010-10-05
*/

#include "control.h"

long InitDesktop(GUIOBJ_DESKTOP *dsk)
{
	GDIFillRect(dsk->obj.rect.xpos, dsk->obj.rect.ypos, dsk->obj.rect.xend - dsk->obj.rect.xpos, dsk->obj.rect.yend - dsk->obj.rect.ypos, 0xFF);
	return NO_ERROR;
}

long InitWindow(GUIOBJ_WINDOW *wnd)
{
	GDIFillRect(wnd->obj.rect.xpos, wnd->obj.rect.ypos, wnd->obj.rect.xend - wnd->obj.rect.xpos, wnd->obj.rect.yend - wnd->obj.rect.ypos, 0xFF0000);
	return NO_ERROR;
}

long InitButton(GUIOBJ_BUTTON *btn)
{
	GDIFillRect(btn->obj.rect.xpos, btn->obj.rect.ypos, btn->obj.rect.xend - btn->obj.rect.xpos, btn->obj.rect.yend - btn->obj.rect.ypos, 0xFF00);
	return NO_ERROR;
}

long ReleaseDesktop(GUIOBJ_DESKTOP *dsk)
{
	return NO_ERROR;
}

long ReleaseWindow(GUIOBJ_WINDOW *wnd)
{
	return NO_ERROR;
}

long ReleaseButton(GUIOBJ_BUTTON *btn)
{
	return NO_ERROR;
}

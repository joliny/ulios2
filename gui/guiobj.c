/*	guiobj.c for ulios graphical user interface
	���ߣ�����
	���ܣ�ͼ���û�����GUI�����������
	����޸����ڣ�2010-10-05
*/

#include "gui.h"
#include "control.h"

GUIOBJ_DESC *gobjt[GOBJT_LEN];	/*GUI����������ָ���*/
GUIOBJ_DESC **FstGobj;			/*��һ���ն���������ָ��*/

/*����ն���������ID*/
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

/*�ͷſն���������ID*/
void FreeGobj(WORD gid)
{
	GUIOBJ_DESC **gobj;

	gobj = &gobjt[gid];
	*gobj = NULL;
	if (FstGobj > gobj)
		FstGobj = gobj;
}

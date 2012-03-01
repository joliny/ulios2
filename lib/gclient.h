/*	gclient.h for ulios graphical user interface
���ߣ�����
���ܣ�GUI�ͻ��˹��ܿⶨ��
����޸����ڣ�2011-08-15
*/

#ifndef	_GCLIENT_H_
#define	_GCLIENT_H_

#include "../gui/guiapi.h"

/**********��ͼ���ܶ���**********/

#define GC_ERR_LOCATION			-2688	/*�������*/
#define GC_ERR_AREASIZE			-2689	/*�ߴ����*/
#define GC_ERR_OUT_OF_MEM		-2690	/*�ڴ治��*/
#define GC_ERR_INVALID_GUIMSG	-2691	/*�Ƿ�GUI��Ϣ*/
#define GC_ERR_WRONG_ARGS		-2692	/*��������*/

typedef struct _UDI_IMAGE
{
	DWORD width, height;	/*�ߴ�*/
	DWORD buf[];			/*λͼ����,��չ��width*height��С*/
}UDI_IMAGE;	/*GUIλͼ*/

typedef struct _UDI_AREA
{
	DWORD width, height;	/*�ߴ�*/
	DWORD *vbuf;			/*��ͼ����*/
	struct _UDI_AREA *root;	/*����ͼ��*/
}UDI_AREA;	/*GUI��ͼ����*/

extern const BYTE *GCfont;
extern DWORD GCwidth, GCheight;
extern DWORD GCCharWidth, GCCharHeight;

/*��ʼ��GC��*/
long GCinit();

/*����GC��*/
void GCrelease();

/*���û�ͼ����Ĳ����ͻ���,��һ�������޸���ͼ��ʱ�����uda->vbuf*/
long GCSetArea(UDI_AREA *uda, DWORD width, DWORD height, const UDI_AREA *par, long x, long y);

/*���ջ�ͼ���򻺴�*/
void GCFreeArea(UDI_AREA *uda);

/*����*/
long GCPutPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD c);

/*ȡ��*/
long GCGetPixel(UDI_AREA *uda, DWORD x, DWORD y, DWORD *c);

/*��ͼ*/
long GCPutImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h);

/*ȥ����ɫ��ͼ*/
long GCPutBCImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h, DWORD bc);

/*��ͼ*/
long GCGetImage(UDI_AREA *uda, long x, long y, DWORD *img, long w, long h);

/*������*/
long GCFillRect(UDI_AREA *uda, long x, long y, long w, long h, DWORD c);

/*����*/
long GCDrawLine(UDI_AREA *uda, long x1, long y1, long x2, long y2, DWORD c);

/*��Բ*/
long GCcircle(UDI_AREA *uda, long cx, long cy, long r, DWORD c);

/*���ƺ���*/
long GCDrawHz(UDI_AREA *uda, long x, long y, DWORD hz, DWORD c);

/*����ASCII�ַ�*/
long GCDrawAscii(UDI_AREA *uda, long x, long y, DWORD ch, DWORD c);

/*�����ַ���*/
long GCDrawStr(UDI_AREA *uda, long x, long y, const char *str, DWORD c);

/*����BMPͼ���ļ�*/
long GCLoadBmp(char *path, DWORD *buf, DWORD len, DWORD *width, DWORD *height);

/**********GUI�ͻ����Զ�����Ϣ**********/

#define GM_CLOSE		0x8000	/*�رմ�����Ϣ*/

/**********�ؼ�����**********/

/*�ؼ����ʹ���*/
#define GC_CTRL_TYPE_DESKTOP	0		/*����*/
#define GC_CTRL_TYPE_WINDOW		1		/*����*/
#define GC_CTRL_TYPE_BUTTON		2		/*��ť*/
#define GC_CTRL_TYPE_TEXT		3		/*��̬�ı���*/
#define GC_CTRL_TYPE_SLEDIT		4		/*���б༭��*/
#define GC_CTRL_TYPE_MLEDIT		5		/*���б༭��*/
#define GC_CTRL_TYPE_SCROLL		6		/*������*/
#define GC_CTRL_TYPE_LIST		7		/*�б��*/

typedef long (*MSGPROC)(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);	/*������Ϣ������*/

typedef struct _CTRL_ARGS
{
	DWORD width, height;	/*����ߴ�*/
	long x, y;				/*����λ��*/
	WORD type, style;		/*����,��ʽ*/
	MSGPROC MsgProc;		/*��Ϣ������*/
}CTRL_ARGS;	/*�ؼ�����*/

typedef struct _CTRL_GOBJ
{
	UDI_AREA uda;			/*�����ͼ����*/
	DWORD gid;				/*GUI����˶���ID*/
	struct _CTRL_GOBJ *pre, *nxt;	/*��/�ܿؼ���ָ��*/
	struct _CTRL_GOBJ *par, *chl;	/*��/�ӿؼ���ָ��*/
	long x, y;				/*����λ��*/
	WORD type, style;		/*����,��ʽ*/
	MSGPROC MsgProc;		/*��Ϣ������*/
	void (*DrawProc)(struct _CTRL_GOBJ *gobj);	/*���ƴ�����*/
}CTRL_GOBJ;	/*�ؼ�����*/

typedef void (*DRAWPROC)(CTRL_GOBJ *gobj);							/*������ƴ�����*/

/*��ʼ��CTRL_GOBJ�ṹ*/
long GCGobjInit(CTRL_GOBJ *gobj, const CTRL_ARGS *args, MSGPROC MsgProc, DRAWPROC DrawProc, DWORD pid, CTRL_GOBJ *ParGobj);

/*ɾ��������*/
void GCGobjDelete(CTRL_GOBJ *gobj);

/*���ƴ�����*/
void GCGobjDraw(CTRL_GOBJ *gobj);

/*GUI�ͻ�����Ϣ����*/
long GCDispatchMsg(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

/*�ƶ��򵥴���*/
void GCGobjMove(CTRL_GOBJ *gobj, long x, long y);

/*���ü򵥴���λ�ô�С*/
void GCGobjSetSize(CTRL_GOBJ *gobj, long x, long y, DWORD width, DWORD height);

/**********����**********/

typedef struct _CTRL_DSK
{
	CTRL_GOBJ obj;
}CTRL_DSK;	/*������*/

long GCDskCreate(CTRL_DSK **dsk, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj);

long GCDskDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCDskDefDrawProc(CTRL_DSK *dsk);

/**********��ť**********/

#define BTN_TXT_LEN			32

#define BTN_STYLE_DISABLED	0x0001	/*������*/

typedef struct _CTRL_BTN
{
	CTRL_GOBJ obj;
	char text[BTN_TXT_LEN];			/*��ť�ı�*/
	UDI_IMAGE *img;					/*��ťͼƬ*/
	BOOL isPressDown;				/*�Ƿ��Ѱ���*/
	void (*PressProc)(struct _CTRL_BTN *btn);	/*���������*/
}CTRL_BTN;	/*��ť��*/

long GCBtnCreate(CTRL_BTN **btn, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, UDI_IMAGE *img, void (*PressProc)(CTRL_BTN *btn));

long GCBtnDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCBtnDefDrawProc(CTRL_BTN *btn);

/*���ð�ť�ı�*/
void GCBtnSetText(CTRL_BTN *btn, const char *text);

/*���ð�ťΪ��������ʽ*/
void GCBtnSetDisable(CTRL_BTN *btn, BOOL isDisable);

/**********����**********/

#define WND_CAP_LEN			128

#define WND_STYLE_CAPTION	0x0001	/*���������*/
#define WND_STYLE_BORDER	0x0002	/*����߿�*/
#define WND_STYLE_CLOSEBTN	0x0004	/*�رհ�ť*/
#define WND_STYLE_MAXBTN	0x0008	/*��󻯰�ť*/
#define WND_STYLE_MINBTN	0x0010	/*��С����ť*/
#define WND_STYLE_SIZEBTN	0x0020	/*�϶����Ű�ť*/

#define WND_STATE_FOCUS		0x8000	/*����״̬*/
#define WND_STATE_TOP		0x4000	/*����״̬*/

typedef struct _CTRL_WND
{
	CTRL_GOBJ obj;
	char caption[WND_CAP_LEN];		/*�����ı�*/
	CTRL_BTN *close;				/*�رհ�ť*/
	CTRL_BTN *max;					/*��󻯰�ť*/
	CTRL_BTN *min;					/*��С����ť*/
	CTRL_BTN *size;					/*���Ű�ť*/
	UDI_AREA client;				/*�ͻ���ͼ��*/
	DWORD MinWidth, MinHeight;		/*��С������С*/
	DWORD MaxWidth, MaxHeight;
	long x0, y0;					/*����λ��,��С*/
	DWORD width0, height0;
}CTRL_WND;	/*������*/

long GCWndCreate(CTRL_WND **wnd, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *caption);

long GCWndDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCWndDefDrawProc(CTRL_WND *wnd);

/*���ô��ڱ����ı�*/
void GCWndSetCaption(CTRL_WND *wnd, const char *caption);

/*���ô���λ�ô�С*/
void GCWndSetSize(CTRL_WND *wnd, long x, long y, DWORD width, DWORD height);

/*ȡ�ô��ڿͻ���λ��*/
void GCWndGetClientLoca(CTRL_WND *wnd, long *x, long *y);

/**********��̬�ı���**********/

#define TXT_TXT_LEN			128

typedef struct _CTRL_TXT
{
	CTRL_GOBJ obj;
	char text[TXT_TXT_LEN];			/*�ı����ı�*/
}CTRL_TXT;	/*��̬�ı�����*/

long GCTxtCreate(CTRL_TXT **txt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text);

long GCTxtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCTxtDefDrawProc(CTRL_TXT *txt);

/*�����ı����ı�*/
void GCTxtSetText(CTRL_TXT *txt, const char *text);

/**********���б༭��**********/

#define SEDT_TXT_LEN		128

#define SEDT_STYLE_RDONLY	0x0001	/*ֻ��*/

#define SEDT_STATE_FOCUS	0x8000	/*����״̬*/
#define SEDT_STATE_IME		0x4000	/*�������뷨*/

typedef struct _CTRL_SEDT
{
	CTRL_GOBJ obj;
	char text[SEDT_TXT_LEN];		/*�༭���ı�*/
	char *FstC, *CurC;				/*���ַ�λ��,���λ��*/
	void (*EnterProc)(struct _CTRL_SEDT *edt);	/*�س�������*/
}CTRL_SEDT;	/*���б༭��*/

long GCSedtCreate(CTRL_SEDT **edt, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, const char *text, void (*EnterProc)(CTRL_SEDT *edt));

long GCSedtDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCSedtDefDrawProc(CTRL_SEDT *edt);

/*���õ��б༭���ı�*/
void GCSedtSetText(CTRL_SEDT *edt, const char *text);

/*׷�ӵ��б༭���ı�*/
void GCSedtAddText(CTRL_SEDT *edt, const char *text);

/**********���б༭��**********/

/**********������**********/

#define SCRL_STYLE_HOR		0x0000	/*ˮƽ*/
#define SCRL_STYLE_VER		0x0001	/*��ֱ*/

typedef struct _CTRL_SCRL
{
	CTRL_GOBJ obj;
	long min, max;			/*��С���ֵ*/
	long pos, page;			/*λ��ֵ,ҳ��С*/
	CTRL_BTN *sub;			/*��С��ť*/
	CTRL_BTN *add;			/*����ť*/
	CTRL_BTN *drag;			/*�϶���ť*/
	void (*ChangeProc)(struct _CTRL_SCRL *scl);	/*ֵ�ı䴦����*/
}CTRL_SCRL;	/*������*/

long GCScrlCreate(CTRL_SCRL **scl, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, long min, long max, long pos, long page, void (*ChangeProc)(CTRL_SCRL *scl));

long GCScrlDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCScrlDefDrawProc(CTRL_SCRL *scl);

/*���ù�����λ�ô�С*/
void GCScrlSetSize(CTRL_SCRL *scl, long x, long y, DWORD width, DWORD height);

/*���ù���������*/
long GCScrlSetData(CTRL_SCRL *scl, long min, long max, long pos, long page);

/**********�б��**********/

#define LSTITM_ATTR_SELECTED	0x00000001	/*��ѡ��*/
#define LSTITM_TXT_LEN		128

typedef struct _LIST_ITEM
{
	struct _LIST_ITEM *pre, *nxt;	/*ǰ��ָ��*/
	DWORD attr;			/*����*/
	char text[LSTITM_TXT_LEN];	/*�б�����ı�*/
}LIST_ITEM;	/*�б����*/

typedef struct _CTRL_LST
{
	CTRL_GOBJ obj;
	DWORD ItemCou;		/*������*/
	DWORD CurPos;		/*��ǰ��λ��*/
	DWORD MaxWidth;		/*��ַ������ؿ��*/
	DWORD TextX;		/*�ı�����ƫ��*/
	UDI_AREA cont;		/*���ݻ�ͼ��*/
	LIST_ITEM *item;	/*���б�ָ��*/
	LIST_ITEM *CurItem;	/*��ǰ��*/
	LIST_ITEM *SelItem;	/*��ѡ��*/
	CTRL_SCRL *hscl;	/*�������*/
	CTRL_SCRL *vscl;	/*�ݹ�����*/
	void (*SelProc)(struct _CTRL_LST *lst);	/*��ѡ��ı䴦����*/
}CTRL_LST;	/*�б��*/

long GCLstCreate(CTRL_LST **lst, const CTRL_ARGS *args, DWORD pid, CTRL_GOBJ *ParGobj, void (*SelProc)(CTRL_LST *lst));

long GCLstDefMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN]);

void GCLstDefDrawProc(CTRL_LST *lst);

/*�����б��λ�ô�С*/
void GCLstSetSize(CTRL_LST *lst, long x, long y, DWORD width, DWORD height);

/*������*/
long GCLstInsertItem(CTRL_LST *lst, LIST_ITEM *pre, const char *text, LIST_ITEM **item);

/*ɾ����*/
long GCLstDeleteItem(CTRL_LST *lst, LIST_ITEM *item);

/*ɾ��������*/
long GCLstDelAllItem(CTRL_LST *lst);

#endif

/*	calc.c for ulios application
	���ߣ�����
	���ܣ�����������
	����޸����ڣ�2011-10-30
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

#include "../lib/math.h"

#define WOERR_NOERR		0	// ������ȷ
#define WOERR_ARGUMENT	1	// ��������
#define WOERR_MEMORY	2	// �ڴ����
#define WOERR_VARIABLE	3	// ������������зǷ��ַ�
#define WOERR_FORMAT	4	// ���ʽ��ʽ����
#define WOERR_BRACKETS	5	// ���Ų�ƥ��
#define WOERR_DIVZERO	6	// ������Ϊ��
#define WOERR_MODZERO	7	// �������㱻ģ��Ϊ��
#define WOERR_FMODZERO	8	// �������㱻ģ��Ϊ��
#define WOERR_COMPLEX	9	// ������С���η���������
#define WOERR_ARCCOS	10	// �����Ҷ��������
#define WOERR_ARCSIN	11	// �����Ҷ��������
#define WOERR_LN		12	// ��Ȼ�������������
#define WOERR_LG		13	// ��Ȼ�������������
#define WOERR_SQR		14	// �����������������

#define WOTYP_END		0
#define WOTYP_START		1
#define WOTYP_BINOP		2
#define WOTYP_FUNC		3
#define WOTYP_NEGA		4
#define WOTYP_NUM		5
#define WOTYP_LBRACK	6
#define WOTYP_RBRACK	7

typedef struct _WONODE
{
	union
	{
		struct
		{
			DWORD lev;
			DWORD(*fun)(double*, double);
		}BinOp;	/*��Ԫ����*/
		struct
		{
			DWORD lev;
			DWORD(*fun)(double*);
		}Func;	/*��������*/
		struct
		{
			DWORD lev;
		}Nega;	/*��������*/
		double Num;	/*������*/
	}node;	/*�ڵ�����*/
	DWORD type;	/*�ڵ����ͱ�־*/
	struct _WONODE *nxt;	/*nextָ��*/
}WONODE;	/*�������ͽڵ�*/

#define CTRLCOU		3
#define BINOPCOU	7
#define FUNCCOU		14
#define KEYWORDCOU	(CTRLCOU + BINOPCOU + FUNCCOU)
#define NEGAID		4

#define NEGALEV		3
#define FUNCLEV		4
#define LEVCOU		5

const char *Keyword[] =	/*�ؼ�������*/
{
	"(",	")",	" ",	/*3�������ַ�*/
	"+",	"-",	"*",	"/",	"%",	"#",	"^",	/*7����Ԫ�����*/
	"abs",	"acs",	"asn",	"atn",	"cos",	"ch",	"ep",
	"lg",	"ln",	"sin",	"sh",	"sqr",	"tan",	"th",	/*14������*/
};

DWORD WoAdd(double *x, double y)
{
	*x += y;
	return WOERR_NOERR;
}
DWORD WoDec(double *x, double y)
{
	*x -= y;
	return WOERR_NOERR;
}
DWORD WoMul(double *x, double y)
{
	*x *= y;
	return WOERR_NOERR;
}
DWORD WoDiv(double *x, double y)
{
	if (y == 0.0)
		return WOERR_DIVZERO;
	*x /= y;
	return WOERR_NOERR;
}
DWORD WoMod(double *x, double y)
{
	if ((long)y == 0)
		return WOERR_MODZERO;
	*x = (long)*x % (long)y;
	return WOERR_NOERR;
}
DWORD WoFmod(double *x, double y)
{
	if (y == 0.0)
		return WOERR_FMODZERO;
	*x = fmod(*x, y);
	return WOERR_NOERR;
}
DWORD WoPow(double *x, double y)
{
	if (*x < 0.0 && y != floor(y))
		return WOERR_COMPLEX;
	*x = pow(*x, y);
	return WOERR_NOERR;
}

DWORD WoAbs(double *x)
{
	*x = fabs(*x);
	return WOERR_NOERR;
}
DWORD WoAcs(double *x)
{
	if (*x < -1.0 || *x > 1.0)
		return WOERR_ARCCOS;
	*x = acos(*x);
	return WOERR_NOERR;
}
DWORD WoAsn(double *x)
{
	if (*x < -1.0 || *x > 1.0)
		return WOERR_ARCSIN;
	*x = asin(*x);
	return WOERR_NOERR;
}
DWORD WoAtn(double *x)
{
	*x = atan(*x);
	return WOERR_NOERR;
}
DWORD WoCos(double *x)
{
	*x = cos(*x);
	return WOERR_NOERR;
}
DWORD WoCh(double *x)
{
	*x = cosh(*x);
	return WOERR_NOERR;
}
DWORD WoEp(double *x)
{
	*x = exp(*x);
	return WOERR_NOERR;
}
DWORD WoLg(double *x)
{
	if (*x <= 0.0)
		return WOERR_LG;
	*x = log10(*x);
	return WOERR_NOERR;
}
DWORD WoLn(double *x)
{
	if (*x <= 0.0)
		return WOERR_LN;
	*x = log(*x);
	return WOERR_NOERR;
}
DWORD WoSin(double *x)
{
	*x = sin(*x);
	return WOERR_NOERR;
}
DWORD WoSh(double *x)
{
	*x = sinh(*x);
	return WOERR_NOERR;
}
DWORD WoSqr(double *x)
{
	if (*x < 0.0)
		return WOERR_SQR;
	*x = sqrt(*x);
	return WOERR_NOERR;
}
DWORD WoTan(double *x)
{
	*x = tan(*x);
	return WOERR_NOERR;
}
DWORD WoTh(double *x)
{
	*x = tanh(*x);
	return WOERR_NOERR;
}

DWORD (*WoBinOp[])(double*, double) =	// 7����Ԫ�����
{	WoAdd,	WoDec,	WoMul,	WoDiv,	WoMod,	WoFmod,	WoPow};
const DWORD WoBinLev[] =	//7����Ԫ����ļ���
{	0,		0,		1,		1,		1,		1,		2};
DWORD (*WoFunc[])(double*) =	// 14������
{	WoAbs,	WoAcs,	WoAsn,	WoAtn,	WoCos,	WoCh,	WoEp,	WoLg,	WoLn,	WoSin,	WoSh,	WoSqr,	WoTan,	WoTh};

// ȡ�ú�һ������ļ���
DWORD NxtLev(const WONODE *wol)
{
	while (wol)
	{
		switch (wol->type)
		{
		case WOTYP_BINOP:
			return wol->node.BinOp.lev;
		case WOTYP_FUNC:
			return wol->node.Func.lev;
		case WOTYP_NEGA:
			return wol->node.Nega.lev;
		}
		wol = wol->nxt;
	}
	return 0;
}

// �ַ���ת��Ϊ˫����
double atof(const char *str)
{
	double res, step;

	res = 0.0;
	for (;;)
	{
		if (*str >= '0' && *str <= '9')
			res = res * 10.0 + (*str - '0');
		else
			break;
		str++;
	}
	if (*str != '.')
		return res;
	str++;
	step = 0.1;
	for (;;)
	{
		if (*str >= '0' && *str <= '9')
			res = res + step * (*str - '0');
		else
			break;
		str++;
		step *= 0.1;
	}
	return res;
}

// ���Ը�ʽ�Ƿ���ȷ�����������ͱ�־
DWORD ChkFmtErr(DWORD id, DWORD *type)
{
	static const DWORD FrontErr0[] = {WOTYP_START,	WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_LBRACK,	WOTYP_END};
	static const DWORD FrontErr1[] = {WOTYP_END};
	static const DWORD FrontErr2[] = {WOTYP_START,	WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_LBRACK,	WOTYP_END};
	static const DWORD FrontErr3[] = {WOTYP_FUNC,	WOTYP_NUM,		WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr4[] = {WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_NUM,		WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr5[] = {WOTYP_FUNC,	WOTYP_NUM,		WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr6[] = {WOTYP_NUM,	WOTYP_RBRACK,	WOTYP_END};
	static const DWORD FrontErr7[] = {WOTYP_BINOP,	WOTYP_FUNC,		WOTYP_NEGA,		WOTYP_LBRACK,	WOTYP_END};
	static const DWORD *FrontErr[] = {FrontErr0, FrontErr1, FrontErr2, FrontErr3, FrontErr4, FrontErr5, FrontErr6, FrontErr7};	// ֮ǰ��Ӧ���ֵ�����

	const DWORD *typep;
	for (typep = FrontErr[id]; *typep != WOTYP_END; typep++)
		if (*typep == *type)	// ƥ�䵽��Ӧ���ֵ�����
			return TRUE;	// ��鵽����
	*type = id;
	return FALSE;	// û�д���
}

//	���Ա��ʽ�ؼ��֣���ȡ�ùؼ���
DWORD ChkKeyErr(const char **expre, DWORD *id, const char *variable[], DWORD varn)
{
	DWORD i;
	// �������йؼ���
	for (i = 0; i < KEYWORDCOU; i++)
	{
		const char *curkey = Keyword[i], *cure = *expre;
		while (*curkey)
			if (*cure++ != *curkey++)
				goto nxtkey;
		*id = i;
		*expre = cure;
		return FALSE;
nxtkey:	continue;
	}
	// ���Ա����ؼ���
	for (i = 0; i < varn; i++)
	{
		const char *curkey = variable[i], *cure = *expre;
		while (*curkey)
			if (*cure++ != *curkey++)
				goto nxtvar;
		*id = i + KEYWORDCOU;
		*expre = cure;
		return FALSE;
nxtvar:	continue;
	}
	// ��������
	if (((**expre) >= '0' && (**expre) <= '9') || (**expre) == '.')
	{
		*id = (~0);
		return FALSE;
	}
	return TRUE;
}

// �����ַ������ʽ��������
DWORD workout(const char *expression,	// ���룺���ʽ�ַ���
			  const char *variable[],		// ���룺������������
			  const double value[],		// ���룺����ֵ����
			  DWORD varn,					// ���룺��������
			  double *res)				// �����������
{
	WONODE *wol = NULL, *curwo = NULL;	// ���ʽ������ǰ����ڵ�
	DWORD type = WOTYP_START;	// ��ǰ��������
	DWORD lev = 0;	// ��ǰ���㼶��
	WONODE wobuf[128], *wop;

	if (expression == NULL || res == NULL)
		return WOERR_ARGUMENT;
	if (varn && (variable == NULL || value == NULL))
		return WOERR_ARGUMENT;
	wop = wobuf;
	while (*expression)
	{
		DWORD id;
		WONODE *tmpwo;

		if (ChkKeyErr(&expression, &id, variable, varn))	// ƥ��ؼ���
			return WOERR_VARIABLE;
		// �����ַ�����
		if (id == 0)	// (
		{
			if (ChkFmtErr(WOTYP_LBRACK, &type))
				return WOERR_FORMAT;
			lev += LEVCOU;
			continue;
		}
		if (id == 1)	// )
		{
			if (ChkFmtErr(WOTYP_RBRACK, &type))
				return WOERR_FORMAT;
			lev -= LEVCOU;
			if (lev < 0)
				return WOERR_BRACKETS;
			continue;
		}
		if (id == 2)	// �ո�
			continue;
		// ��������ڵ�
		tmpwo = wop++;
		tmpwo->nxt = NULL;
		if (wol == NULL)
			wol = tmpwo;
		else
			curwo->nxt = tmpwo;
		curwo = tmpwo;
		// �����ַ�����
		if (id == NEGAID && (type == WOTYP_START || type == WOTYP_LBRACK))	// �жϸ���
		{
			if (ChkFmtErr(WOTYP_NEGA, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Nega.lev = lev + NEGALEV;
			continue;
		}
		if (id < CTRLCOU + BINOPCOU)	// �ж϶�Ԫ����
		{
			if (ChkFmtErr(WOTYP_BINOP, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.BinOp.fun = WoBinOp[id - CTRLCOU];
			tmpwo->node.BinOp.lev = lev + WoBinLev[id - CTRLCOU];
			continue;
		}
		if (id < CTRLCOU + BINOPCOU + FUNCCOU)	// �жϺ���
		{
			if (ChkFmtErr(WOTYP_FUNC, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Func.fun = WoFunc[id - CTRLCOU - BINOPCOU];
			tmpwo->node.Func.lev = lev + FUNCLEV;
			continue;
		}
		if (id < CTRLCOU + BINOPCOU + FUNCCOU + varn)	// �жϱ�����ֵ
		{
			if (ChkFmtErr(WOTYP_NUM, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Num = value[id - CTRLCOU - BINOPCOU - FUNCCOU];
			continue;
		}
		if (ChkFmtErr(WOTYP_NUM, &type))	// �жϳ�����ֵ
			return WOERR_FORMAT;
		tmpwo->type = type;
		tmpwo->node.Num = atof(expression);
		while ((*expression >= '0' && *expression <= '9') || *expression == '.')
			expression++;
	}
	if (ChkFmtErr(WOTYP_END, &type))	// ����β״̬
		return WOERR_FORMAT;
	if (lev)	// �������ƥ��״��
		return WOERR_BRACKETS;

	while (wol->nxt)	// ������������
	{
		DWORD prelev = 0;
		WONODE *prewo = NULL;
		curwo = wol;
		while (curwo)
		{
			switch(curwo->type)
			{
			case WOTYP_BINOP:
				lev = curwo->node.BinOp.lev;
				break;
			case WOTYP_FUNC:
				lev = curwo->node.Func.lev;
				break;
			case WOTYP_NEGA:
				lev = curwo->node.Nega.lev;
				break;
			case WOTYP_NUM:
				prewo = curwo;
				curwo = curwo->nxt;
				continue;
			}
			if (lev >= prelev && lev >= NxtLev(curwo->nxt))	// ����Ƿ��ǰ�����㼶���
			{
				DWORD ErrCode;
				switch(curwo->type)
				{
				case WOTYP_BINOP:
					ErrCode = curwo->node.BinOp.fun(&prewo->node.Num, curwo->nxt->node.Num);
					if (ErrCode != WOERR_NOERR)
						return ErrCode;
					curwo = prewo->nxt = curwo->nxt->nxt;
					break;
				case WOTYP_FUNC:
				case WOTYP_NEGA:
					if (curwo->type == WOTYP_NEGA)
						curwo->node.Num = -curwo->nxt->node.Num;
					else
					{
						ErrCode = curwo->node.Func.fun(&curwo->nxt->node.Num);
						if (ErrCode != WOERR_NOERR)
							return ErrCode;
						curwo->node.Num = curwo->nxt->node.Num;
					}
					curwo->type = WOTYP_NUM;
					prewo = curwo;
					curwo = prewo->nxt = curwo->nxt->nxt;
					break;
				}
				prelev = lev;
			}
			else
			{
				prewo = curwo;
				curwo = curwo->nxt;
			}
		}
	}
	*res = wol->node.Num;

	return WOERR_NOERR;
}

const char *WoErrStr[] = {
	NULL,
	"��������",
	"�ڴ����",
	"������������зǷ��ַ�",
	"���ʽ��ʽ����",
	"���Ų�ƥ��",
	"������Ϊ��",
	"�������㱻ģ��Ϊ��",
	"�������㱻ģ��Ϊ��",
	"������С���η���������",
	"�����Ҷ��������",
	"�����Ҷ��������",
	"��Ȼ�������������",
	"��Ȼ�������������",
	"�����������������"
};

/*˫����ת��Ϊ����*/
char *ftoa(char *buf, double n)
{
	char *p, *q;
	DWORD intn;

	if (n < 0.0)
	{
		*buf++ = '-';
		n = -n;
	}
	intn = (DWORD)n;	/*������������*/
	n -= intn;
	q = buf;
	do
	{
		*buf++ = intn % 10 + '0';
		intn /= 10;
	}
	while (intn);
	p = buf - 1;
	while (p > q)	/*��ת�����ַ���*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	if (n >= 1e-16)
	{
		*buf++ = '.';
		q = buf;
		do
		{
			n *= 10.0;
			intn = (DWORD)n;
			n -= intn;
			*buf++ = intn + '0';
		}
		while (buf - q < 16 && n >= 1e-16);
	}
	*buf = '\0';
	return buf;
}

#define WND_WIDTH	244
#define WND_HEIGHT	244
#define EDT_WIDTH	236
#define EDT_HEIGHT	20
#define BTN_WIDTH	36
#define BTN_HEIGHT	24
#define GRA_WIDTH	288
#define GRA_HEIGHT	216
#define SIDE_X		3	/*�����ͻ����߾�*/
#define SIDE_Y		4	/*�ϲ���ͻ����߾�*/

CTRL_WND *wnd;
CTRL_SEDT *edt;
CTRL_BTN *but[42];
BOOL isShowGra;
UDI_AREA GraArea;
long p2d[GRA_WIDTH];
#define P3D_WID	51
float p3d[P3D_WID][P3D_WID], a, b;
long x0, y0;

static const char *strfind(const char *str, char c)
{
	while (*str)
	{
		if (*str == c)
			return str;
		str++;
	}
	return NULL;
}

void SetGraMode(BOOL isGraMode)
{
	long i;
	if (isGraMode)
	{
		GCBtnSetText(but[5], "<<");
		GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, WND_WIDTH + GRA_WIDTH + 4, WND_HEIGHT);
		GCSetArea(&GraArea, GRA_WIDTH, GRA_HEIGHT, &wnd->client, WND_WIDTH - 1, SIDE_Y);
	}
	else
	{
		GCBtnSetText(but[5], ">>");
		GCWndSetSize(wnd, wnd->obj.x, wnd->obj.y, WND_WIDTH, WND_HEIGHT);
	}
	isShowGra = isGraMode;
	GCSetArea(&edt->obj.uda, EDT_WIDTH, EDT_HEIGHT, &wnd->obj.uda, edt->obj.x, edt->obj.y);
	GCSedtDefDrawProc(edt);
	for (i = 0; i < 42; i++)
	{
		GCSetArea(&(but[i]->obj.uda), BTN_WIDTH, BTN_HEIGHT, &wnd->obj.uda, but[i]->obj.x, but[i]->obj.y);
		GCBtnDefDrawProc(but[i]);
	}
	GUIpaint(wnd->obj.gid, 4, 24, WND_WIDTH - 4 * 2, WND_HEIGHT - 20 - 4 * 2);	/*��������ύ*/
}

/*������ת��Ϊ����*/
char *Itoa(char *buf, long n)
{
	static const char num[] = {'0','1','2','3','4','5','6','7','8','9'};
	char *p, *q;

	if (n < 0)
	{
		n = -n;
		*buf++ = '-';
	}
	q = p = buf;
	do
	{
		*p++ = num[n % 10];
		n /= 10;
	}
	while (n);
	buf = p;	/*ȷ���ַ���β��*/
	*p-- = '\0';
	while (p > q)	/*��ת�ַ���*/
	{
		char c = *q;
		*q++ = *p;
		*p-- = c;
	}
	return buf;
}

void Calc2D()
{
	static const char *varnam[] = {"x"};
	double x, y;
	long i;
	for (i = 0; i < GRA_WIDTH; i++)
	{
		x = 0.05 * (i - GRA_WIDTH / 2);	//x��Сת��
		if (workout(edt->text, varnam, &x, 1, &y) == WOERR_NOERR)	//����δ֪������
			p2d[i] = (GRA_HEIGHT / 2) - (long)(20.0 * y);	//y�Ŵ�ת��
		else
			p2d[i] = -1;
	}
}

void Draw2D()
{
	long i;
	GCFillRect(&GraArea, 0, 0, GRA_WIDTH, GRA_HEIGHT, 0xFFFFFF);	// ����
	for (i = 4; i < GRA_WIDTH; i += 20)
		GCFillRect(&GraArea, i, 0, 1, GRA_HEIGHT, 0xFFFF00);
	for (i = 8; i < GRA_HEIGHT; i += 20)
		GCFillRect(&GraArea, 0, i, GRA_WIDTH, 1, 0xFFFF00);
	GCFillRect(&GraArea, GRA_WIDTH / 2, 0, 1, GRA_HEIGHT, 0xFF0000);
	GCFillRect(&GraArea, 0, GRA_HEIGHT / 2, GRA_WIDTH, 1, 0xFF0000);
	for (i = -7; i <= 7; i++)
	{
		char buf[4];
		Itoa(buf, i);
		GCDrawStr(&GraArea, i * 20 + GRA_WIDTH / 2 - 6, GRA_HEIGHT / 2 + 1, buf, 0xFF0000);
	}
	for (i = -5; i <= 5; i++)
	{
		char buf[4];
		Itoa(buf, i);
		GCDrawStr(&GraArea, GRA_WIDTH / 2 - 6, i * 20 + GRA_HEIGHT / 2 + 1, buf, 0xFF0000);
	}
	GCDrawAscii(&GraArea, GRA_WIDTH - 6, GRA_HEIGHT / 2 + 1, 'X', 0);
	GCDrawAscii(&GraArea, GRA_WIDTH / 2 - 6, 1, 'Y', 0);
	for (i = 1; i < GRA_WIDTH; i++)	//�����㻭������ͼ��
	{
		if (p2d[i - 1] != -1 && p2d[i] != -1)	//�쳣�㲻��
			GCDrawLine(&GraArea, i - 1, p2d[i - 1], i, p2d[i], 0);
	}
	GUIpaint(wnd->obj.gid, WND_WIDTH, 20 + SIDE_Y, GRA_WIDTH, GRA_HEIGHT);	/*��������ύ*/
}

void Calc3D()
{
	static const char *varnam[] = {"x", "y"};
	double val[2], z;
	long i, j;
	for (j = 0; j < P3D_WID; j++)
	{
		val[1] = 0.2 * j - 5.0;	//y��Сת��
		for (i = 0; i < P3D_WID; i++)
		{
			val[0] = 0.2 * i - 5.0;	//x��Сת��
			if (workout(edt->text, varnam, val, 2, &z) == WOERR_NOERR)	//����δ֪������
				p3d[i][j] = (float)(5.0 * z);	//z�Ŵ�ת��
			else
				p3d[i][j] = 999999.f;
		}
	}
}

void Draw3D()
{
	long i, j;
	long x3d[P3D_WID][P3D_WID], y3d[P3D_WID][P3D_WID];
	float sa, ca, sb, cb;
	sa = (float)sin(a);
	ca = (float)cos(a);
	sb = (float)sin(b);
	cb = (float)cos(b);
	for (j = 0; j < P3D_WID; j++)	//��ά�任
	{
		for (i = 0; i < P3D_WID; i++)
		{
			float x, y, z;
			x = (i - P3D_WID / 2) * ca + (j - P3D_WID / 2) * sa;
			y = (j - P3D_WID / 2) * ca - (i - P3D_WID / 2) * sa;
			z = p3d[i][j] * cb + x * sb;
			x = (x * cb - p3d[i][j] * sb) * 10.f;
			x3d[i][j] = GRA_WIDTH / 2 + (long)(6000.f * y / (1200.f - x));
			y3d[i][j] = GRA_HEIGHT / 2 - (long)(6000.f * z / (1200.f - x));
		}
	}
	GCFillRect(&GraArea, 0, 0, GRA_WIDTH, GRA_HEIGHT, 0xFFFFFF);	// ����
	for (j = 0; j < P3D_WID; j++)	//�����㻭������ͼ��
	{
		for (i = 1; i < P3D_WID; i++)
		{
			if (p3d[i][j] == 999999.f)	//�쳣�㲻��
				continue;
			if (p3d[i - 1][j] != 999999.f)
				GCDrawLine(&GraArea, x3d[i - 1][j], y3d[i - 1][j], x3d[i][j], y3d[i][j], 0);
			if (p3d[j][i - 1] != 999999.f)
				GCDrawLine(&GraArea, x3d[j][i - 1], y3d[j][i - 1], x3d[j][i], y3d[j][i], 0);
		}
	}
	GUIpaint(wnd->obj.gid, WND_WIDTH, 20 + SIDE_Y, GRA_WIDTH, GRA_HEIGHT);	/*��������ύ*/
}

/*�༭��س�����*/
void EdtEnterProc(CTRL_SEDT *edt)
{
	if (strfind(edt->text, 'y'))	/*��y,������άͼ��*/
	{
		if (!isShowGra)
			SetGraMode(TRUE);
		Calc3D();
		Draw3D();
	}
	else if (strfind(edt->text, 'x'))	/*��x,���ƶ�άͼ��*/
	{
		if (!isShowGra)
			SetGraMode(TRUE);
		Calc2D();
		Draw2D();
	}
	else
	{
		char buf[128];
		double wores;
		DWORD res;

		if (isShowGra)
			SetGraMode(FALSE);
		if ((res = workout(edt->text, NULL, NULL, 0, &wores)) != WOERR_NOERR)
			strcpy(buf, WoErrStr[res]);
		else
			ftoa(buf, wores);
		GCSedtSetText(edt, buf);
	}
}

/*�ȺŰ�ť����*/
void EquPressProc(CTRL_BTN *btn)
{
	EdtEnterProc(edt);
}

/*�˸�ť����*/
void BksPressProc(CTRL_BTN *btn)
{
	THREAD_ID ptid;
	DWORD data[MSG_DATA_LEN];

	ptid.ProcID = INVALID;
	data[MSG_API_ID] = MSG_ATTR_GUI | GM_KEY;
	data[1] = '\b';
	data[GUIMSG_GOBJ_ID] = (DWORD)edt;
	data[MSG_RES_ID] = NO_ERROR;
	edt->obj.MsgProc(ptid, data);
}

/*�����ť����*/
void ClrPressProc(CTRL_BTN *btn)
{
	GCSedtSetText(edt, NULL);
}

/*������ť����*/
void HlpPressProc(CTRL_BTN *btn)
{

}

/*�˳���ť����*/
void QutPressProc(CTRL_BTN *btn)
{
	KExitProcess(NO_ERROR);
}

/*ͼ��ť����*/
void GrpPressProc(CTRL_BTN *btn)
{
	SetGraMode(!isShowGra);
}

/*���ఴť����*/
void OthPressProc(CTRL_BTN *btn)
{
	GCSedtAddText(edt, btn->text);
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	static const char *btnam[] =
	{
		"=",	"�˸�",	"���",	"����",	"�˳�",	">>",
		"sin",	"cos",	"tan",	"asn",	"acs",	"atn",
		"sh",	"ch",	"th",	"ep",	"lg",	"ln",
		"7",	"8",	"9",	"/",	"abs",	"sqr",
		"4",	"5",	"6",	"*",	"%",	"#",
		"1",	"2",	"3",	"-",	"(",	")",
		"0",	".",	"^",	"+",	"x",	"y",
	};
	static void (*BtnPressProc[6])(CTRL_BTN *btn) =
	{EquPressProc, BksPressProc, ClrPressProc, HlpPressProc, QutPressProc, GrpPressProc};
	CTRL_WND *wnd = (CTRL_WND*)data[GUIMSG_GOBJ_ID];
	switch (data[MSG_API_ID] & MSG_API_MASK)
	{
	case GM_CREATE:
		{
			long i, CliX, CliY;
			CTRL_ARGS args;
			GCWndGetClientLoca(wnd, &CliX, &CliY);
			args.width = EDT_WIDTH;
			args.height = EDT_HEIGHT;
			args.x = CliX + SIDE_X;
			args.y = CliY + SIDE_Y;
			args.style = 0;
			args.MsgProc = NULL;
			GCSedtCreate(&edt, &args, wnd->obj.gid, &wnd->obj, NULL, EdtEnterProc);
			args.width = BTN_WIDTH;
			args.height = BTN_HEIGHT;
			args.y = CliY + EDT_HEIGHT + 8;
			for (i = 0; i < 6; i++)
			{
				args.x = CliX + SIDE_X + i * (BTN_WIDTH + 4);
				GCBtnCreate(&but[i], &args, wnd->obj.gid, &wnd->obj, btnam[i], NULL, BtnPressProc[i]);
			}
			for (; i < 42; i++)
			{
				args.x = CliX + SIDE_X + (i % 6) * (BTN_WIDTH + 4);
				args.y = CliY + EDT_HEIGHT + 8 + (i / 6) * (BTN_HEIGHT + 4);
				GCBtnCreate(&but[i], &args, wnd->obj.gid, &wnd->obj, btnam[i], NULL, OthPressProc);
			}
		}
		break;
	case GM_LBUTTONDOWN:
		x0 = (short)(data[5] & 0xFFFF);
		y0 = (short)(data[5] >> 16);
		break;
	case GM_MOUSEMOVE:
		if (data[1] & MUS_STATE_LBUTTON)
		{
			a += (x0 - (short)(data[5] & 0xFFFF)) * 0.007f;
			b += (y0 - (short)(data[5] >> 16)) * 0.007f;
			x0 = (short)(data[5] & 0xFFFF);
			y0 = (short)(data[5] >> 16);
			Draw3D();
		}
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*����16MB���ڴ�*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = WND_WIDTH;
	args.height = WND_HEIGHT;
	args.x = 128;
	args.y = 128;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "���Ӻ���������");

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*�ȴ���Ϣ*/
			break;
		if (GCDispatchMsg(ptid, data) == NO_ERROR)	/*����GUI��Ϣ*/
		{
			if ((data[MSG_API_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*����������,�˳�����*/
				break;
		}
	}
	return NO_ERROR;
}

/*	workout.c for ulios application
	���ߣ�����
	���ܣ��ַ��������ѧ���ʽ�������
	����޸����ڣ�2010-07-10
	��ע�����ڲ�����ѧ���㺯��
*/

#include "../driver/basesrv.h"
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
	"abs",	"acs",	"asn",	"atn",	"cos",	"ch",	"exp",
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
DWORD WoExp(double *x)
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
{	WoAbs,	WoAcs,	WoAsn,	WoAtn,	WoCos,	WoCh,	WoExp,	WoLg,	WoLn,	WoSin,	WoSh,	WoSqr,	WoTan,	WoTh};

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

int main(int argc, char *argv[])
{
	double wores;
	DWORD res;
	int i;

	if (argc <= 1)
	{
		CUIPutS(
			"����:���ʽ1 [���ʽ2] [���ʽ3] ...\n"
			"���ʽΪ��׼����ѧ���ʽ\n"
			"��ʹ�����ţ���(1+2)*abs(-3)��sin(3.14)\n"
			"��ʹ�ö�Ԫ���㣺��+ ��- ��* ��/ ����ģ% ʵ��ģ# ��^\n"
			"��ʹ�ú������£�\n"
			"����ֵabs ������acs ������asn ������atn ����cos ˫������ch e����exp\n"
			"���ö���lg ��Ȼ����ln ����sin ˫������sh ����sqr ����tan ˫������th\n"
		);
		return NO_ERROR;
	}
	for (i = 1; i < argc; i++)
	{
		char buf[128], *bufp;

		bufp = strcpy(buf, argv[i]) - 1;
		if ((res = workout(argv[i], NULL, NULL, 0, &wores)) != WOERR_NOERR)
		{
			bufp = strcpy(bufp, "�޷�����:") - 1;
			bufp = strcpy(bufp, WoErrStr[res]) - 1;
			strcpy(bufp, "\n");
		}
		else
		{
			bufp = strcpy(bufp, " = ") - 1;
			bufp = ftoa(bufp, wores);
			strcpy(bufp, "\n");
		}
		CUIPutS(buf);
	}
	return NO_ERROR;
}

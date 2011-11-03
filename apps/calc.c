/*	calc.c for ulios application
	作者：孙亮
	功能：计算器程序
	最后修改日期：2011-10-30
*/

#include "../lib/malloc.h"
#include "../lib/gclient.h"

#include "../lib/math.h"

#define WOERR_NOERR		0	// 运算正确
#define WOERR_ARGUMENT	1	// 参数错误
#define WOERR_MEMORY	2	// 内存错误
#define WOERR_VARIABLE	3	// 变量名错误或含有非法字符
#define WOERR_FORMAT	4	// 表达式格式错误
#define WOERR_BRACKETS	5	// 括号不匹配
#define WOERR_DIVZERO	6	// 被除数为零
#define WOERR_MODZERO	7	// 整数运算被模数为零
#define WOERR_FMODZERO	8	// 浮点运算被模数为零
#define WOERR_COMPLEX	9	// 负数的小数次方产生复数
#define WOERR_ARCCOS	10	// 反余弦定义域错误
#define WOERR_ARCSIN	11	// 反正弦定义域错误
#define WOERR_LN		12	// 自然对数定义域错误
#define WOERR_LG		13	// 自然对数定义域错误
#define WOERR_SQR		14	// 开方函数定义域错误

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
		}BinOp;	/*二元运算*/
		struct
		{
			DWORD lev;
			DWORD(*fun)(double*);
		}Func;	/*函数运算*/
		struct
		{
			DWORD lev;
		}Nega;	/*负数运算*/
		double Num;	/*运算数*/
	}node;	/*节点数据*/
	DWORD type;	/*节点类型标志*/
	struct _WONODE *nxt;	/*next指针*/
}WONODE;	/*计算类型节点*/

#define CTRLCOU		3
#define BINOPCOU	7
#define FUNCCOU		14
#define KEYWORDCOU	(CTRLCOU + BINOPCOU + FUNCCOU)
#define NEGAID		4

#define NEGALEV		3
#define FUNCLEV		4
#define LEVCOU		5

const char *Keyword[] =	/*关键字数组*/
{
	"(",	")",	" ",	/*3个控制字符*/
	"+",	"-",	"*",	"/",	"%",	"#",	"^",	/*7个二元运算符*/
	"abs",	"acs",	"asn",	"atn",	"cos",	"ch",	"exp",
	"lg",	"ln",	"sin",	"sh",	"sqr",	"tan",	"th",	/*14个函数*/
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

DWORD (*WoBinOp[])(double*, double) =	// 7个二元运算符
{	WoAdd,	WoDec,	WoMul,	WoDiv,	WoMod,	WoFmod,	WoPow};
const DWORD WoBinLev[] =	//7个二元运算的级别
{	0,		0,		1,		1,		1,		1,		2};
DWORD (*WoFunc[])(double*) =	// 14个函数
{	WoAbs,	WoAcs,	WoAsn,	WoAtn,	WoCos,	WoCh,	WoExp,	WoLg,	WoLn,	WoSin,	WoSh,	WoSqr,	WoTan,	WoTh};

// 取得后一个运算的级别
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

// 字符串转化为双精度
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

// 测试格式是否正确，并设置类型标志
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
	static const DWORD *FrontErr[] = {FrontErr0, FrontErr1, FrontErr2, FrontErr3, FrontErr4, FrontErr5, FrontErr6, FrontErr7};	// 之前不应出现的类型

	const DWORD *typep;
	for (typep = FrontErr[id]; *typep != WOTYP_END; typep++)
		if (*typep == *type)	// 匹配到不应出现的类型
			return TRUE;	// 检查到错误
	*type = id;
	return FALSE;	// 没有错误
}

//	测试表达式关键字，并取得关键字
DWORD ChkKeyErr(const char **expre, DWORD *id, const char *variable[], DWORD varn)
{
	DWORD i;
	// 测试已有关键字
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
	// 测试变量关键字
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
	// 测试数字
	if (((**expre) >= '0' && (**expre) <= '9') || (**expre) == '.')
	{
		*id = (~0);
		return FALSE;
	}
	return TRUE;
}

// 进行字符串表达式解析计算
DWORD workout(const char *expression,	// 输入：表达式字符串
			  const char *variable[],		// 输入：变量名称数组
			  const double value[],		// 输入：变量值数组
			  DWORD varn,					// 输入：变量数量
			  double *res)				// 输出：计算结果
{
	WONODE *wol = NULL, *curwo = NULL;	// 表达式链表，当前计算节点
	DWORD type = WOTYP_START;	// 当前运算类型
	DWORD lev = 0;	// 当前运算级别
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

		if (ChkKeyErr(&expression, &id, variable, varn))	// 匹配关键字
			return WOERR_VARIABLE;
		// 控制字符处理
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
		if (id == 2)	// 空格
			continue;
		// 分配运算节点
		tmpwo = wop++;
		tmpwo->nxt = NULL;
		if (wol == NULL)
			wol = tmpwo;
		else
			curwo->nxt = tmpwo;
		curwo = tmpwo;
		// 运算字符处理
		if (id == NEGAID && (type == WOTYP_START || type == WOTYP_LBRACK))	// 判断负号
		{
			if (ChkFmtErr(WOTYP_NEGA, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Nega.lev = lev + NEGALEV;
			continue;
		}
		if (id < CTRLCOU + BINOPCOU)	// 判断二元运算
		{
			if (ChkFmtErr(WOTYP_BINOP, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.BinOp.fun = WoBinOp[id - CTRLCOU];
			tmpwo->node.BinOp.lev = lev + WoBinLev[id - CTRLCOU];
			continue;
		}
		if (id < CTRLCOU + BINOPCOU + FUNCCOU)	// 判断函数
		{
			if (ChkFmtErr(WOTYP_FUNC, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Func.fun = WoFunc[id - CTRLCOU - BINOPCOU];
			tmpwo->node.Func.lev = lev + FUNCLEV;
			continue;
		}
		if (id < CTRLCOU + BINOPCOU + FUNCCOU + varn)	// 判断变量数值
		{
			if (ChkFmtErr(WOTYP_NUM, &type))
				return WOERR_FORMAT;
			tmpwo->type = type;
			tmpwo->node.Num = value[id - CTRLCOU - BINOPCOU - FUNCCOU];
			continue;
		}
		if (ChkFmtErr(WOTYP_NUM, &type))	// 判断常数数值
			return WOERR_FORMAT;
		tmpwo->type = type;
		tmpwo->node.Num = atof(expression);
		while ((*expression >= '0' && *expression <= '9') || *expression == '.')
			expression++;
	}
	if (ChkFmtErr(WOTYP_END, &type))	// 检查结尾状态
		return WOERR_FORMAT;
	if (lev)	// 检查括号匹配状况
		return WOERR_BRACKETS;

	while (wol->nxt)	// 进行链表收缩
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
			if (lev >= prelev && lev >= NxtLev(curwo->nxt))	// 检查是否比前后运算级别高
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
	"参数错误",
	"内存错误",
	"变量名错误或含有非法字符",
	"表达式格式错误",
	"括号不匹配",
	"被除数为零",
	"整数运算被模数为零",
	"浮点运算被模数为零",
	"负数的小数次方产生复数",
	"反余弦定义域错误",
	"反正弦定义域错误",
	"自然对数定义域错误",
	"自然对数定义域错误",
	"开方函数定义域错误"
};

/*双精度转化为数字*/
char *ftoa(char *buf, double n)
{
	char *p, *q;
	DWORD intn;

	if (n < 0.0)
	{
		*buf++ = '-';
		n = -n;
	}
	intn = (DWORD)n;	/*处理整数部分*/
	n -= intn;
	q = buf;
	do
	{
		*buf++ = intn % 10 + '0';
		intn /= 10;
	}
	while (intn);
	p = buf - 1;
	while (p > q)	/*翻转整数字符串*/
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

CTRL_SEDT *edt;

/*编辑框回车处理*/
void EdtEnterProc(CTRL_SEDT *edt)
{
	double wores;
	DWORD res;
	char buf[128], *bufp;

	if ((res = workout(edt->text, NULL, NULL, 0, &wores)) != WOERR_NOERR)
		strcpy(buf, WoErrStr[res]);
	else
		ftoa(buf, wores);
	GCSedtSetText(edt, buf);
}

/*等号按钮处理*/
void EquPressProc(CTRL_BTN *btn)
{
	EdtEnterProc(edt);
}

/*退格按钮处理*/
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

/*清除按钮处理*/
void ClrPressProc(CTRL_BTN *btn)
{
	GCSedtSetText(edt, NULL);
}

/*帮助按钮处理*/
void HlpPressProc(CTRL_BTN *btn)
{

}

/*退出按钮处理*/
void QutPressProc(CTRL_BTN *btn)
{
	KExitProcess(NO_ERROR);
}

/*图像按钮处理*/
void GrpPressProc(CTRL_BTN *btn)
{

}

/*其余按钮处理*/
void OthPressProc(CTRL_BTN *btn)
{
	GCSedtAddText(edt, btn->text);
}

long MainMsgProc(THREAD_ID ptid, DWORD data[MSG_DATA_LEN])
{
	static const char *btnam[] =
	{
		"=",	"退格",	"清除",	"帮助",	"退出",	">>",
		"sin",	"cos",	"tan",	"asn",	"acs",	"atn",
		"sh",	"ch",	"th",	"exp",	"lg",	"ln",
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
			args.width = 236;
			args.height = 20;
			args.x = CliX + 3;
			args.y = CliY + 4;
			args.style = 0;
			args.MsgProc = NULL;
			GCSedtCreate(&edt, &args, wnd->obj.gid, &wnd->obj, NULL, EdtEnterProc);
			args.width = 36;
			args.height = 24;
			args.y = CliY + 28;
			for (i = 0; i < 6; i++)
			{
				args.x = CliX + 3 + i * 40;
				GCBtnCreate(NULL, &args, wnd->obj.gid, &wnd->obj, btnam[i], BtnPressProc[i]);
			}
			for (; i < 42; i++)
			{
				args.x = CliX + 3 + (i % 6) * 40;
				args.y = CliY + 28 + (i / 6) * 28;
				GCBtnCreate(NULL, &args, wnd->obj.gid, &wnd->obj, btnam[i], OthPressProc);
			}
		}
		break;
	}
	return GCWndDefMsgProc(ptid, data);
}

int main()
{
	CTRL_WND *wnd;
	CTRL_ARGS args;
	long res;

	if ((res = InitMallocTab(0x1000000)) != NO_ERROR)	/*设置16MB堆内存*/
		return res;
	if ((res = GCinit()) != NO_ERROR)
		return res;
	args.width = 244;
	args.height = 244;
	args.x = 100;
	args.y = 100;
	args.style = WND_STYLE_CAPTION | WND_STYLE_BORDER | WND_STYLE_CLOSEBTN;
	args.MsgProc = MainMsgProc;
	GCWndCreate(&wnd, &args, 0, NULL, "可视函数计算器");

	for (;;)
	{
		THREAD_ID ptid;
		DWORD data[MSG_DATA_LEN];

		if ((res = KRecvMsg(&ptid, data, INVALID)) != NO_ERROR)	/*等待消息*/
			break;
		if (GCDispatchMsg(ptid, data) == GC_ERR_INVALID_GUIMSG)	/*非GUI消息另行处理*/
			;
		else if ((data[MSG_ATTR_ID] & MSG_API_MASK) == GM_DESTROY && data[GUIMSG_GOBJ_ID] == (DWORD)wnd)	/*销毁主窗体,退出程序*/
			break;
	}
	return NO_ERROR;
}

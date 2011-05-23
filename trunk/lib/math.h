/*	math.h for ulios
	���ߣ�����
	���ܣ���ѧ������
	����޸����ڣ�2009-05-26
*/

#ifndef	_MATH_H_
#define	_MATH_H_

#define DBL_MAX	1.7976931348623158e+308
#define DBL_MIN	2.2250738585072014e-308

/*����x/y������*/
static inline double fmod(double x, double y)
{
	register double ret;
	__asm__(
		"1: fprem\n\t"
		"fstsw %%ax\n\t"
		"sahf\n\t"
		"jp 1b"
		: "=t"(ret)
		: "0"(x), "u"(y)
		: "ax", "cc"
	);
	return ret;
}

/*��������x�ֽ���������ֺ�С�����֡�����С�����֣����������ִ���* iptr��ָ�ڴ��С�*/
static inline double modf(double x, double *iptr)
{
	double ret = fmod(x, 1.0);
	*iptr = x - ret;
	return ret;
}

/*װ�ظ�������v��β����eΪָ��*/
static inline double ldexp(double v, int e)
{
	double temp1, texp, temp2;
	texp = e;
	__asm__(
		"fscale"
		: "=u"(temp2), "=t"(temp1)
		: "0"(texp), "1"(v)
	);
	return temp1;
}

/*�Ѹ�����x�ֽ��β����ָ����x=m*2^exptr��mΪ���С��������β��m������ָ������exptr�С�*/
static inline double frexp(double x, int *exptr)
{
	union
	{
		double d;
		unsigned char c[8];
	}u;
	u.d = x;
	//�õ����룬����ȥ1022�õ�ָ��ֵ��
	*exptr = (int)(((u.c[7] & 0x7F) << 4) | (u.c[6] >> 4)) - 1022;
	//��ָ��������Ϊ0x03FE
	u.c[7] &= 0x80;
	u.c[7] |= 0x3F;
	u.c[6] &= 0x0F;
	u.c[6] |= 0xE0;
	return u.d;
}

/*ƽ����*/
static inline double sqrt(double x)
{
	register double ret;
	__asm__(
		"fsqrt"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

/*e��x����*/
static inline double exp(double x)
{
	register double ret, value;
	__asm__(
		"fldl2e;"
		"fmul %%st(1);"
		"fst %%st(1);\n\t"
		"frndint;"
		"fxch;\n\t"
		"fsub %%st(1);"
		"f2xm1"
		: "=t"(ret), "=u"(value)
		: "0"(x)
	);
	ret += 1.0;
	__asm__(
		"fscale"
		: "=t"(ret)
		: "0"(ret), "u"(value)
	);
	return ret;
}

/*����ֵ*/
static inline double fabs(double x)
{
	register double ret;
	__asm__ (
		"fabs"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

/*˫������ֵ*/
static inline double tanh(double x)
{
	double ret, temp;
	if (x > 50.0)
		return 1.0;
	else if (x < -50.0)
		return -1.0;
	else
	{
		ret = exp(x);
		temp = 1.0 / ret;
		return (ret - temp) / (ret + temp);
	}
}

/*˫������ֵ*/
static inline double sinh(double x)
{
	double ret;
	if (x >= 0.0)
	{
		ret = exp(x);
		return (ret - 1.0 / ret) / 2.0;
	}
	else
	{
		ret = exp(-x);
		return (1.0 / ret - ret) / 2.0;
	}
}

/*˫������ֵ*/
static inline double cosh(double x)
{
	double ret;
	ret = exp(fabs(x));
	return (ret + 1.0 / ret) / 2.0;
}

/*������ֵ*/
static inline double atan(double x)
{
	register double ret;
	__asm__
	(
		"fld1\n\t"
		"fpatan"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

/*x/y�ķ�����*/
static inline double atan2(double x, double y)
{
	register double ret;
	__asm__(
		"fpatan\n\t"
		"fld %%st(0)"
		: "=t"(ret)
		: "0"(y), "u"(x)
	);
	return ret;
}

/*������ֵ*/
static inline double asin(double x)
{
	return atan2(x, sqrt(1.0 - x * x));
}

/*������ֵ*/
static inline double acos(double x)
{
	return atan2(sqrt(1.0 - x * x), x);
}

/*����ȡ��*/
static inline double floor(double x)
{
	register double ret;
	unsigned short int temp1, temp2;
	__asm__("fnstcw %0": "=m"(temp1));
	temp2 = (temp1 & 0xF3FF) | 0x0400;
	__asm__("fldcw %0":: "m"(temp2));
	__asm__("frndint": "=t"(ret): "0"(x));
	__asm__("fldcw %0":: "m"(temp1));
	return ret;
}

/*����ȡ��*/
static inline double ceil(double x)
{
	register double ret;
	unsigned short int temp1, temp2;
	__asm__("fnstcw %0": "=m"(temp1));
	temp2 = (temp1 & 0xF3FF) | 0x0800;
	__asm__("fldcw %0":: "m"(temp2));
	__asm__("frndint": "=t"(ret): "0"(x));
	__asm__("fldcw %0":: "m"(temp1));
	return ret;
}

/*10��x����*/
static inline double pow10(double x)
{
	register double ret, value;
	__asm__(
		"fldl2t;\n\t"
		"fmul %%st(1);\n\t"
		"fst %%st(1);\n\t"
		"frndint;\n\t"
		"fxch;\n\t"
		"fsub %%st(1);\n\t"
		"f2xm1;\n\t"
		: "=t"(ret), "=u"(value)
		: "0"(x)
	);
	ret += 1.0;
	__asm__(
		"fscale"
		: "=t"(ret)
		: "0"(ret), "u"(value)
	);
	return ret;
}

/*���ö���*/
static inline double log10(double x)
{
	register double ret;
	__asm__(
		"fldlg2\n\t"
		"fxch\n\t"
		"fyl2x"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

/*��2Ϊ�׵Ķ���*/
static inline double log2(double x)
{
	register double ret;
	__asm__(
		"fld1\n\t"
		"fxch\n\t"
		"fyl2x"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

/*��Ȼ����*/
static inline double log(double x)
{
	register double ret;
	__asm__(
		"fldln2\n\t"
		"fxch\n\t"
		"fyl2x"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

/*x��y����*/
static inline double pow(double x, double y)
{
	register double ret, value;
	double r = 1.0;
	long p = (long) y;
	if (x == 0.0 && y > 0.0)
		return 0.0;
	if (y == (double) p)
	{
		if (p == 0)
			return 1.0;
		if (p < 0)
		{
			p = -p;
			x = 1.0 / x;
		}
		while (1)
		{
			if (p & 1)
				r *= x;
			p >>= 1;
			if (p == 0)
				return r;
			x *= x;
		}
	}
	__asm__(
		"fmul %%st(1);"
		"fst %%st(1);"
		"frndint;\n\t"
		"fxch;\n\t"
		"fsub %%st(1);\n\t"
		"f2xm1;\n\t"
		: "=t"(ret), "=u"(value)
		: "0"(log2(x)), "1"(y)
	);
	ret += 1.0;
	__asm__(
		"fscale"
		: "=t"(ret)
		: "0"(ret), "u"(value)
	);
	return ret;
}

/*����ֵ*/
static inline double tan(double x)
{
	register double ret;
	register double value;
	__asm__(
		"fptan"
		: "=t"(value), "=u"(ret)
		: "0"(x)
	);
	return ret;
}

/*����ֵ*/
static inline double cos(double x)
{
	register double ret;
	__asm__(
		"fcos"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

/*����ֵ*/
static inline double sin(double x)
{
	register double ret;
	__asm__ (
		"fsin"
		: "=t"(ret)
		: "0"(x)
	);
	return ret;
}

#endif

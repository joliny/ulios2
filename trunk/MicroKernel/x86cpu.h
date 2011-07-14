/*	x86cpu.h for ulios
	���ߣ�����
	���ܣ�Intel Pentium CPU ��ؽṹ����
	����޸����ڣ�2009-05-26
*/

#ifndef	_X86CPU_H_
#define	_X86CPU_H_

#include "ulidef.h"

/*EFLAGS��־λ����*/
#define EFLAGS_CF		0x00000001
#define EFLAGS_PF		0x00000004
#define EFLAGS_AF		0x00000010
#define EFLAGS_ZF		0x00000040
#define EFLAGS_SF		0x00000080
#define EFLAGS_TF		0x00000100	/*�����־*/
#define EFLAGS_IF		0x00000200	/*�ж����ñ�־*/
#define EFLAGS_DF		0x00000400	/**/
#define EFLAGS_OF		0x00000800	/**/
#define EFLAGS_IOPL		0x00003000	/*I/O��Ȩ��*/
#define EFLAGS_NT		0x00004000	/*Ƕ�������־*/
#define EFLAGS_RF		0x00010000	/*�ָ���־*/
#define EFLAGS_VM		0x00020000	/*����8086ģʽ*/
#define EFLAGS_AC		0x00040000	/*������*/
#define EFLAGS_VIF		0x00080000	/*�����жϱ�־*/
#define EFLAGS_VIP		0x00100000	/*�����жϵȴ�*/
#define EFLAGS_ID		0x00200000	/*ʶ��λ*/

/*������������������*/
#define DESC_ATTR_DT	0x00001000	/*0:ϵͳ������������������1:�洢��������*/
#define DESC_ATTR_DPL0	0x00000000	/*ϵͳ��Ȩ��*/
#define DESC_ATTR_DPL1	0x00002000
#define DESC_ATTR_DPL2	0x00004000
#define DESC_ATTR_DPL3	0x00006000	/*�û���Ȩ��*/
#define DESC_ATTR_P		0x00008000	/*0:��������Ч1:��������Ч*/
/*����������������*/
#define SEG_ATTR_AVL	0x00100000	/*����Զ�*/
#define SEG_ATTR_G		0x00800000	/*0:�ν�������Ϊ�ֽ�1:�ν�������Ϊ4KB*/
/*�洢������������*/
#define STOSEG_ATTR_T_A		0x00000100	/*0:δ������1:���ʹ�(CPU�Զ��޸�)*/
#define STOSEG_ATTR_T_D_W	0x00000200	/*0:���ݶβ���д1:���ݶο�д*/
#define STOSEG_ATTR_T_D_ED	0x00000400	/*0:���ݶ���߶���չ1:���ݶ���Ͷ���չ*/
#define STOSEG_ATTR_T_C_R	0x00000200	/*0:����β��ɶ�1:����οɶ�*/
#define STOSEG_ATTR_T_C_C	0x00000400	/*0:��ͨ�����1:һ�´����*/
#define STOSEG_ATTR_T_E		0x00000800	/*0:����ִ��(���ݶ�)1:��ִ��(�����)*/
#define STOSEG_ATTR_D		0x00400000	/*0:16λ��1:32λ��*/
/*ϵͳ�����������Ź�ռ����*/
/*#define SYS_ATTR_T_UDEF0	0x00000000	δ����*/
/*#define SYS_ATTR_T_2TSS	0x00000100	����286TSS*/
#define SYSSEG_ATTR_T_LDT	0x00000200	/*LDT*/
/*#define SYS_ATTR_T_B2TSS	0x00000300	æ��286TSS*/
/*#define SYS_ATTR_T_2CALG	0x00000400	286������*/
#define GATE_ATTR_T_TASK	0x00000500	/*������*/
/*#define SYS_ATTR_T_2INTG	0x00000600	286�ж���*/
/*#define SYS_ATTR_T_2TRPG	0x00000700	286������*/
/*#define SYS_ATTR_T_UDEF8	0x00000800	δ����*/
#define SYSSEG_ATTR_T_TSS	0x00000900	/*����386TSS*/
/*#define SYS_ATTR_T_UDEFA	0x00000A00	δ����*/
#define SYSSEG_ATTR_T_BTSS	0x00000B00	/*æ��386TSS*/
#define GATE_ATTR_T_CALL	0x00000C00	/*386������*/
/*#define SYS_ATTR_T_UDEFD	0x00000D00	δ����*/
#define GATE_ATTR_T_INTR	0x00000E00	/*386�ж���*/
#define GATE_ATTR_T_TRAP	0x00000F00	/*386������*/

typedef struct _SEG_GATE_DESC
{
	DWORD d0;
	DWORD d1;
}SEG_GATE_DESC;	/*����������*/

typedef struct _TSS
{
	DWORD link;		/*ǰһ�����TSS������ѡ����*/
	struct _TSS_STK
	{
		DWORD esp;	/*�ڲ��ջָ��*/
		DWORD ss;	/*�ڲ��ջ��*/
	}stk[3];
	DWORD cr3;		/*ҳĿ¼ָ��*/

	void *eip;		/*�Ĵ���״̬*/
	DWORD eflags;
	DWORD eax;
	DWORD ecx;
	DWORD edx;
	DWORD ebx;
	DWORD esp;
	DWORD ebp;
	DWORD esi;
	DWORD edi;
	DWORD es;
	DWORD cs;
	DWORD ss;
	DWORD ds;
	DWORD fs;
	DWORD gs;

	DWORD ldtr;		/*LDT�Ĵ���*/
	WORD t, io;		/*����,I/O���λͼƫ��*/
}TSS;	/*����״̬�ṹ*/

typedef struct _I387
{
	DWORD cwd;	/*������*/
	DWORD swd;	/*״̬��*/
	DWORD twd;	/*�����*/
	DWORD fip;	/*ָ��ָ��ƫ��*/
	DWORD fcs;	/*ָ��ָ��ѡ��*/
	DWORD foo;	/*������ƫ��*/
	DWORD fos;	/*������ѡ��*/
	DWORD st[20];	/*10��8�ֽ�FP�Ĵ���*/
}I387;	/*I387����Э�������Ĵ���*/

typedef struct _I387SSE
{
	WORD cwd;	/*������*/
	WORD swd;	/*״̬��*/
	WORD twd;	/*�����*/
	WORD fop;	/*���һ��ָ�������*/
	union _I387SSE_REG
	{
		struct _I387SSE_INS_R
		{
			QWORD rip;	/*ָ��ָ��*/
			QWORD rdp;	/*����ָ��*/
		}rreg;
		struct _I387SSE_INS_F
		{
			DWORD fip;	/*FPUָ��ָ��ƫ��*/
			DWORD fcs;	/*FPUָ��ָ��ѡ��*/
			DWORD foo;	/*FPU������ƫ��*/
			DWORD fos;	/*FPU������ѡ��*/
		}freg;
	}reg;
	DWORD mxcsr;		/*MXCSR�Ĵ���״̬*/
	DWORD mxcsr_mask;	/*MXCSRλ�ɰ�*/
	DWORD st[32];		/*8��16�ֽ�FP�Ĵ���*/
	DWORD xmm[64];		/*16��16�ֽ�XMM�Ĵ���*/
	DWORD padding[12];
	union _I387SSE_PAD
	{
		DWORD padding1[12];
		DWORD sw_reserved[12];
	}pad;
}__attribute__((aligned(16))) I387SSE;	/*SSE�������Ĵ���*/

/*ҳĿ¼��ҳ����*/
#define PAGE_ATTR_P		0x00000001	/*0:ҳ������1:ҳ����*/
#define PAGE_ATTR_W		0x00000002	/*0:ֻ��1:��д*/
#define PAGE_ATTR_U		0x00000004	/*0:ֻϵͳ(0,1,2)����1:�û�(3)�ɷ���*/
#define PAGE_ATTR_PWT	0x00000008	/*0:������ָ���ҳ��ҳ����д����ʱʹ�û�д��1:ʹ��͸д����CR0��CDλΪ1ʱPWT��PCD��־���ᱻ����*/
#define PAGE_ATTR_PCD	0x00000010	/*0:������ָ���ҳ��ҳ����Ա�����1:������*/
#define PAGE_ATTR_A		0x00000020	/*0:δ������1:���ʹ�(CPU�Զ��޸�)*/
#define PAGE_ATTR_D		0x00000040	/*0:δ��д1:��д��(CPU�Զ��޸�)*/
#define PAGE_ATTR_PS	0x00000080	/*0:ҳ���СΪ4KB��ҳĿ¼��ָ��һ��ҳ��1:ҳ���СΪ4MB��2MB(��CR4��PAEλ����)��ҳĿ¼��ֱ��ָ��һ��ҳ*/
#define PAGE_ATTR_G		0x00000100	/*0:��ͨҳ1:ȫ��ҳ(��PGEΪ1ʱ��������CR3������ֵ�������л���ָ��ȫ��ҳ�ı�����TLB����Ȼ��Ч�������־ֻ��ֱ��ָ��һ��ҳ�ı�������Ч������ᱻ���ԡ�)*/
#define PAGE_ATTR_AVL	0x00000E00	/*����Զ�*/

typedef DWORD PAGE_DESC;	/*ҳĿ¼��ҳ����*/

/*���ö�������*/
static inline void SetSegDesc(SEG_GATE_DESC *desc, DWORD base, DWORD limit, DWORD attr)
{
	desc->d0 = (limit & 0x0000FFFF) | (base << 16);
	desc->d1 = ((base >> 16) & 0x000000FF) | attr | (limit & 0x000F0000) | (base & 0xFF000000);
}

/*ȡ�ö���������ַ*/
static inline DWORD GetSegDescBase(SEG_GATE_DESC *desc)
{
	return (desc->d0 >> 16) | ((desc->d1 << 16) & 0x00FF0000) | (desc->d1 & 0xFF000000);
}

/*ȡ�ö�����������*/
static inline DWORD GetSegDescLimit(SEG_GATE_DESC *desc)
{
	return (desc->d0 & 0x0000FFFF) | (desc->d1 & 0x000F0000);
}

/*���ö���������ַ*/
static inline void SetSegDescBase(SEG_GATE_DESC *desc, DWORD base)
{
	desc->d0 = (desc->d0 & 0x0000FFFF) | (base << 16);
	desc->d1 = (desc->d1 & 0x00FFFF00) | ((base >> 16) & 0x000000FF) | (base & 0xFF000000);
}

/*���ö�����������*/
static inline void SetSegDescLimit(SEG_GATE_DESC *desc, DWORD limit)
{
	desc->d0 = (desc->d0 & 0xFFFF0000) | (limit & 0x0000FFFF);
	desc->d1 = (desc->d1 & 0xFFF0FFFF) | (limit & 0x000F0000);
}

/*������������*/
static inline void SetGate(SEG_GATE_DESC *desc, DWORD seg, DWORD off, DWORD attr)
{
	desc->d0 = (off & 0x0000FFFF) | (seg << 16);
	desc->d1 = (off & 0xFFFF0000) | attr;
}

/*���CR0��TS��־*/
static inline void ClearTs()
{
	__asm__("clts");
}

/*ˢ��ҳ����*/
static inline void RefreshTlb()
{
	register DWORD reg;
	__asm__ __volatile__("movl %%cr3, %0;movl %0, %%cr3": "=r"(reg));
}

/*ȡ��ҳ�������Ե�ַ*/
static inline void *GetPageFaultAddr()
{
	register void *_addr;
	__asm__("movl %%cr2, %0": "=r"(_addr));
	return _addr;
}

#endif

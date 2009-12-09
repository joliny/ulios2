/*
���ߣ�����
���ܣ�fat32�ļ�ϵͳ��֧��
*/
#include"..\ulidef.h"

typedef struct
{	word bps;	/*ÿ�����ֽ���512*/
	byte spc;	/***ÿ��������*/
	word ressec;/***����������(��һ��FAT��ʼ֮ǰ��������)*/
	byte fats;	/***FAT��һ��Ϊ2*/
	word rtents;/*��Ŀ¼����(FAT32Ϊ0)*/
	word smlsec;/*С������(FAT32Ϊ0)*/
	byte media;	/*ý��������Ӳ��0xF8*/
	word spf;	/*ÿFAT������(FAT32Ϊ0)*/
	word spt;	/*ÿ��������*/
	word heads;	/*��ͷ��*/
	dword relsec;/*��������������ǰ����������*/
	dword totsec;/***��������*/
	dword spfat;/***ÿFAT������FAT32ʹ��*/
	word exflg;	/*��չ��־*/
	word fsver;	/*�ļ�ϵͳ�汾*/
	dword rtclu;/***��Ŀ¼�غ�*/
	word fsinfo;/***�ļ�ϵͳ��Ϣ������һ��Ϊ1*/
	word bkbot;	/*������������6*/
	byte reser[12];	/*����12�ֽ�*/
	/*����Ϊ��չBPB*/
	byte pdn;	/*������������,��һ��������Ϊ0x80*/
	byte exres;	/*����*/
	byte exbtsg;/*��չ������ǩΪ0x29*/
	dword volume;/*�������,�������ִ���*/
	byte vollab[11];/*���*/
	byte fsid[8];/*ϵͳID*/
}__attribute__((packed)) BPBfat32;	/*FAT32��BPB*/
typedef struct
{	byte spc;	/*ÿ��������*/
	byte fats;	/*fat��Ŀ*/
	byte fs_l;	/*��Ϣ�ṹ�����ź���*/
	word ressec;/*����������(FAT֮ǰ������)*/
	word fsinfo;/*�ļ�ϵͳ��Ϣ������һ��Ϊ1*/
	dword spfat;/*ÿFAT������*/
	dword clu0;	/*0��������*/
	dword clus;	/*������*/
	dword rtclu;/*��Ŀ¼�غ�*/
	dword fstec;/*��һ���մغ�*/
	dword rescc;/*ʣ�������*/
}FAT32;	/*�ڴ��е��ļ�ϵͳ��Ϣ�ṹ*/
typedef struct
{	dword RRaA;
	byte res0[480];
	dword rrAa;
	dword frecou;
	dword nxtfre;
	byte res1[12];
	dword end;
}FSIfat32;	/*�ļ���Ϣ����*/
typedef struct
{	byte name[11];	/*�ļ���*/
	byte attr;	/*����
		00000001(ֻ��r)
		00000010(����h)
		00000100(ϵͳs)
		00001000(���l)
		00010000(��Ŀ¼d)
		00100000(�鵵)*/
	byte reserved;	/*����*/
	byte crtmils;	/*����ʱ��10����λ*/
	word crttime;	/*����ʱ��*/
	word crtdate;	/*��������*/
	word acsdate;	/*��������*/
	word idxh;		/*�״ظ�16λ*/
	word chgtime;	/*�޸�ʱ��*/
	word chgdate;	/*�޸�����*/
	word idxl;		/*�״ص�16λ*/
	dword len;		/*����*/
}DIRfat32;	/*FAT32Ŀ¼��ṹ*/
	/*��д�����ĺ�*/
#define rwpar(p,f,c,buf,rw) rwbuf(PART[p].hdid,PART[p].fst+(f),(c),(buf),(rw))
	/*��д�صĺ�(������,�غ�,����,������,��д)*/
#define rwcluFAT32(p,f,c,buf,rw) rwpar((p),((FAT32 *)PART[p].fs)->clu0+((FAT32 *)PART[p].fs)->spc*(f),((FAT32 *)PART[p].fs)->spc*(c),(buf),(rw))

/****�ļ�ϵͳ�������ܺ���****/
/*��ʼ��fat32����,����1�ɹ�0*/
dword initFAT32(dword p)
{	dword i;
	FAT32 *fs;
	byte *buf;
	BPBfat32 *bpb;

	if((fs=(FAT32 *)kmalloc(sizeof(FAT32)))==NULL) return 1;
	if((buf=(byte *)kmalloc(512))==NULL) {kfree(fs,sizeof(FAT32));return 1;}
	rwpar(p,0,1,buf,0);	/*��������¼*/
	if(*((word *)(buf+510))!=0xaa55) {kfree(buf,512);kfree(fs,sizeof(FAT32));return 1;}
	bpb=(BPBfat32 *)(buf+11);
	PART[p].cou=bpb->totsec;	/*����������*/
	PART[p].fs=fs;
	fs->spc=bpb->spc;
	fs->fats=bpb->fats;
	fs->fs_l=0;
	fs->ressec=bpb->ressec;
	fs->spfat=bpb->spfat;
	fs->clu0=bpb->ressec+(bpb->spfat*bpb->fats)-(bpb->spc<<1);
	fs->clus=(PART[p].cou-fs->clu0)/bpb->spc;
	fs->rtclu=bpb->rtclu;
	fs->fsinfo=bpb->fsinfo;
	rwpar(p,fs->fsinfo,1,buf,0);
	fs->rescc=((FSIfat32 *)buf)->frecou;
	fs->fstec=((FSIfat32 *)buf)->nxtfre;
	if(fs->fstec<=2)	/*�׿մغŴ���*/
	{	rwpar(p,fs->ressec,1,buf,0);	/*��1��FAT��*/
		for(i=2;;i++)	/*���ҿմ�*/
		{	if((i&0x7f)==0) rwpar(p,fs->ressec+(i>>7),1,buf,0);
			if(((dword *)buf)[i&0x7f]==0)	/*(i%128)�ǿմ�*/
			{	fs->fstec=i;	/*ȡ���׿մغ�*/
				break;
			}
		}
	}
	kfree(buf,512);
	return 0;
}
/*ж���ļ�ϵͳ*/
void unmntFAT32(dword p)
{	FSIfat32 *buf=(FSIfat32 *)kmalloc(512);
	rwpar(p,((FAT32 *)PART[p].fs)->fsinfo,1,(byte *)buf,0);
	buf->frecou=((FAT32 *)PART[p].fs)->rescc;
	buf->nxtfre=((FAT32 *)PART[p].fs)->fstec;
	rwpar(p,((FAT32 *)PART[p].fs)->fsinfo,1,(byte *)buf,1);
	kfree(buf,512);
	kfree(PART[p].fs,sizeof(FAT32));
}
/*�����,cou�Ƿ������,���ط���ĵ�һ�غ�,����-1��ʾ����*/
dword getcluFAT32(dword p,dword cou)
{	dword i,d,*bufd,*buf,*swp;/*ѭ��,��غ�,��黺��,��ǰ����,����ָ��*/

	d=i=((FAT32 *)PART[p].fs)->fstec;
	if((bufd=buf=(dword *)kmalloc(512))==NULL) return -1;
	if((swp=(dword *)kmalloc(512))==NULL) {kfree(buf,512);return -1;}
	((FAT32 *)PART[p].fs)->rescc-=cou;
	cou--;
	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(i>>7),1,(byte *)buf,0);
	for(i++;;i++)
	{	if((i&0x7f)==0 && (i>>7)!=(d>>7))	/*������FAT����*/
		{	buf=swp;	/*�����»���*/
			rwpar(p,((FAT32 *)PART[p].fs)->ressec+(i>>7),1,(byte *)buf,0);
		}
		if(buf[i&0x7f]==0)	/*�ҵ��մ�*/
		{	if(cou)	/*����û�����*/
			{	bufd[d&0x7f]=i;
				if((i>>7)!=(d>>7))	/*��ǰ����鲻ͬ*/
				{	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(d>>7),1,(byte *)bufd,1);
					rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(d>>7),1,(byte *)bufd,1);
					swp=bufd;	/*��������*/
					bufd=buf;	/*�ϲ�����뵱ǰ*/
				}
				d=i; cou--;
			}
			else	/*ȫ��������*/
			{	bufd[d&0x7f]=0x0fffffff;
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+(d>>7),1,(byte *)bufd,1);
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(d>>7),1,(byte *)bufd,1);
				kfree(bufd,512); kfree(swp,512);
				d=((FAT32 *)PART[p].fs)->fstec;
				((FAT32 *)PART[p].fs)->fstec=i;	/*���������׿մ�*/
				return d;
			}
		}
	}
}
/*���մ�,clu�����Ժ�Ĵر��ͷ�,����1�ɹ�0*/
dword putcluFAT32(dword p,dword clu)
{	dword *buf;

	if((buf=(dword *)kmalloc(512))==NULL) return 1;
	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(clu>>7),1,(byte *)buf,0);
	for(;;)
	{	dword i;
		if(clu<((FAT32 *)PART[p].fs)->fstec) ((FAT32 *)PART[p].fs)->fstec=clu;
		i=buf[clu&0x7f];
		buf[clu&0x7f]=0;
		((FAT32 *)PART[p].fs)->rescc++;
		if(i==0x0fffffff) break;
		if((i>>7)!=(clu>>7))
		{	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(clu>>7),1,(byte *)buf,1);
			rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(clu>>7),1,(byte *)buf,1);
			rwpar(p,((FAT32 *)PART[p].fs)->ressec+(i>>7),1,(byte *)buf,0);
		}
		clu=i;
	}
	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(clu>>7),1,(byte *)buf,1);
	rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(clu>>7),1,(byte *)buf,1);
	kfree(buf,512);
	return 0;
}
/*ȡ��Ŀ¼��ʱ��,op:0ȡ�ô���ʱ��1ȡ���޸�ʱ��,����1�ɹ�0*/
dword getimFAT32(DIRfat32 *dir,TM *tm,dword op)
{	if(op==0)
	{	tm->yer=(dir->crtdate>>9)+1980;
		tm->mon=(dir->crtdate>>5)&0x0f;
		tm->day=dir->crtdate&0x1f;
		tm->hor=dir->crttime>>11;
		tm->min=(dir->crttime>>5)&0x1f;
		tm->sec=(dir->crttime<<1)&0x1f;
		tm->mil=0;
	}
	else
	{	tm->yer=(dir->chgdate>>9)+1980;
		tm->mon=(dir->chgdate>>5)&0x0f;
		tm->day=dir->chgdate&0x1f;
		tm->hor=dir->chgtime>>11;
		tm->min=(dir->chgtime>>5)&0x1f;
		tm->sec=(dir->chgtime<<1)&0x1f;
		tm->mil=0;
	}
	if(tm->mon==0 || tm->mon>12) return 1;
	if(tm->day==0 || tm->day>31) return 1;
	if(tm->hor>23 || tm->min>59 || tm->sec>59) return 1;
	return 0;
}
/*����Ŀ¼��ʱ��,op:0���ô���ʱ��1�����޸�ʱ��2������,����1�ɹ�0*/
dword setimFAT32(DIRfat32 *dir,TM *tm,dword op)
{	if(tm->yer<1980) return 1;
	if(tm->mon==0 || tm->mon>12) return 1;
	if(tm->day==0 || tm->day>31) return 1;
	if(tm->hor>23 || tm->min>59 || tm->sec>59) return 1;
	if(op==0)
	{	dir->crtdate=((tm->yer-1980)<<9)+(tm->mon<<5)+tm->day;	/*ʱ���ʽ��FAT32��ͬ*/
		dir->crttime=(tm->hor<<11)+(tm->min<<5)+(tm->sec>>1);
	}
	else
	{	dir->chgdate=((tm->yer-1980)<<9)+(tm->mon<<5)+tm->day;
		dir->chgtime=(tm->hor<<11)+(tm->min<<5)+(tm->sec>>1);
		if(op==2)
		{	dir->crtdate=dir->chgdate;
			dir->crttime=dir->chgtime;
		}
	}
	return 0;
}
/*��д�ļ�,���ݲ��ó����ļ�β,����1�ɹ�0*/
/*����,Ŀ¼��,��ǰ�غ�,���ֽ�,�ֽ���,������,д��Ϊ1*/
dword rwfileFAT32(dword p,DIRfat32 *dir,dword *curc,dword fst,dword len,byte *buf,dword rw)
{	dword j,f,n,rd,*pi=NULL;/*�غż���,��һ�غ�,���ֽ���,��дָ��,FAT��ָ��*/
	byte *pd;/*���ݴ�ָ��*/

	if(!dir) return 0;
	n=((FAT32 *)PART[p].fs)->spc<<9;
	if((pd=(byte *)kmalloc(n))==NULL) return 1;
	if(curc && (*curc)) {j=*curc;rd=fst-fst%n;}
	else {j=((dword)(dir->idxh)<<16) | dir->idxl;rd=0;}

	for(f=j+128;;)
	{	if(rd>=fst)	/*��ͷ�Ժ�*/
		{	if(rd+n>fst+len)	/*ĩβ*/
			{	if((fst+len)%n)	/*����Ҫ����)*/
				{	rwcluFAT32(p,j,1,pd,0);
					if(rw)
					{	memcpy(pd,buf,(fst+len)%n);
						rwcluFAT32(p,j,1,pd,1);
					}
					else
						memcpy(buf,pd,(fst+len)%n);
				}
			}
			else	/*�м�*/
			{	rwcluFAT32(p,j,1,buf,rw);
				buf+=n;
			}
		}
		else if(rd+n>fst)	/*��Ҫ��ͷ*/
		{	rwcluFAT32(p,j,1,pd,0);
			if(rd+n>fst+len)	/*��ͷ��ĩβ*/
			{	if(rw)
				{	memcpy(pd+fst%n,buf,(fst+len)%n-fst%n);
					rwcluFAT32(p,j,1,pd,1);
				}
				else
					memcpy(buf,pd+fst%n,(fst+len)%n-fst%n);
			}
			else	/*��ͷ*/
			{	if(rw)
				{	memcpy(pd+fst%n,buf,n-fst%n);
					rwcluFAT32(p,j,1,pd,1);
				}
				else
					memcpy(buf,pd+fst%n,n-fst%n);
				buf+=(n-fst%n);
			}
		}
		rd+=n;
		if(rd>fst+len)	/*���*/
		{	if(curc) (*curc)=j;
			if(pi)kfree(pi,512);kfree(pd,n);return 0;
		}
		if(pi==NULL && (pi=(dword *)kmalloc(512))==NULL) {kfree(pd,n);return 1;}
		if((j>>7)!=(f>>7))	/*��һ�غŲ��ڻ�����*/
			rwpar(p,((FAT32 *)PART[p].fs)->ressec+(j>>7),1,(byte *)pi,0);
		f=j;
		j=pi[j&0x7f];
		if(j==0x0fffffff)	/*�ļ�����*/
		{	if(curc) (*curc)=0;
			kfree(pi,512);kfree(pd,n);return 0;
		}
	}
}
/*�����ļ�����,����1�ɹ�0*/
/*����,Ŀ¼��,����*/
dword setlenFAT32(dword p,DIRfat32 *dir,dword len)
{	dword j,f,n,tl,rd,*pi=NULL;/*�غż���,��һ�غ�,���ֽ���,��ʱ����,��дָ��,FAT��ָ��*/

	n=((FAT32 *)PART[p].fs)->spc<<9;
	tl=dir->len;
	if(len==tl) return 0;
	if((len+n-1)/n==(tl+n-1)/n) {dir->len=len;return 0;}	/*����Ҫ�޸Ĵ�*/
	WAIT(((FAT32 *)PART[p].fs)->fs_l);
	if(len>tl&&(len+n-1)/n-(tl+n-1)/n>((FAT32 *)PART[p].fs)->rescc)
		{SIGN(((FAT32 *)PART[p].fs)->fs_l);return 1;}	/*����ʣ��ش�С*/
	if((pi=(dword *)kmalloc(512))==NULL)
		{SIGN(((FAT32 *)PART[p].fs)->fs_l);return 1;}
	j=((dword)(dir->idxh)<<16) | dir->idxl;
	f=j+128;
	rd=0;

	if(len<tl)	/*��С�ļ�*/
	{	for(;;)
		{	if(rd>=len)	/*�Ӵ˿�ʼ����*/
			{	if(putcluFAT32(p,j)==1)
				{	SIGN(((FAT32 *)PART[p].fs)->fs_l);
					kfree(pi,512);return 1;
				}
				if(len)	/*��ʣ���*/
				{	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(f>>7),1,(byte *)pi,0);
					pi[f&0x7f]=0x0fffffff;
					rwpar(p,((FAT32 *)PART[p].fs)->ressec+(f>>7),1,(byte *)pi,1);
					rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(f>>7),1,(byte *)pi,1);
				}
				else
					dir->idxh=dir->idxl=0;
				SIGN(((FAT32 *)PART[p].fs)->fs_l);
				kfree(pi,512);dir->len=len;return 0;
			}
			rd+=n;
			if((j>>7)!=(f>>7))	/*��һ�غŲ��ڻ�����*/
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+(j>>7),1,(byte *)pi,0);
			f=j;
			j=pi[j&0x7f];
		}
	}
	else	/*�����ļ�*/
	{	for(;;)
		{	if(tl==0 || j==0x0fffffff)
			{	dword clu=getcluFAT32(p,(len+n-1-rd)/n);
				if(clu==0xffffffff)
				{	SIGN(((FAT32 *)PART[p].fs)->fs_l);
					kfree(pi,512);return 1;
				}
				if(tl)	/*��ʣ���*/
				{	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(f>>7),1,(byte *)pi,0);
					pi[f&0x7f]=clu;
					rwpar(p,((FAT32 *)PART[p].fs)->ressec+(f>>7),1,(byte *)pi,1);
					rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(f>>7),1,(byte *)pi,1);
				}
				else
				{	dir->idxh=clu>>16;
					dir->idxl=clu;
				}
				SIGN(((FAT32 *)PART[p].fs)->fs_l);
				kfree(pi,512);dir->len=len;return 0;
			}
			rd+=n;
			if((j>>7)!=(f>>7))	/*��һ�غŲ��ڻ�����*/
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+(j>>7),1,(byte *)pi,0);
			f=j;
			j=pi[j&0x7f];
		}
	}
}
/*����Ŀ¼���Ƿ�ƥ��,isdir:0�ļ�1Ŀ¼2����*/
dword namatchFAT32(byte *path,dword isdir,DIRfat32 *dir)
{	byte name[13],*s,*t=path;
	dword i;
	if(dir->attr==0xf) return 0;	/*����֧�ֳ��ļ���*/
	if(isdir<2 && isdir!=((dir->attr&0x10)>>4)) return 0;	/*���Բ�ƥ��*/
	for(s=name,i=0;i<11;i++)	/*ת���ļ�����ʽ*/
	{	if(i==8) *(s++)='.';
		*s=dir->name[i];
		if(*s!=' ') s++;
	}
	s--;
	if(*s!='.') s++;
	*s=0;
	if(name[0]==0x05) name[0]=0xe5;	/*����0x05�ַ�*/
	for(s=name;*s || (*path && *path!='/');s++,path++)
		if(*s!=((*path>='a' && *path<='z')?*path-32:*path)) return 0;	/*�ļ�����ƥ��*/
	return 1;
}
/*������ȡ��Ŀ¼�е�Ŀ¼��,�������,�����ڷ���-1,����-2*/
/*����,������Ŀ¼,������·��,�Ƿ�Ŀ¼,Ŀ¼���*/
dword schdirFAT32(dword p,DIRfat32 *dir,byte *path,dword isdir,DIRfat32 *buf)
{	if(dir==NULL)	/*ȡ�ø�Ŀ¼��*/
	{	memset(buf,0,32);	/*����һ����Ŀ¼��*/
		buf->idxh=((FAT32 *)PART[p].fs)->rtclu>>16;
		buf->idxl=((FAT32 *)PART[p].fs)->rtclu;
		buf->attr=0x10;	/*����:Ŀ¼*/
		return 0;
	}
	else	/*ȡ������Ŀ¼��*/
	{	dword i,c;	/*ѭ��,��ʱ�غ�*/
		for(i=c=0;;i+=32)
		{	if(rwfileFAT32(p,dir,&c,i,32,(byte*)buf,0)) return -2;
			if(buf->name[0]==0) return -1;
			if(namatchFAT32(path,isdir,buf)) return i>>5;
		}
	}
}
/*���������ȷ��*/
dword errnamFAT32(byte *name)
{	dword i,j;
	static byte errchar[]={	/*�����ַ�*/
		0x22,0x2A,0x2B,0x2C,0x2E,0x2F,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x5B,0x5C,0x5D,0x7C
	};
	if(*name=='.') return 1;
	for(i=0;*name && *name!='.';i++,name++)	/*�����зǷ��ַ��򳬳�*/
	{	if(*name<=0x20 || i>=8) return 1;
		for(j=0;j<16;j++)
			if(*name==errchar[j]) return 1;
	}
	if(*name=='.') name++; else return 0;
	for(i=0;*name;i++,name++)	/*��չ���зǷ��ַ��򳬳�*/
	{	if(*name<=0x20 || i>=3) return 1;
		for(j=0;j<16;j++)
			if(*name==errchar[j]) return 1;
	}
	return 0;
}
/*����Ŀ¼������*/
void setnamFAT32(DIRfat32 *dir,byte *name)
{	dword i;
	if(*name==0xe5) {dir->name[0]=0x05;i=1;} else i=0;
	for(;i<11;i++)
	{	if(i==8 && *name=='.') name++;
		if(*name && *name!='.')
		{	dir->name[i]=((*name>='a' && *name<='z')?*name-32:*name);
			name++;
		}
		else
			dir->name[i]=' ';
	}
}
/*����.��..,����1�ɹ�0*/
dword dirdotFAT32(dword p,DIRfat32 *dir1,DIRfat32 *dir2)
{	DIRfat32 *buf;
	dword n=((FAT32 *)PART[p].fs)->spc<<9;
	if(setlenFAT32(p,dir2,n)) return 1;
	dir2->len=0;
	if((buf=(DIRfat32 *)kmalloc(n))==NULL) return 1;
	memcpy(buf,dir2,32); memset(buf,' ',11); buf[0].name[0]='.';
	memcpy(buf+1,dir1,32); memset(buf+1,' ',11); buf[1].name[1]=buf[1].name[0]='.';
	if(dir1->name[0]==0) buf[1].idxh=buf[1].idxl=0;
	memset(buf+2,0,n-64);
	if(rwfileFAT32(p,dir2,0,0,n,(byte*)buf,1)) {kfree(buf,n);return 1;}
	kfree(buf,n);
	return 0;
}
/*��dir�д���Ŀ¼��buf,�������,����-1*/
dword newdirFAT32(dword p,DIRfat32 *dir,byte *name,dword isdir,DIRfat32 *buf)
{	dword i,c;	/*ѭ��,��ʱ�غ�*/
	TM tm;	/*����ʱ��*/

	if(errnamFAT32(name)) return -1;
	/*������λ*/
	for(i=c=0;i<0x10000*32;i+=32)	/*�������2^16��Ŀ¼��*/
	{	if(rwfileFAT32(p,dir,&c,i,32,(byte*)buf,0)) return -1;
		switch(buf->name[0])
		{
		case 0:
			dir->len=i+32;
			if(setlenFAT32(p,dir,i+64)) {dir->len=0;return -1;}
			if(rwfileFAT32(p,dir,&c,i+32,32,(byte*)buf,1)) {dir->len=0;return -1;}
			dir->len=0;
		case 0xe5:
			memset(buf,0,32);
			setnamFAT32(buf,name);
			buf->attr=(isdir<<4);
			sys_curtime(&tm);
			setimFAT32(buf,&tm,2);
			if(isdir) dirdotFAT32(p,dir,buf);
			if(rwfileFAT32(p,dir,0,i,32,(byte*)buf,1)) return -1;
			return i>>5;
		}
	}
	return -1;
}
/*Ŀ¼��������,����1�ɹ�0*/
dword rendirFAT32(DIRfat32 *dir,byte *name)
{	if(dir->name[0]=='.') return 1;	/*"."��".."���ܱ�����*/
	if(errnamFAT32(name)) return 1;
	setnamFAT32(dir,name);
	return 0;
}
/*��dir��ɾ��Ŀ¼��id,����1�ɹ�0*/
dword deldirFAT32(dword p,DIRfat32 *dir,dword id)
{	DIRfat32 *buf;
	if((buf=(DIRfat32 *)kmalloc(32))==NULL) return 1;
	if(rwfileFAT32(p,dir,0,(id<<5)+32,32,(byte*)buf,0)) {kfree(buf,32);return 1;}
	if(buf->name[0]==0)	/*�������һ��*/
	{	if(id==0)	/*��Ŀ¼���������*/
		{	buf->name[0]=0;
			if(rwfileFAT32(p,dir,0,0,32,(byte*)buf,1)) {kfree(buf,32);return 1;}
		}
		else	/*������һ��Ŀ¼��,һ��ɾ��*/
		{	dir->len=(id<<5)+64;
			do
				if(rwfileFAT32(p,dir,0,((--id)<<5),32,(byte*)buf,0)) {dir->len=0;kfree(buf,32);return 1;}
			while(buf->name[0]==0xe5);
			buf->name[0]=0;
			if(rwfileFAT32(p,dir,0,(id<<5)+32,32,(byte*)buf,1)) {dir->len=0;kfree(buf,32);return 1;}
			if(setlenFAT32(p,dir,(id<<5)+64)) {dir->len=0;kfree(buf,32);return 1;}
			dir->len=0;
		}
	}
	else	/*ֱ�ӱ��Ϊɾ��*/
	{	buf->name[0]=0xe5;
		if(rwfileFAT32(p,dir,0,id<<5,32,(byte*)buf,1)) {kfree(buf,32);return 1;}
	}
	kfree(buf,32);
	return 0;
}
/*�ж�Ŀ¼�Ƿ��*/
dword empdirFAT32(dword p,DIRfat32 *dir)
{	dword i,c;
	DIRfat32 *buf;

	if(dir->name[0]=='.') return 0;
	if((buf=(DIRfat32 *)kmalloc(32))==NULL) return 0;
	for(i=64,c=0;;i+=32)
	{	if(rwfileFAT32(p,dir,&c,i,32,(byte*)buf,0)) {kfree(buf,32);return 0;}
		if(buf->name[0]==0) {kfree(buf,32);dir->len=(i<<5)+32;return 1;}
		if(buf->name[0]!=0xe5) {kfree(buf,32);return 0;}
	}
}
/*�����ļ�����*/
dword filenFAT32(DIRfat32 *dir)
{	return dir->len;
}
/*����Ŀ¼�������,����1�ɹ�0*/
dword setatrFAT32(DIRfat32 *dir,dword attr)
{	if((attr&0xf)==0xf) return 1;	/*�������ó��ļ�������*/
	if(dir->attr&0x10) attr|=0x10; else attr&=0xef;
	dir->attr=(attr&0x3f);
	return 0;
}
/*ȡ��Ŀ¼�������*/
dword attrFAT32(DIRfat32 *dir)
{	return (dir->attr)&0x7f;	/*�����ܿ��޸�*/
}
/*ȡ��Ŀ¼���е�ʵ����,�ɹ�0����1����-1*/
dword realnamFAT32(dword p,DIRfat32 *dir,dword *off,dword *curc,byte *name,dword *isd)
{	dword i;
	byte *s;
	DIRfat32 *buf;

	if((buf=(DIRfat32 *)kmalloc(32))==NULL) return -1;
	do	/*ȡ����ЧĿ¼��*/
	{	if(rwfileFAT32(p,dir,curc,*off,32,(byte*)buf,0)) {kfree(buf,32);return -1;}
		if(buf->name[0]==0 || (*curc)==0) {(*off)=(*curc)=0;kfree(buf,32);return 1;}
		(*off)+=32;
	}while(buf->name[0]=='.' || buf->name[0]==0xe5 || (buf->attr&0x0e));
	memset(name,0,13);
	for(s=name,i=0;i<11;i++)	/*ת���ļ�����ʽ*/
	{	if(i==8) *(s++)='.';
		*s=buf->name[i];
		if(*s!=' ') s++;
	}
	s--;
	if(*s!='.') s++;
	*s=0;
	if(*name==0x05) *name=0xe5;	/*����0x05�ַ�*/
	if(buf->attr&0x10) *isd=1; else *isd=0;
	kfree(buf,32);
	return 0;
}

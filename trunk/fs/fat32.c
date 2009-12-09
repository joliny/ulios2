/*
作者：孙亮
功能：fat32文件系统简单支持
*/
#include"..\ulidef.h"

typedef struct
{	word bps;	/*每扇区字节数512*/
	byte spc;	/***每簇扇区数*/
	word ressec;/***保留扇区数(第一个FAT开始之前的扇区数)*/
	byte fats;	/***FAT数一般为2*/
	word rtents;/*根目录项数(FAT32为0)*/
	word smlsec;/*小扇区数(FAT32为0)*/
	byte media;	/*媒体描述符硬盘0xF8*/
	word spf;	/*每FAT扇区数(FAT32为0)*/
	word spt;	/*每道扇区数*/
	word heads;	/*磁头数*/
	dword relsec;/*盘上引导扇区以前所有扇区数*/
	dword totsec;/***总扇区数*/
	dword spfat;/***每FAT扇区数FAT32使用*/
	word exflg;	/*扩展标志*/
	word fsver;	/*文件系统版本*/
	dword rtclu;/***根目录簇号*/
	word fsinfo;/***文件系统信息扇区号一般为1*/
	word bkbot;	/*备份引导扇区6*/
	byte reser[12];	/*保留12字节*/
	/*以下为扩展BPB*/
	byte pdn;	/*物理驱动器号,第一个驱动器为0x80*/
	byte exres;	/*保留*/
	byte exbtsg;/*扩展引导标签为0x29*/
	dword volume;/*分区序号,用于区分磁盘*/
	byte vollab[11];/*卷标*/
	byte fsid[8];/*系统ID*/
}__attribute__((packed)) BPBfat32;	/*FAT32的BPB*/
typedef struct
{	byte spc;	/*每簇扇区数*/
	byte fats;	/*fat数目*/
	byte fs_l;	/*信息结构操作信号量*/
	word ressec;/*保留扇区数(FAT之前的扇区)*/
	word fsinfo;/*文件系统信息扇区号一般为1*/
	dword spfat;/*每FAT扇区数*/
	dword clu0;	/*0簇扇区号*/
	dword clus;	/*簇数量*/
	dword rtclu;/*根目录簇号*/
	dword fstec;/*第一个空簇号*/
	dword rescc;/*剩余簇数量*/
}FAT32;	/*内存中的文件系统信息结构*/
typedef struct
{	dword RRaA;
	byte res0[480];
	dword rrAa;
	dword frecou;
	dword nxtfre;
	byte res1[12];
	dword end;
}FSIfat32;	/*文件信息扇区*/
typedef struct
{	byte name[11];	/*文件名*/
	byte attr;	/*属性
		00000001(只读r)
		00000010(隐藏h)
		00000100(系统s)
		00001000(卷标l)
		00010000(子目录d)
		00100000(归档)*/
	byte reserved;	/*保留*/
	byte crtmils;	/*创建时间10毫秒位*/
	word crttime;	/*创建时间*/
	word crtdate;	/*创建日期*/
	word acsdate;	/*访问日期*/
	word idxh;		/*首簇高16位*/
	word chgtime;	/*修改时间*/
	word chgdate;	/*修改日期*/
	word idxl;		/*首簇低16位*/
	dword len;		/*长度*/
}DIRfat32;	/*FAT32目录项结构*/
	/*读写分区的宏*/
#define rwpar(p,f,c,buf,rw) rwbuf(PART[p].hdid,PART[p].fst+(f),(c),(buf),(rw))
	/*读写簇的宏(分区号,簇号,个数,缓冲区,读写)*/
#define rwcluFAT32(p,f,c,buf,rw) rwpar((p),((FAT32 *)PART[p].fs)->clu0+((FAT32 *)PART[p].fs)->spc*(f),((FAT32 *)PART[p].fs)->spc*(c),(buf),(rw))

/****文件系统基础功能函数****/
/*初始化fat32分区,出错1成功0*/
dword initFAT32(dword p)
{	dword i;
	FAT32 *fs;
	byte *buf;
	BPBfat32 *bpb;

	if((fs=(FAT32 *)kmalloc(sizeof(FAT32)))==NULL) return 1;
	if((buf=(byte *)kmalloc(512))==NULL) {kfree(fs,sizeof(FAT32));return 1;}
	rwpar(p,0,1,buf,0);	/*读引导记录*/
	if(*((word *)(buf+510))!=0xaa55) {kfree(buf,512);kfree(fs,sizeof(FAT32));return 1;}
	bpb=(BPBfat32 *)(buf+11);
	PART[p].cou=bpb->totsec;	/*更新扇区数*/
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
	if(fs->fstec<=2)	/*首空簇号错误*/
	{	rwpar(p,fs->ressec,1,buf,0);	/*第1个FAT簇*/
		for(i=2;;i++)	/*查找空簇*/
		{	if((i&0x7f)==0) rwpar(p,fs->ressec+(i>>7),1,buf,0);
			if(((dword *)buf)[i&0x7f]==0)	/*(i%128)是空簇*/
			{	fs->fstec=i;	/*取得首空簇号*/
				break;
			}
		}
	}
	kfree(buf,512);
	return 0;
}
/*卸载文件系统*/
void unmntFAT32(dword p)
{	FSIfat32 *buf=(FSIfat32 *)kmalloc(512);
	rwpar(p,((FAT32 *)PART[p].fs)->fsinfo,1,(byte *)buf,0);
	buf->frecou=((FAT32 *)PART[p].fs)->rescc;
	buf->nxtfre=((FAT32 *)PART[p].fs)->fstec;
	rwpar(p,((FAT32 *)PART[p].fs)->fsinfo,1,(byte *)buf,1);
	kfree(buf,512);
	kfree(PART[p].fs,sizeof(FAT32));
}
/*分配簇,cou是分配个数,返回分配的第一簇号,返回-1表示出错*/
dword getcluFAT32(dword p,dword cou)
{	dword i,d,*bufd,*buf,*swp;/*循环,脏簇号,脏块缓冲,当前缓冲,交换指针*/

	d=i=((FAT32 *)PART[p].fs)->fstec;
	if((bufd=buf=(dword *)kmalloc(512))==NULL) return -1;
	if((swp=(dword *)kmalloc(512))==NULL) {kfree(buf,512);return -1;}
	((FAT32 *)PART[p].fs)->rescc-=cou;
	cou--;
	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(i>>7),1,(byte *)buf,0);
	for(i++;;i++)
	{	if((i&0x7f)==0 && (i>>7)!=(d>>7))	/*到达新FAT扇区*/
		{	buf=swp;	/*设置新缓冲*/
			rwpar(p,((FAT32 *)PART[p].fs)->ressec+(i>>7),1,(byte *)buf,0);
		}
		if(buf[i&0x7f]==0)	/*找到空簇*/
		{	if(cou)	/*分配没有完成*/
			{	bufd[d&0x7f]=i;
				if((i>>7)!=(d>>7))	/*当前与脏块不同*/
				{	rwpar(p,((FAT32 *)PART[p].fs)->ressec+(d>>7),1,(byte *)bufd,1);
					rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(d>>7),1,(byte *)bufd,1);
					swp=bufd;	/*交换缓冲*/
					bufd=buf;	/*合并脏块与当前*/
				}
				d=i; cou--;
			}
			else	/*全部分配完*/
			{	bufd[d&0x7f]=0x0fffffff;
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+(d>>7),1,(byte *)bufd,1);
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+((FAT32 *)PART[p].fs)->spfat+(d>>7),1,(byte *)bufd,1);
				kfree(bufd,512); kfree(swp,512);
				d=((FAT32 *)PART[p].fs)->fstec;
				((FAT32 *)PART[p].fs)->fstec=i;	/*重新设置首空簇*/
				return d;
			}
		}
	}
}
/*回收簇,clu及其以后的簇被释放,出错1成功0*/
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
/*取得目录项时间,op:0取得创建时间1取得修改时间,出错1成功0*/
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
/*设置目录项时间,op:0设置创建时间1设置修改时间2都设置,出错1成功0*/
dword setimFAT32(DIRfat32 *dir,TM *tm,dword op)
{	if(tm->yer<1980) return 1;
	if(tm->mon==0 || tm->mon>12) return 1;
	if(tm->day==0 || tm->day>31) return 1;
	if(tm->hor>23 || tm->min>59 || tm->sec>59) return 1;
	if(op==0)
	{	dir->crtdate=((tm->yer-1980)<<9)+(tm->mon<<5)+tm->day;	/*时间格式与FAT32相同*/
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
/*读写文件,数据不得超出文件尾,出错1成功0*/
/*分区,目录项,当前簇号,首字节,字节数,缓冲区,写则为1*/
dword rwfileFAT32(dword p,DIRfat32 *dir,dword *curc,dword fst,dword len,byte *buf,dword rw)
{	dword j,f,n,rd,*pi=NULL;/*簇号计数,上一簇号,簇字节数,读写指针,FAT簇指针*/
	byte *pd;/*数据簇指针*/

	if(!dir) return 0;
	n=((FAT32 *)PART[p].fs)->spc<<9;
	if((pd=(byte *)kmalloc(n))==NULL) return 1;
	if(curc && (*curc)) {j=*curc;rd=fst-fst%n;}
	else {j=((dword)(dir->idxh)<<16) | dir->idxl;rd=0;}

	for(f=j+128;;)
	{	if(rd>=fst)	/*开头以后*/
		{	if(rd+n>fst+len)	/*末尾*/
			{	if((fst+len)%n)	/*还需要操作)*/
				{	rwcluFAT32(p,j,1,pd,0);
					if(rw)
					{	memcpy(pd,buf,(fst+len)%n);
						rwcluFAT32(p,j,1,pd,1);
					}
					else
						memcpy(buf,pd,(fst+len)%n);
				}
			}
			else	/*中间*/
			{	rwcluFAT32(p,j,1,buf,rw);
				buf+=n;
			}
		}
		else if(rd+n>fst)	/*将要开头*/
		{	rwcluFAT32(p,j,1,pd,0);
			if(rd+n>fst+len)	/*开头即末尾*/
			{	if(rw)
				{	memcpy(pd+fst%n,buf,(fst+len)%n-fst%n);
					rwcluFAT32(p,j,1,pd,1);
				}
				else
					memcpy(buf,pd+fst%n,(fst+len)%n-fst%n);
			}
			else	/*开头*/
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
		if(rd>fst+len)	/*完成*/
		{	if(curc) (*curc)=j;
			if(pi)kfree(pi,512);kfree(pd,n);return 0;
		}
		if(pi==NULL && (pi=(dword *)kmalloc(512))==NULL) {kfree(pd,n);return 1;}
		if((j>>7)!=(f>>7))	/*下一簇号不在缓冲中*/
			rwpar(p,((FAT32 *)PART[p].fs)->ressec+(j>>7),1,(byte *)pi,0);
		f=j;
		j=pi[j&0x7f];
		if(j==0x0fffffff)	/*文件结束*/
		{	if(curc) (*curc)=0;
			kfree(pi,512);kfree(pd,n);return 0;
		}
	}
}
/*设置文件长度,出错1成功0*/
/*分区,目录项,长度*/
dword setlenFAT32(dword p,DIRfat32 *dir,dword len)
{	dword j,f,n,tl,rd,*pi=NULL;/*簇号计数,上一簇号,簇字节数,临时长度,读写指针,FAT簇指针*/

	n=((FAT32 *)PART[p].fs)->spc<<9;
	tl=dir->len;
	if(len==tl) return 0;
	if((len+n-1)/n==(tl+n-1)/n) {dir->len=len;return 0;}	/*不需要修改簇*/
	WAIT(((FAT32 *)PART[p].fs)->fs_l);
	if(len>tl&&(len+n-1)/n-(tl+n-1)/n>((FAT32 *)PART[p].fs)->rescc)
		{SIGN(((FAT32 *)PART[p].fs)->fs_l);return 1;}	/*超出剩余簇大小*/
	if((pi=(dword *)kmalloc(512))==NULL)
		{SIGN(((FAT32 *)PART[p].fs)->fs_l);return 1;}
	j=((dword)(dir->idxh)<<16) | dir->idxl;
	f=j+128;
	rd=0;

	if(len<tl)	/*减小文件*/
	{	for(;;)
		{	if(rd>=len)	/*从此开始回收*/
			{	if(putcluFAT32(p,j)==1)
				{	SIGN(((FAT32 *)PART[p].fs)->fs_l);
					kfree(pi,512);return 1;
				}
				if(len)	/*还剩余簇*/
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
			if((j>>7)!=(f>>7))	/*下一簇号不在缓冲中*/
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+(j>>7),1,(byte *)pi,0);
			f=j;
			j=pi[j&0x7f];
		}
	}
	else	/*增大文件*/
	{	for(;;)
		{	if(tl==0 || j==0x0fffffff)
			{	dword clu=getcluFAT32(p,(len+n-1-rd)/n);
				if(clu==0xffffffff)
				{	SIGN(((FAT32 *)PART[p].fs)->fs_l);
					kfree(pi,512);return 1;
				}
				if(tl)	/*还剩余簇*/
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
			if((j>>7)!=(f>>7))	/*下一簇号不在缓冲中*/
				rwpar(p,((FAT32 *)PART[p].fs)->ressec+(j>>7),1,(byte *)pi,0);
			f=j;
			j=pi[j&0x7f];
		}
	}
}
/*测试目录项是否匹配,isdir:0文件1目录2忽略*/
dword namatchFAT32(byte *path,dword isdir,DIRfat32 *dir)
{	byte name[13],*s,*t=path;
	dword i;
	if(dir->attr==0xf) return 0;	/*还不支持长文件名*/
	if(isdir<2 && isdir!=((dir->attr&0x10)>>4)) return 0;	/*属性不匹配*/
	for(s=name,i=0;i<11;i++)	/*转换文件名格式*/
	{	if(i==8) *(s++)='.';
		*s=dir->name[i];
		if(*s!=' ') s++;
	}
	s--;
	if(*s!='.') s++;
	*s=0;
	if(name[0]==0x05) name[0]=0xe5;	/*处理0x05字符*/
	for(s=name;*s || (*path && *path!='/');s++,path++)
		if(*s!=((*path>='a' && *path<='z')?*path-32:*path)) return 0;	/*文件名不匹配*/
	return 1;
}
/*搜索并取得目录中的目录项,返回项号,不存在返回-1,出错-2*/
/*分区,被搜索目录,被搜索路径,是否目录,目录项缓冲*/
dword schdirFAT32(dword p,DIRfat32 *dir,byte *path,dword isdir,DIRfat32 *buf)
{	if(dir==NULL)	/*取得根目录项*/
	{	memset(buf,0,32);	/*创建一个根目录项*/
		buf->idxh=((FAT32 *)PART[p].fs)->rtclu>>16;
		buf->idxl=((FAT32 *)PART[p].fs)->rtclu;
		buf->attr=0x10;	/*属性:目录*/
		return 0;
	}
	else	/*取得其他目录项*/
	{	dword i,c;	/*循环,临时簇号*/
		for(i=c=0;;i+=32)
		{	if(rwfileFAT32(p,dir,&c,i,32,(byte*)buf,0)) return -2;
			if(buf->name[0]==0) return -1;
			if(namatchFAT32(path,isdir,buf)) return i>>5;
		}
	}
}
/*检查名称正确性*/
dword errnamFAT32(byte *name)
{	dword i,j;
	static byte errchar[]={	/*错误字符*/
		0x22,0x2A,0x2B,0x2C,0x2E,0x2F,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,0x5B,0x5C,0x5D,0x7C
	};
	if(*name=='.') return 1;
	for(i=0;*name && *name!='.';i++,name++)	/*主名有非法字符或超长*/
	{	if(*name<=0x20 || i>=8) return 1;
		for(j=0;j<16;j++)
			if(*name==errchar[j]) return 1;
	}
	if(*name=='.') name++; else return 0;
	for(i=0;*name;i++,name++)	/*扩展名有非法字符或超长*/
	{	if(*name<=0x20 || i>=3) return 1;
		for(j=0;j<16;j++)
			if(*name==errchar[j]) return 1;
	}
	return 0;
}
/*设置目录项名称*/
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
/*创建.和..,出错1成功0*/
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
/*在dir中创建目录项buf,返回项号,出错-1*/
dword newdirFAT32(dword p,DIRfat32 *dir,byte *name,dword isdir,DIRfat32 *buf)
{	dword i,c;	/*循环,临时簇号*/
	TM tm;	/*创建时间*/

	if(errnamFAT32(name)) return -1;
	/*搜索空位*/
	for(i=c=0;i<0x10000*32;i+=32)	/*允许最多2^16个目录项*/
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
/*目录项重命名,出错1成功0*/
dword rendirFAT32(DIRfat32 *dir,byte *name)
{	if(dir->name[0]=='.') return 1;	/*"."和".."不能被改名*/
	if(errnamFAT32(name)) return 1;
	setnamFAT32(dir,name);
	return 0;
}
/*在dir中删除目录项id,出错1成功0*/
dword deldirFAT32(dword p,DIRfat32 *dir,dword id)
{	DIRfat32 *buf;
	if((buf=(DIRfat32 *)kmalloc(32))==NULL) return 1;
	if(rwfileFAT32(p,dir,0,(id<<5)+32,32,(byte*)buf,0)) {kfree(buf,32);return 1;}
	if(buf->name[0]==0)	/*已是最后一项*/
	{	if(id==0)	/*根目录的特殊情况*/
		{	buf->name[0]=0;
			if(rwfileFAT32(p,dir,0,0,32,(byte*)buf,1)) {kfree(buf,32);return 1;}
		}
		else	/*检查最后一个目录项,一并删除*/
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
	else	/*直接标记为删除*/
	{	buf->name[0]=0xe5;
		if(rwfileFAT32(p,dir,0,id<<5,32,(byte*)buf,1)) {kfree(buf,32);return 1;}
	}
	kfree(buf,32);
	return 0;
}
/*判断目录是否空*/
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
/*返回文件长度*/
dword filenFAT32(DIRfat32 *dir)
{	return dir->len;
}
/*设置目录项的属性,出错1成功0*/
dword setatrFAT32(DIRfat32 *dir,dword attr)
{	if((attr&0xf)==0xf) return 1;	/*尝试设置长文件名属性*/
	if(dir->attr&0x10) attr|=0x10; else attr&=0xef;
	dir->attr=(attr&0x3f);
	return 0;
}
/*取得目录项的属性*/
dword attrFAT32(DIRfat32 *dir)
{	return (dir->attr)&0x7f;	/*属性总可修改*/
}
/*取得目录项中的实名称,成功0结束1出错-1*/
dword realnamFAT32(dword p,DIRfat32 *dir,dword *off,dword *curc,byte *name,dword *isd)
{	dword i;
	byte *s;
	DIRfat32 *buf;

	if((buf=(DIRfat32 *)kmalloc(32))==NULL) return -1;
	do	/*取得有效目录项*/
	{	if(rwfileFAT32(p,dir,curc,*off,32,(byte*)buf,0)) {kfree(buf,32);return -1;}
		if(buf->name[0]==0 || (*curc)==0) {(*off)=(*curc)=0;kfree(buf,32);return 1;}
		(*off)+=32;
	}while(buf->name[0]=='.' || buf->name[0]==0xe5 || (buf->attr&0x0e));
	memset(name,0,13);
	for(s=name,i=0;i<11;i++)	/*转换文件名格式*/
	{	if(i==8) *(s++)='.';
		*s=buf->name[i];
		if(*s!=' ') s++;
	}
	s--;
	if(*s!='.') s++;
	*s=0;
	if(*name==0x05) *name=0xe5;	/*处理0x05字符*/
	if(buf->attr&0x10) *isd=1; else *isd=0;
	kfree(buf,32);
	return 0;
}

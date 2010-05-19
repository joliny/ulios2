######################
#	Makefile for app
#	作者：孙亮
#	功能：应用程序组建脚本
#	最后修改日期：2009-07-02
######################

# Entry point of ulios testapp
ENTRYPOINT	= 0x08048000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE)
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h ../fs/fsapi.h
OBJS		= ../objs/test.o ../objs/pro.o
HEADOBJ		= ../objs/apphead.o

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : ../out/ULIOS/test.bin ../out/ULIOS/pro.bin

redo : clean all

clean :
	rm -f $(OBJS) $(HEADOBJ) $(BINS)

../out/ULIOS/test.bin : ../objs/test.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/test.o

../out/ULIOS/pro.bin : ../objs/pro.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/pro.o

../objs/apphead.o : ../MkApi/apphead.asm
	$(ASM) $(ASMFLAGS) -o $@ $<

../objs/test.o : test.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/pro.o : pro.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

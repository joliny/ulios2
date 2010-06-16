######################
#	Makefile for app
#	作者：孙亮
#	功能：应用程序组建脚本
#	最后修改日期：2009-07-02
######################

# Entry point of ulios testapp
ENTRYPOINT	= 0x08000000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE) -Map ../objs/test.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h ../fs/fsapi.h
OBJS		= ../objs/test.o ../objs/pro.o ../objs/gdi.o ../objs/loader.o ../objs/cmd.o
HEADOBJ		= ../objs/apphead.o

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : ../out/ulios/test.bin ../out/ulios/pro.bin ../out/ulios/loader.bin ../out/ulios/cmd.bin

redo : clean all

clean :
	rm -f $(OBJS) $(HEADOBJ) $(BINS)

../out/ulios/test.bin : ../objs/test.o ../objs/gdi.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/test.o ../objs/gdi.o

../out/ulios/pro.bin : ../objs/pro.o ../objs/gdi.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/pro.o ../objs/gdi.o

../out/ulios/loader.bin : ../objs/loader.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/loader.o

../out/ulios/cmd.bin : ../objs/cmd.o $(HEADOBJ) $(LDFILE)
	$(LD) $(LDFLAGS) -o $@ $(HEADOBJ) ../objs/cmd.o

../objs/apphead.o : ../MkApi/apphead.c
	$(CC) $(CFLAGS) -o $@ $<

../objs/gdi.o : ../driver/gdi.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/test.o : test.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/pro.o : pro.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/loader.o : loader.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/cmd.o : cmd.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

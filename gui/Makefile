######################
#	Makefile for ulios graphical user interface
#	作者：孙亮
#	功能：图形用户界面组建脚本
#	最后修改日期：2010-11-09
######################

# Entry point of ulios graphical user interface
ENTRYPOINT	= 0x08000000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall -m32 -fno-builtin -fleading-underscore -fno-stack-protector
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE) -mi386go32 -Map ../objs/guimap.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
GUIBIN		= ../out/ulios/gui.bin
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h gui.h guiapi.h control.h
OBJS		= ../objs/apphead.o ../objs/guiapi.o ../objs/guialloc.o ../objs/guiobj.o ../objs/control.o

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : $(GUIBIN)

redo : clean all

clean :
	rm -f $(OBJS) $(GUIBIN)

$(GUIBIN) : $(OBJS) $(LDFILE)
	$(LD) $(LDFLAGS) -o $(GUIBIN) $(OBJS) ../objs/gdi.o

../objs/apphead.o : ../MkApi/apphead.c
	$(CC) $(CFLAGS) -o $@ $<

../objs/guiapi.o : guiapi.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/guialloc.o : guialloc.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/guiobj.o : guiobj.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/control.o : control.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

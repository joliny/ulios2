######################
#	Makefile for ulios file system
#	���ߣ�����
#	���ܣ��ļ�ϵͳ�齨�ű�
#	����޸����ڣ�2010-03-31
######################

# Entry point of ulios file system
ENTRYPOINT	= 0x08048000

# Programs, flags, etc.
ASM		= nasm
DASM		= ndisasm
CC		= gcc
LD		= ld
ASMFLAGS	= -f aout
CFLAGS		= -c -O1 -Wall
LDFILE		= ../MkApi/app.ld
LDFLAGS		= -T $(LDFILE) -Map ../objs/fsmap.txt
DASMFLAGS	= -u -o $(ENTRYPOINT) -e $(ENTRYOFFSET)

# This Program
FSBIN		= ../out/ULIOS/fs.bin
HEADS		= ../MkApi/ulimkapi.h ../driver/basesrv.h fs.h fsapi.h
OBJS		= ../objs/apphead.o ../objs/fsapi.o ../objs/malloc.o ../objs/cache.o ../objs/fs.o ../objs/fat32.o ../objs/ulifs.o

# All Phony Targets
.PHONY : all redo clean

# Default starting position

all : $(FSBIN)

redo : clean all

clean :
	rm -f $(OBJS) $(FSBIN)

$(FSBIN) : $(OBJS) $(LDFILE)
	$(LD) $(LDFLAGS) -o $(FSBIN) $(OBJS)

../objs/apphead.o : ../MkApi/apphead.c
	$(CC) $(CFLAGS) -o $@ $<

../objs/fsapi.o : fsapi.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/malloc.o : malloc.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/cache.o : cache.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/fs.o : fs.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/fat32.o : fat32.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<

../objs/ulifs.o : ulifs.c $(HEADS)
	$(CC) $(CFLAGS) -o $@ $<
##############################################################
## Makefile for 'CrazyWWWBoard.cgi'                         ##
##                                                          ##
##             Technical contact : nobreak@nobreak.com      ##
##                          Developed by 'Seung-young, Kim' ##
##                                                          ##
##                          (c) Nobreak Technologies, Inc.  ##
##############################################################

##
## Define
##
PRGNAME = CrazyWWWBoard
CGINAME = $(PRGNAME).cgi 

# Which compiler
CC      = gcc

# Where are include files kept
INCLUDE = -I./

# Options for release
CFLAGS  = -Wall

# You may need to add -lcrypt if you find some problems while linking
# crypt() library.
LIB     = $(DEFLIB) $(EXTLIB)
DEFLIB  = ../qDecoder-4.3.1/libqDecoder.a ../gdbm-1.7.3/libgdbm.a ../md5/libmd5.a
#EXTLIB  = -lcrypt

OBJ     = $(OBJ1) $(OBJ2) $(OBJ3)
OBJ1    = cwb.o cwb_admin.o cwb_stat.o cwb_sync.o
OBJ2    = cwb_list.o cwb_read.o cwb_write.o cwb_delete.o cwb_download.o
OBJ3	= gdbm_lib.o

##
## Main
##
all:	$(CGINAME)

reall:	clean all

##
## CrazyWWWBoard.cgi
##
$(CGINAME): $(OBJ) 
	$(CC) $(INCLUDE) $(CFLAGS) -o $(CGINAME) $(OBJ) $(LIB)
	chmod 6755 $(CGINAME)

## Compile Module
%.o:	%.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c -o $@ $<

## Clear Module
clean:
	rm -f $(OBJ) $(CGINAME)


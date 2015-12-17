##############################################################
## Makefile for 'CrazyAdmin.cgi'                            ##
##                                                          ##
##             Technical contact : nobreak@nobreak.com      ##
##                          Developed by 'Seung-young, Kim' ##
##                                                          ##
##                          (c) Nobreak Technologies, Inc.  ##
##############################################################

##
## Define
##
PRGNAME = CrazyAdmin
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
OBJ1    = adm.o adm_auth.o adm_newpass.o
OBJ2    = adm_list.o adm_remove.o adm_create.o adm_message.o adm_config.o
OBJ3	= gdbm_lib.o

##
## Main
##
all:	$(CGINAME)

reall:	clean all

##
## CrazyAdmin.cgi
##
$(CGINAME): $(OBJ) 
	$(CC) -I$(INCLUDE) $(CFLAGS) -o $(CGINAME) $(OBJ) $(LIB)
	chmod 6755 $(CGINAME)

## Compile Module
%.o:	%.c
	$(CC) $(INCLUDE) $(CFLAGS) -c -o $@ $<

## Clear Module
clean:
	rm -f $(OBJ) $(CGINAME)


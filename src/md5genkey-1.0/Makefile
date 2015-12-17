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
PRGNAME = md5genkey
CGINAME = $(PRGNAME) 

# Which compiler
CC      = gcc

# Where are include files kept
INCLUDE = -I./

# Options for release
CFLAGS  = -Wall

# You may need to add -lcrypt if you find some problems while linking
# crypt() library.
LIB     = $(DEFLIB) $(EXTLIB)
DEFLIB  = ../md5/libmd5.a
#EXTLIB  = -lcrypt

OBJ     = $(OBJ1) $(OBJ2) $(OBJ3)
OBJ1    = md5genkey.o

##
## Main
##
all:	$(CGINAME)

reall:	clean all

##
## LINK
##
$(CGINAME): $(OBJ) 
	$(CC) $(INCLUDE) $(CFLAGS) -o $(CGINAME) $(OBJ) $(LIB)

## Compile Module
%.o:	%.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c -o $@ $<

## Clear Module
clean:
	rm -f $(OBJ) $(CGINAME)


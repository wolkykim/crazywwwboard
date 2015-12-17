##############################################################
## Makefile for 'CrazyArchive'                              ##
##                                                          ##
##             Technical contact : nobreak@nobreak.com      ##
##                          Developed by 'Seung-young, Kim' ##
##                                                          ##
##                          (c) Nobreak Technologies, Inc.  ##
##############################################################

##
## Define
##
PRGNAME = CrazyArchive

# Which compiler
CC      = gcc

# Where are include files kept
INCLUDE = -I./

# Options for release
CFLAGS  = -Wall

LIB     = $(DEFLIB)
DEFLIB  = ../qDecoder-4.3.1/libqDecoder.a ../gdbm-1.7.3/libgdbm.a

OBJ     = $(OBJ1) $(OBJ2)
OBJ1    = arc.o arc_parse.o arc_write.o
OBJ2    = gdbm_lib.o

##
## Main
##
all:	$(PRGNAME)

reall:	clean all

##
## CrazyArchive
##
$(PRGNAME): $(OBJ) 
	$(CC) -I$(INCLUDE) $(CFLAGS) -o $(PRGNAME) $(OBJ) $(LIB)
	chmod 6755 $(PRGNAME)

## Compile Module
%.o:	%.c
	$(CC) $(INCLUDE) $(CFLAGS) -c -o $@ $<

## Clear Module
clean:
	rm -f $(OBJ) $(PRGNAME)


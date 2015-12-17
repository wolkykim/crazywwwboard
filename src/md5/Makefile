##############################################################
## Makefile for 'MD5'                                       ##
##                                                          ##
##             Technical contact : nobreak@nobreak.com      ##
##                          Developed by 'Seung-young, Kim' ##
##                                                          ##
##                          (c) Nobreak Technologies, Inc.  ##
##############################################################

#
# Define
#
LIBNAME = libmd5.a

# Which compiler
CC      = gcc

# Where are include files kept
INCLUDE = .

# Options for release
CFLAGS  = -Wall

# Which library archiver
AR	= ar
ARFLAGS = -q

# Which ranlib
RANLIB	= ranlib

OBJ  = $(OBJ1) $(OBJ2)
OBJ1 = md5_crypt.o md5c.o
OBJ2 = 

## Make Library
all: $(OBJ)
	$(AR) $(ARFLAGS) $(LIBNAME) $(OBJ)
	$(RANLIB) $(LIBNAME)

reall: clean all

## Compile Module
%.o:	%.c
	$(CC) -I$(INCLUDE) $(CFLAGS) -c -o $@ $<

## Clear Module
clean:
	rm -f $(OBJ) $(LIBNAME)


#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET = libscenery.a

CFILES = scanner.c parser.c common.c mesh.c
HFILES = 
LFILES = scanner.l
YFILES = parser.y
OFILES = $(CFILES:.c=.o)

CC = gcc
CFLAGS = -g -Wall
# CFLAGS = -O2 -Wall

FLEX = flex
BISON = bison -v
AR = ar

INCLUDES = 

LIBS = -lfl


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(OFILES) $(HFILES)
	$(AR) rv $(TARGET) $(OFILES)

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

scanner.c: scanner.l parser.h
	$(FLEX) -oscanner.c scanner.l

scanner.o: scanner.c
	$(CC) $(CFLAGS) -c scanner.c

parser.h: parser.y
	$(BISON) -o parser.c -d parser.y

parser.c: parser.y common.h mesh.h scenery.h
	$(BISON) -o parser.c parser.y

parser.o: parser.c
	$(CC) $(CFLAGS) -c parser.c

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

mesh.o: mesh.c mesh.h common.h
	$(CC) $(CFLAGS) -c mesh.c


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/05/16 16:07:02  curt
# Initial revision.
#

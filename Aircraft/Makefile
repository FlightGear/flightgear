#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET = libaircraft.a

CFILES = aircraft.c
HFILES = aircraft.h
OFILES = $(CFILES:.c=.o)

CC = gcc
CFLAGS = -g -Wall
# CFLAGS = -O2 -Wall

AR = ar

INCLUDES = 

LIBS = 


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES) $(HFILES)
	$(AR) rv $(TARGET) $(OFILES)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

aircraft.o: aircraft.c aircraft.h
	$(CC) $(CFLAGS) $(INCLUDES) -c aircraft.c

aircraft.h: ../flight/flight.h ../controls/controls.h
	touch aircraft.h


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/05/16 15:58:23  curt
# Initial revision.
#

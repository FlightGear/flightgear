#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET=libflight.a

CFILES = slew.c
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

$(TARGET): $(OFILES)
	$(AR) rv $(TARGET) $(OFILES)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

flight.h: slew.h
	touch flight.h

slew.o: slew.c slew.h flight.h ../aircraft/aircraft.h ../controls/controls.h
	$(CC) $(CFLAGS) $(INCLUDES) -c slew.c


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/05/16 16:04:44  curt
# Initial revision.
#

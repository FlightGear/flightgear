#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET = stamp-done

CFILES = 
HFILES = controls.h
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
	touch stamp-done

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

controls.h: ../limits.h
	touch controls.h


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/05/16 15:59:47  curt
# Initial revision.
#

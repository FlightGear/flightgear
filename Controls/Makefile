#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# Copyright (C) 1997  Curtis L. Olson  - curt@infoplane.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET = libControls.a

CFILES = controls.c
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

$(TARGET): $(OFILES)
	$(AR) rv $(TARGET) $(OFILES)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) lib*.a *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

controls.h: ../limits.h
	touch controls.h

controls.o: controls.c controls.h ../Aircraft/aircraft.h
	$(CC) $(CFLAGS) $(INCLUDES) -c controls.c


#---------------------------------------------------------------------------
# $Log$
# Revision 1.5  1997/06/25 15:39:46  curt
# Minor changes to compile with rsxnt/win32.
#
# Revision 1.4  1997/06/21 17:12:47  curt
# Capitalized subdirectory names.
#
# Revision 1.3  1997/05/31 19:16:27  curt
# Elevator trim added.
#
# Revision 1.2  1997/05/23 15:40:32  curt
# Added GNU copyright headers.
#
# Revision 1.1  1997/05/16 15:59:47  curt
# Initial revision.
#

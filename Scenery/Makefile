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


TARGET = libScenery.a

CFILES = common.c mesh.c
HFILES = 
OFILES = $(CFILES:.c=.o)

CC = gcc
CFLAGS = -g -Wall
# CFLAGS = -O2 -Wall

AR = ar

INCLUDES = 


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(OFILES) $(HFILES)
	$(AR) rv $(TARGET) $(OFILES)

clean:
	rm -f *.o $(TARGET) lib*.a *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

mesh.o: mesh.c mesh.h common.h
	$(CC) $(CFLAGS) -c mesh.c


#---------------------------------------------------------------------------
# $Log$
# Revision 1.9  1997/06/25 15:39:48  curt
# Minor changes to compile with rsxnt/win32.
#
# Revision 1.8  1997/06/21 17:58:07  curt
# directory shuffling ...
#
# Revision 1.1  1997/06/21 17:39:28  curt
# Moved to the ParseScn subdirectory.
#
# Revision 1.6  1997/06/21 17:12:55  curt
# Capitalized subdirectory names.
#
# Revision 1.5  1997/05/31 19:16:29  curt
# Elevator trim added.
#
# Revision 1.4  1997/05/27 17:48:50  curt
# Added -f flag to flex to generate a "fast" scanner.
#
# Revision 1.3  1997/05/23 15:40:40  curt
# Added GNU copyright headers.
#
# Revision 1.2  1997/05/17 00:17:01  curt
# Cosmetic changes.
#
# Revision 1.1  1997/05/16 16:07:02  curt
# Initial revision.
#

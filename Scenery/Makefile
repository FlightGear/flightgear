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
	rm -f *.o $(TARGET) parser.output *~ core


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
# Revision 1.3  1997/05/23 15:40:40  curt
# Added GNU copyright headers.
#
# Revision 1.2  1997/05/17 00:17:01  curt
# Cosmetic changes.
#
# Revision 1.1  1997/05/16 16:07:02  curt
# Initial revision.
#

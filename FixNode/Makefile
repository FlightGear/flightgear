#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started October 1997.
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


TARGET = fixnode

CFILES = ../Dem2node/demparse.c fixnode.c main.c ../Dem2node/mesh.c triload.c
OFILES = $(CFILES:.c=.o)


include ../make.inc


CFLAGS = $(FG_CFLAGS) -g


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(OFILES)
	$(CC) $(OFILES) -o $(TARGET)

clean:
	rm -f *.o $(TARGET) lib*.a *.os2 *~ core

realclean: clean


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

makedepend:
	$(CC) -MM *.c > depend

include depend

../Dem2node/demparse.o: ../Dem2node/demparse.c
	$(CC) $(CFLAGS) -c ../Dem2node/demparse.c -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o $@

mesh.o: mesh.c
	$(CC) $(CFLAGS) -c mesh.c -o $@

triload.o: triload.c
	$(CC) $(CFLAGS) -c triload.c -o $@


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/11/27 00:17:32  curt
# Initial revision.
#

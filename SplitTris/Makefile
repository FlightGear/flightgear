#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started January 1998.
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


TARGET = splittris

CFILES = splittris.c ../../Src/Scenery/tileutils.c
OFILES = $(CFILES:.c=.o)


include ../make.inc


CFLAGS = $(FG_CFLAGS) -g


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

all: $(TARGET)

$(TARGET): $(OFILES)
	$(CC) $(OFILES) -o $(TARGET) ../../Src/Math/libMath.a -lm

clean:
	rm -f *.o $(TARGET) lib*.a *.os2 *~ core

realclean: clean


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

makedepend:
	$(CC) -MM *.c > depend

include depend

splittris.o: splittris.c
	$(CC) $(CFLAGS) -c splittris.c -o $@


#---------------------------------------------------------------------------
# $Log$
# Revision 1.2  1998/01/14 15:54:42  curt
# Initial revision completed.
#
# Revision 1.1  1998/01/14 02:11:30  curt
# Initial revision.
#

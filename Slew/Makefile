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


TARGET=libSlew.a

CFILES = slew.c
OFILES = $(CFILES:.c=.o)


include ../../make.inc


CFLAGS = $(FG_CFLAGS)


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES)
	$(AR) rv $(TARGET) $(OFILES)
	$(RANLIB) $(TARGET)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) lib*.a *.os2 *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

include depend

slew.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c slew.c -o $@


#---------------------------------------------------------------------------
# $Log$
# Revision 1.8  1997/07/20 02:19:10  curt
# First stab at a system to generate os2 makefiles automatically.
#
# Revision 1.7  1997/07/12 02:24:49  curt
# Added ranlib.
#
# Revision 1.6  1997/06/27 21:38:07  curt
# Working on Makefile structure.
#
# Revision 1.5  1997/06/27 20:03:36  curt
# Working on Makefile structure.
#
# Revision 1.4  1997/06/26 19:08:31  curt
# Restructuring make, adding automatic "make dep" support.
#
# Revision 1.3  1997/06/25 15:39:46  curt
# Minor changes to compile with rsxnt/win32.
#
# Revision 1.2  1997/06/21 17:12:51  curt
# Capitalized subdirectory names.
#
# Revision 1.1  1997/05/29 02:29:42  curt
# Moved to their own directory.
#
# Revision 1.2  1997/05/23 15:40:36  curt
# Added GNU copyright headers.
#
# Revision 1.1  1997/05/16 16:04:44  curt
# Initial revision.
#

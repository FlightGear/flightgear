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

CFILES = common.c geometry.c mesh.c moon.c obj.c orbits.c planets.c \
	scenery.c stars.c sun.c

OFILES = $(CFILES:.c=.o)


include ../make.inc


CFLAGS = $(FG_CFLAGS)


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES) $(HFILES) 
	$(AR) rv $(TARGET) $(OFILES)
	$(RANLIB) $(TARGET)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) lib*.a *.os2 *~ core

realclean: clean


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

include depend

common.o:
	$(CC) $(CFLAGS) -c common.c -o $@

mesh.o:
	$(CC) $(CFLAGS) -c mesh.c -o $@

mesh2GL.o:
	$(CC) $(CFLAGS) -c mesh2GL.c -o $@

moon.o:
	$(CC) $(CFLAGS) -c moon.c -o $@

obj.o:
	$(CC) $(CFLAGS) -c obj.c -o $@

orbits.o:
	$(CC) $(CFLAGS) -c orbits.c -o $@

planets.o:
	$(CC) $(CFLAGS) -c planets.c -o $@

scenery.o:
	$(CC) $(CFLAGS) -c scenery.c -o $@

stars.c:
	$(CC) $(CFLAGS) -c stars.c -o $@

sun.o:
	$(CC) $(CFLAGS) -c sun.c -o $@

geometry.o:
	$(CC) $(CFLAGS) -c geometry.c -o $@


#---------------------------------------------------------------------------
# $Log$
# Revision 1.23  1997/10/28 21:00:20  curt
# Changing to new terrain format.
#
# Revision 1.22  1997/10/25 03:30:07  curt
# Misc. tweaks.
#
# Revision 1.21  1997/10/25 03:18:26  curt
# Incorporated sun, moon, and planet position and rendering code contributed
# by Durk Talsma.
#
# Revision 1.20  1997/09/22 14:44:21  curt
# Continuing to try to align stars correctly.
#
# Revision 1.19  1997/08/27 03:30:23  curt
# Changed naming scheme of basic shared structures.
#
# Revision 1.18  1997/08/02 19:10:12  curt
# Incorporated mesh2GL.c into mesh.c
#
# Revision 1.17  1997/07/23 21:52:23  curt
# Put comments around the text after an #endif for increased portability.
#
# Revision 1.16  1997/07/20 02:19:11  curt
# First stab at a system to generate os2 makefiles automatically.
#
# Revision 1.15  1997/07/12 02:24:47  curt
# Added ranlib.
#
# Revision 1.14  1997/06/29 21:16:47  curt
# More twiddling with the Scenery Management system.
#
# Revision 1.13  1997/06/27 21:38:10  curt
# Working on Makefile structure.
#
# Revision 1.12  1997/06/27 20:03:37  curt
# Working on Makefile structure.
#
# Revision 1.11  1997/06/26 22:14:57  curt
# Beginning work on a scenery management system.
#
# Revision 1.10  1997/06/26 19:08:34  curt
# Restructuring make, adding automatic "make dep" support.
#
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

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


TARGET = libMath.a

CFILES = MAT3geom.c MAT3inv.c MAT3mat.c MAT3vec.c
HFILES = mat3.h mat3defs.h mat3err.h
OFILES = $(CFILES:.c=.o)


include ../make.inc


CFLAGS = $(FG_CFLAGS)


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES) $(HFILES)
	$(AR) rv $(TARGET) $(OFILES)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) lib*.a *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

include depend


#---------------------------------------------------------------------------
# $Log$
# Revision 1.7  1997/07/05 20:43:31  curt
# renamed mat3 directory to Math so we could add other math related routines.
#
# Revision 1.6  1997/06/27 21:38:14  curt
# Working on Makefile structure.
#
# Revision 1.5  1997/06/27 20:03:40  curt
# Working on Makefile structure.
#
# Revision 1.4  1997/06/26 19:08:38  curt
# Restructuring make, adding automatic "make dep" support.
#
# Revision 1.3  1997/06/25 15:39:49  curt
# Minor changes to compile with rsxnt/win32.
#
# Revision 1.2  1997/05/30 19:30:16  curt
# The LaRCsim flight model is starting to look like it is working.
#
# Revision 1.1  1997/05/30 19:25:56  curt
# The MAT3 routines from SRGP.

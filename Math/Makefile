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

ARLIBRARY = libMath.a
TARGETS = $(ARLIBRARY) geotest

CFILES = MAT3geom.c MAT3inv.c MAT3mat.c MAT3vec.c fg_geodesy.c fg_random.c polar.c vector.c
CXXFILES = 

LDIRT = $(FG_ROOT_LIB)/$(ARLIBRARY)

include $(FG_ROOT_SRC)/commondefs

geotest: geotest.o $(ARLIBRARY)
	$(CC) $(LDFLAGS) -o $@ $< -lMath -lm -lc

include $(COMMONRULES)

#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started January 1998.
#
# Copyright (C) 1998  Curtis L. Olson  - curt@me.umn.edu
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


bin_PROGRAMS = splittris

splittris_SOURCES = \
	MAT3vec.c \
	fg_geodesy.c fg_geodesy.h \
	mat3.h \
	polar.c polar.h \
	splittris.c splittris.h

splittris_LDADD = \
	$(top_builddir)/Simulator/Scenery/Bucket/libBucket.la

INCLUDES += -I../../Simulator


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1998/04/08 23:21:10  curt
# Adopted Gnu automake/autoconf system.
#
# Revision 1.3  1998/01/21 02:55:55  curt
# Incorporated new make system from Bob Kuehne <rpk@sgi.com>.
#
# Revision 1.2  1998/01/14 15:54:42  curt
# Initial revision completed.
#
# Revision 1.1  1998/01/14 02:11:30  curt
# Initial revision.
#

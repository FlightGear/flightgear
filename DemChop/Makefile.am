#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started March 1999.
#
# Copyright (C) 1999  Curtis L. Olson  - curt@flightgear.org
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


bin_PROGRAMS = demchop

demchop_SOURCES = \
	demchop.cxx point2d.hxx

demchop_LDADD = \
	$(top_builddir)/Tools/Lib/DEM/libDEM.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	$(top_builddir)/Lib/zlib/libz.a \
	$(base_LIBS)

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Tools/Lib

# We can't build this with "-O2" (optimization) since this causes a seg fault
# I haven't found a way to strip this out of the CXXFLAGS, so I'm just
# setting it to "-g"
CXXFLAGS = -g 


#---------------------------------------------------------------------------
# $Log$
# Revision 1.2  1999/03/12 22:53:45  curt
# First working version!
#
# Revision 1.1  1999/03/10 01:02:54  curt
# Initial revision.
#

#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started June 1998.
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


bin_PROGRAMS = deminfo

deminfo_SOURCES = \
	deminfo.cxx

deminfo_LDADD = \
	$(top_builddir)/Lib/DEM/libDEM.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/zlib/libz.a

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib

# We can't build this with "-O2" (optimization) since this causes a seg fault
# I haven't found a way to strip this out of the CXXFLAGS, so I'm just
# setting it to "-g"
CXXFLAGS = -g 


#---------------------------------------------------------------------------
# $Log$
# Revision 1.2  1998/07/30 23:49:24  curt
# Removed libtool support.
#
# Revision 1.1  1998/06/04 19:18:04  curt
# Initial revision.
#

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
#---------------------------------------------------------------------------


bin_PROGRAMS = genapts

genapts_SOURCES = \
	area.cxx area.hxx \
	convex_hull.cxx convex_hull.hxx \
	main.cxx \
	point2d.cxx point2d.hxx

genapts_LDADD = \
	$(top_builddir)/Tools/Lib/Polygon/libPolygon.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(top_builddir)/Lib/zlib/libz.a \
	$(base_LIBS)

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Tools/Lib


#---------------------------------------------------------------------------

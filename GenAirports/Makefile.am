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


bin_PROGRAMS = genapts

genapts_SOURCES = \
	area.cxx area.hxx \
	convex_hull.cxx convex_hull.hxx \
	main.cxx \
	point2d.cxx point2d.hxx

genapts_LDADD = \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/zlib/libz.a \
	$(base_LIBS)

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib


#---------------------------------------------------------------------------
# $Log$
# Revision 1.3  1998/11/04 23:01:54  curt
# Changes to the automake/autoconf system to reduce the number of libraries
# that are unnecessarily linked into the various executables.
#
# Revision 1.2  1998/09/04 23:04:47  curt
# Beginning of convex hull genereration routine.
#
# Revision 1.1  1998/09/01 19:34:32  curt
# Initial revision.
#
# Revision 1.2  1998/07/30 23:49:18  curt
# Removed libtool support.
#
# Revision 1.1  1998/07/20 12:54:53  curt
# Whoops, need to commit Makefile.am, not Makefile.
#
# Revision 1.2  1998/04/14 02:25:59  curt
# Code reorganizations.  Added a Lib/ directory for more general libraries.
#
# Revision 1.1  1998/04/08 22:54:57  curt
# Adopted Gnu automake/autoconf system.
#
# Revision 1.2  1998/01/21 02:55:46  curt
# Incorporated new make system from Bob Kuehne <rpk@sgi.com>.
#
# Revision 1.1  1998/01/15 02:45:25  curt
# Initial revision.
#


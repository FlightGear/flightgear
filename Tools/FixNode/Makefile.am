#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started October 1997.
#
# Copyright (C) 1997 - 1998  Curtis L. Olson  - curt@me.umn.edu
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


bin_PROGRAMS = fixnode

fixnode_SOURCES = \
	fixnode.cxx fixnode.hxx \
	main.cxx

fixnode_LDADD = \
	$(top_builddir)/Tools/Lib/DEM/libDEM.a \
        $(top_builddir)/Lib/Bucket/libBucket.a \
        $(top_builddir)/Lib/Misc/libMisc.a \
        $(top_builddir)/Lib/zlib/libz.a \
	$(base_LIBS)

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Tools/Lib

# We can't build this with "-O2" (optimization) since this causes a seg fault
# I haven't found a way to strip this out of the CXXFLAGS, so I'm just
# setting it to "-g"
# CXXFLAGS = -g


#---------------------------------------------------------------------------
# $Log$
# Revision 1.10  1999/03/17 23:50:59  curt
# Removed forced -g compiler flag.
#
# Revision 1.9  1999/03/08 22:00:45  curt
# Lots of directory layout reorganization.
#
# Revision 1.8  1999/02/01 21:09:30  curt
# Moving location of Lib/DEM/ to Tools/DEM/
#
# Revision 1.7  1998/11/04 23:01:51  curt
# Changes to the automake/autoconf system to reduce the number of libraries
# that are unnecessarily linked into the various executables.
#
# Revision 1.6  1998/09/19 20:43:50  curt
# C++-ified and STL-ified the code.  Combined triload.* and fixnode.* into
# a single file.
#
# Revision 1.5  1998/09/19 18:01:26  curt
# Support for changes to libDEM.a
#
# Revision 1.4  1998/07/30 23:49:24  curt
# Removed libtool support.
#
# Revision 1.3  1998/04/18 04:02:54  curt
# Added zlib support in placed and other misc. tweaks.
#
# Revision 1.2  1998/04/14 02:26:02  curt
# Code reorganizations.  Added a Lib/ directory for more general libraries.
#
# Revision 1.1  1998/04/08 23:05:54  curt
# Adopted Gnu automake/autoconf system.
#
# Revision 1.4  1998/04/06 21:09:44  curt
# Additional win32 support.
# Fixed a bad bug in dem file parsing that was causing the output to be
# flipped about x = y.
#
# Revision 1.3  1998/03/19 02:50:19  curt
# Updated to support -lDEM class.
#
# Revision 1.2  1998/01/21 02:55:50  curt
# Incorporated new make system from Bob Kuehne <rpk@sgi.com>.
#
# Revision 1.1  1997/11/27 00:17:32  curt
# Initial revision.
#

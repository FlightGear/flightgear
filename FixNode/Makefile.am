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
	main.cxx \
	triload.cxx triload.hxx

fixnode_LDADD = \
	$(top_builddir)/Tools/DEM/libDEM.la \
        $(top_builddir)/Simulator/Scenery/Bucket/libBucket.la

INCLUDES += -I.. -I../../Simulator


#---------------------------------------------------------------------------
# $Log$
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

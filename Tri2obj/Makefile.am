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


bin_PROGRAMS = tri2obj

tri2obj_SOURCES = tri2obj.c tri2obj.h

tri2obj_LDADD = \
	$(top_builddir)/Lib/Bucket/libBucket.la \
	$(top_builddir)/Lib/Math/libMath.la \
        $(top_builddir)/Lib/Debug/libDebug.la \
        $(top_builddir)/Lib/zlib/libz.la

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib


#---------------------------------------------------------------------------
# $Log$
# Revision 1.4  1998/04/24 00:44:07  curt
# Added zlib support.
#
# Revision 1.3  1998/04/18 04:01:29  curt
# Now use libMath rather than having local copies of math routines.
#
# Revision 1.2  1998/04/14 02:26:09  curt
# Code reorganizations.  Added a Lib/ directory for more general libraries.
#
# Revision 1.1  1998/04/08 23:22:13  curt
# Adopted Gnu automake/autoconf system.
#
# Revision 1.2  1998/01/21 02:55:46  curt
# Incorporated new make system from Bob Kuehne <rpk@sgi.com>.
#
# Revision 1.1  1998/01/15 02:45:25  curt
# Initial revision.
#


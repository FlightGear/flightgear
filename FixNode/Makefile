#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started October 1997.
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


TARGET = fixnode

CFILES = fixnode.c main.c triload.c
LDLIBS = -lDEM -lScenery -lm


include $(FG_ROOT_SRC)/commondefs


#---------------------------------------------------------------------------
# Rule for TARGET
#---------------------------------------------------------------------------

$(TARGET): $(OBJECTS) $(FG_ROOT_LIB)/stamp_libs
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LDLIBS)


include $(COMMONRULES)


#---------------------------------------------------------------------------
# $Log$
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

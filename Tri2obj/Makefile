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


TARGET = tri2obj

CFILES = tri2obj.c
LDLIBS = -lMath -lScenery -lm


include $(FG_ROOT_SRC)/commondefs


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LDLIBS)


include $(COMMONRULES)


#---------------------------------------------------------------------------
# $Log$
# Revision 1.5  1998/01/21 02:55:56  curt
# Incorporated new make system from Bob Kuehne <rpk@sgi.com>.
#
# Revision 1.4  1998/01/17 01:25:39  curt
# Added support for shared normals.
#
# Revision 1.3  1997/12/02 13:13:32  curt
# Fixed problem with averaged vertex normals.
#
# Revision 1.2  1997/11/14 00:29:13  curt
# Transform scenery coordinates at this point in pipeline when scenery is
# being translated to .obj format, not when it is being loaded into the end
# renderer.  Precalculate normals for each node as average of the normals
# of each containing polygon so Garoude shading is now supportable.
#
# Revision 1.1  1997/10/29 23:05:14  curt
# Initial revision.
#
# Revision 1.1  1997/10/23 23:12:09  curt
# Initial revision.
#

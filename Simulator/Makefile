#---------------------------------------------------------------------------
# Toplevel Project Makefile
#
# Written by Curtis Olson, started May 1997.
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


CC = gcc


SUBSUBDIRS = flight/LaRCsim flight/slew
SUBDIRS = aircraft controls flight mat3 scenery timer
MAIN = OpenGL


all:
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    ( cd $$dir; make CC=$(CC) ) ; \
	done


install:
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    ( cd $$dir; make CC=$(CC) install) ; \
	done


clean:
	-rm -f *~
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    (cd $$dir; make clean) ; \
	done


#---------------------------------------------------------------------------
# $Log$
# Revision 1.6  1997/06/16 19:32:50  curt
# Starting to add general timer support.
#
# Revision 1.5  1997/05/30 19:26:56  curt
# The LaRCsim flight model is starting to look like it is working.
#
# Revision 1.4  1997/05/29 02:31:43  curt
# Update subdirectory structure.
#
# Revision 1.3  1997/05/23 15:40:04  curt
# Added GNU copyright headers.
#
# Revision 1.2  1997/05/23 00:35:09  curt
# Trying to get fog to work ...
#
# Revision 1.1  1997/05/16 15:51:13  curt
# Initial revision.
#

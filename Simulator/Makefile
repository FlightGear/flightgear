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


include make.inc


SUBSUBDIRS = Flight/LaRCsim Flight/Slew
SUBDIRS = Aircraft Controls Flight mat3 Scenery Timer
MAIN = OpenGL


all: 
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    ( cd $$dir; $(MAKE) ) ; \
	done

depend:
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    ( echo "Making depend in $$dir"; \
	      cd $$dir; $(CC) -MM *.c > depend ) ; \
	done

clean:
	-rm -f *~
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    (cd $$dir; $(MAKE) clean) ; \
	done


tar: clean
	(cd ../..; \
	tar cvf prototype-0.04.tar FlightGear/COPYING FlightGear/Docs \
	FlightGear/Scenery/mesa-e.scn FlightGear/Src FlightGear/Thanks)


#---------------------------------------------------------------------------
# $Log$
# Revision 1.14  1997/06/29 21:19:16  curt
# Working on scenery management system.
#
# Revision 1.13  1997/06/28 00:49:02  curt
# Tweaking Makefile structure ..
#
# Revision 1.12  1997/06/27 21:37:59  curt
# Working on Makefile structure.
#
# Revision 1.11  1997/06/27 20:03:31  curt
# Working on Makefile structure.
#
# Revision 1.10  1997/06/26 19:08:22  curt
# Restructuring make, adding automatic "make dep" support.
#
# Revision 1.9  1997/06/22 21:44:40  curt
# Working on intergrating the VRML (subset) parser.
#
# Revision 1.8  1997/06/21 17:52:22  curt
# Continue directory shuffling ... everything should be compilable/runnable
# again.
#
# Revision 1.7  1997/06/21 17:12:38  curt
# Capitalized subdirectory names.
#
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

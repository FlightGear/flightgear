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
SUBDIRS = Aircraft Controls Flight Math Scenery Timer Weather
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

Makefile-os2:
	cat Makefile | perl mkmfos2.pl > Makefile.os2; \
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    ( echo "Making Makefile.os2 in $$dir"; \
	      cat $$dir/Makefile | perl mkmfos2.pl > $$dir/Makefile.os2; \
	      cat $$dir/depend   | perl mkmfos2.pl > $$dir/depend.os2) ; \
	done

clean:
	-rm -f *.os2 *~
	for dir in $(SUBSUBDIRS) $(SUBDIRS) $(MAIN); do \
	    (cd $$dir; $(MAKE) clean) ; \
	done


tar: clean
	(cd ../..; \
	tar cvf prototype-$(VERSION).tar FlightGear/COPYING FlightGear/Docs \
	FlightGear/Src FlightGear/Thanks)

zip: clean
	(cd ../..; \
	zip -r prototype-$(VERSION).zip FlightGear/COPYING FlightGear/Docs \
	FlightGear/Src FlightGear/Thanks)


#---------------------------------------------------------------------------
# $Log$
# Revision 1.21  1997/07/30 16:12:38  curt
# Moved fg_random routines from Util/ to Math/
#
# Revision 1.20  1997/07/20 02:19:09  curt
# First stab at a system to generate os2 makefiles automatically.
#
# Revision 1.19  1997/07/19 23:04:46  curt
# Added an initial weather section.
#
# Revision 1.18  1997/07/19 22:36:28  curt
# Added Utils/ subdirectory
#
# Revision 1.17  1997/07/18 23:41:20  curt
# Tweaks for building with Cygnus Win32 compiler.
#
# Revision 1.16  1997/07/07 20:59:47  curt
# Working on scenery transformations to enable us to fly fluidly over the
# poles with no discontinuity/distortion in scenery.
#
# Revision 1.15  1997/07/05 20:43:27  curt
# renamed mat3 directory to Math so we could add other math related routines.
#
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

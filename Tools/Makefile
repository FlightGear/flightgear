#---------------------------------------------------------------------------
# Toplevel FGTools Makefile
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


include make.inc


SUBDIRS = Dem2node Tri2obj Triangle


all: 
	for dir in $(SUBDIRS); do \
	    ( cd $$dir; $(MAKE) ) ; \
	done

depend:
	for dir in $(SUBDIRS); do \
	    ( echo "Making depend in $$dir"; \
	      cd $$dir; $(CC) -MM *.c > depend ) ; \
	done

Makefile-os2:
	cat Makefile | perl mkmfos2.pl > Makefile.os2; \
	for dir in $(SUBDIRS); do \
	    ( echo "Making Makefile.os2 in $$dir"; \
	      cat $$dir/Makefile | perl mkmfos2.pl > $$dir/Makefile.os2; \
	      cat $$dir/depend   | perl mkmfos2.pl > $$dir/depend.os2) ; \
	done

clean:
	-rm -f *.os2 *~
	for dir in $(SUBDIRS); do \
	    (cd $$dir; $(MAKE) clean) ; \
	done


source-tar: clean
	echo "need to fix this"
#	(cd ../..; \
#	tar cvzf source-$(VERSION).tar.gz FlightGear/fgtop FlightGear/COPYING \
#	FlightGear/Docs FlightGear/Src FlightGear/Thanks)

source-zip: clean
	echo "need to fix this"
#	(cd ../..; \
#	zip -r source-$(VERSION).zip FlightGear/fgtop FlightGear/COPYING \
#	FlightGear/Docs FlightGear/Src FlightGear/Thanks)

bin-tar: all
	echo "need to fix this"
#	cp GLUT/fg0 GLUT/runfg ..
#	(cd ../..; \
#	tar cvzf bin-$(VERSION).tar.gz FlightGear/fgtop FlightGear/fg0 \
#	FlightGear/runfg FlightGear/COPYING FlightGear/Docs FlightGear/Thanks)

bin-zip: 
	echo "need to fix this"
#	cp GLUT/fg0.exe GLUT/runfg.bat GLUT/cygwin.dll ..
#	(cd ../..; \
#	zip -r bin-$(VERSION).zip FlightGear/fgtop FlightGear/fg0.exe \
#	FlightGear/runfg.bat FlightGear/cygwin.dll FlightGear/COPYING \
#	FlightGear/Docs FlightGear/Thanks)


#---------------------------------------------------------------------------
# $Log$
# Revision 1.2  1997/11/14 00:31:06  curt
# Abandon Tri2terrain ... replace with Tri2obj so we can use an existing
# tri-stripper.
#
# Revision 1.1  1997/10/20 19:52:18  curt
# Initial revision.
#

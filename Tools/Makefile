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


include $(FG_ROOT_SRC)/commondefs


SUBDIRS = AssemTris Dem2node DemRaw2Ascii FixNode FixObj SplitTris \
	Stripe_u Tri2obj Triangle
ORDEREDDIRS = $(SUBDIRS)


all: 
	for dir in $(ORDEREDDIRS); do \
	    ( cd $$dir; $(MAKE) ) ; \
	done

depend:
	for dir in $(ORDEREDDIRS); do \
	    ( echo "Making depend in $$dir"; \
	      cd $$dir; $(CC) $(CFLAGS) -M *.c > depend ) ; \
	done

Makefile-os2:
	cat Makefile | perl mkmfos2.pl > Makefile.os2; \
	for dir in $(ORDEREDDIRS); do \
	    ( echo "Making Makefile.os2 in $$dir"; \
	      cat $$dir/Makefile | perl mkmfos2.pl > $$dir/Makefile.os2; \
	      cat $$dir/depend   | perl mkmfos2.pl > $$dir/depend.os2) ; \
	done

clean:
	-rm -f *.os2 *~
	for dir in $(ORDEREDDIRS); do \
	    (cd $$dir; $(MAKE) clean) ; \
	done


source-tar: clean
	(cd ..; \
	tar cvzf demtools-$(FG_VERSION).tar.gz Tools/Makefile Tools/README \
	Tools/Todo Tools/make.inc Tools/process-dem.pl Tools/AssemTris \
        Tools/Dem2node Tools/FixNode Tools/FixObj Tools/SplitTris \
	Tools/Stripe_u Tools/Tri2obj Tools/Triangle)

source-zip: clean
	(cd ..; \
	zip -r demtools-$(FG_VERSION).zip Tools/Makefile Tools/README \
	Tools/Todo Tools/make.inc Tools/process-dem.pl Tools/AssemTris \
        Tools/Dem2node Tools/FixNode Tools/FixObj Tools/SplitTris \
	Tools/Stripe_u Tools/Tri2obj Tools/Triangle)

bin-tar: all
	echo "need to fix this"
#	cp GLUT/fg0 GLUT/runfg ..
#	(cd ../..; \
#	tar cvzf bin-$(FG_VERSION).tar.gz FlightGear/fgtop FlightGear/fg0 \
#	FlightGear/runfg FlightGear/COPYING FlightGear/Docs FlightGear/Thanks)

bin-zip: 
	echo "need to fix this"
#	cp GLUT/fg0.exe GLUT/runfg.bat GLUT/cygwin.dll ..
#	(cd ../..; \
#	zip -r bin-$(FG_VERSION).zip FlightGear/fgtop FlightGear/fg0.exe \
#	FlightGear/runfg.bat FlightGear/cygwin.dll FlightGear/COPYING \
#	FlightGear/Docs FlightGear/Thanks)


#---------------------------------------------------------------------------
# $Log$
# Revision 1.10  1998/03/03 15:36:11  curt
# Tweaks for compiling with g++
#
# Revision 1.9  1998/03/03 01:21:17  curt
# Added DemRaw2Ascii
#
# Revision 1.8  1998/01/31 00:41:19  curt
# Made a few changes converting floats to doubles.
#
# Revision 1.7  1998/01/27 18:36:53  curt
# Lots of updates to get back in sync with changes made over in .../Src/
#
# Revision 1.6  1998/01/21 02:55:42  curt
# Incorporated new make system from Bob Kuehne <rpk@sgi.com>.
#
# Revision 1.5  1998/01/14 15:55:34  curt
# Finished splittris, started assemtris.
#
# Revision 1.4  1997/12/10 01:18:25  curt
# Initial revision.
#
# Revision 1.3  1997/11/27 00:18:26  curt
# Added FixNode
#
# Revision 1.2  1997/11/14 00:31:06  curt
# Abandon Tri2terrain ... replace with Tri2obj so we can use an existing
# tri-stripper.
#
# Revision 1.1  1997/10/20 19:52:18  curt
# Initial revision.
#

#---------------------------------------------------------------------------
# Toplevel Project Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


CC = gcc


SUBDIRS = aircraft controls flight scenery
MAIN = OpenGL


all:
	for dir in $(SUBDIRS) $(MAIN); do \
	    ( cd $$dir; make CC=$(CC) ) ; \
	done


install:
	for dir in $(SUBDIRS) $(MAIN); do \
	    ( cd $$dir; make CC=$(CC) install) ; \
	done


clean:
	-rm -f *~
	for dir in $(SUBDIRS) $(MAIN); do \
	    (cd $$dir; make clean) ; \
	done


#---------------------------------------------------------------------------
# $Log$
# Revision 1.2  1997/05/23 00:35:09  curt
# Trying to get fog to work ...
#
# Revision 1.1  1997/05/16 15:51:13  curt
# Initial revision.
#

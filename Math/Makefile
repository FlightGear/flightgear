#---------------------------------------------------------------------------
# Makefile
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


TARGET = libmat3.a

CFILES = MAT3geom.c MAT3inv.c MAT3mat.c MAT3vec.c
HFILES = mat3.h mat3defs.h mat3err.h
OFILES = $(CFILES:.c=.o)

CC = gcc
CFLAGS = -g -Wall
# CFLAGS = -O2 -Wall

AR = ar

INCLUDES = 

LIBS = 


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES) $(HFILES)
	$(AR) rv $(TARGET) $(OFILES)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------



#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/05/30 19:25:56  curt
# The MAT3 routines from SRGP.
#
# Revision 1.2  1997/05/23 15:40:29  curt
# Added GNU copyright headers.
#
# Revision 1.1  1997/05/16 15:58:23  curt
# Initial revision.
#

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


TARGET=fgfs0

CC = gcc

# STD_FLAGS = -O2 -Wall
STD_CFLAGS = -g -Wall


#---------------------------------------------------------------------------
# Define the high level GL interface library
#---------------------------------------------------------------------------

# For GLUT
INTERFACE_FLAGS = -DGLUT
INTERFACE_LIBS = -lglut
INTERFACE_FILES = GLUTkey.c

# For MESA_TK (Tk interface to Mesa)
# INCLUDES = -I/usr/include/mesa
# INTERFACE_FLAGS = -DMESA_TK
# INTERFACE_LIBS = -L/usr/lib/mesa -lMesatk
# INTERFACE_FILES = GLTKkey.c


#---------------------------------------------------------------------------
# Define main GL libraries
#---------------------------------------------------------------------------

# For OpenGL
# GRAPHICS_LIBS = -lGLU -lGL -lXmu -lX11

# For Mesa
MESA_LIBS = -L/usr/lib/mesa -lMesatk -lMesaaux -lMesaGLU -lMesaGL
X11_LIBS =  -L/usr/X11R6/lib -lXext -lXmu -lXi -lX11
GRAPHICS_LIBS = $(MESA_LIBS) $(X11_LIBS)


CFLAGS = $(STD_CFLAGS) $(INTERFACE_FLAGS)
LIBS =  $(INTERFACE_LIBS) $(GRAPHICS_LIBS) -lm -lfl

CFILES = GLmain.c $(INTERFACE_FILES) mesh2GL.c
OFILES = $(CFILES:.c=.o)
AFILES = ../Aircraft/libAircraft.a ../Controls/libControls.a \
	../Flight/libFlight.a ../Flight/LaRCsim/libLaRCsim.a \
	../Flight/Slew/libSlew.a ../mat3/libmat3.a ../Scenery/libScenery.a \
	../Timer/libTimer.a


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES) $(AFILES)
	$(CC) -o $(TARGET) $(OFILES) $(AFILES) $(LIBS)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

GLmain.o: GLmain.c GLUTkey.h ../Aircraft/aircraft.h ../Scenery/scenery.h \
	../Timer/fg_timer.h
	$(CC) $(CFLAGS) $(INCLUDES) -c GLmain.c

GLUTkey.o: GLUTkey.c GLUTkey.h ../Aircraft/aircraft.h
	$(CC) $(CFLAGS) $(INCLUDES) -c GLUTkey.c

GLTKkey.o: GLTKkey.c GLTKkey.h ../Aircraft/aircraft.h
	$(CC) $(CFLAGS) $(INCLUDES) -c GLTKkey.c

mesh2GL.o: mesh2GL.c ../Scenery/mesh.h
	$(CC) $(CFLAGS) $(INCLUDES) -c mesh2GL.c


#---------------------------------------------------------------------------
# $Log$
# Revision 1.13  1997/06/21 17:12:54  curt
# Capitalized subdirectory names.
#
# Revision 1.12  1997/06/16 19:32:51  curt
# Starting to add general timer support.
#
# Revision 1.11  1997/05/31 19:16:25  curt
# Elevator trim added.
#
# Revision 1.10  1997/05/31 04:13:53  curt
# WE CAN NOW FLY!!!
#
# Continuing work on the LaRCsim flight model integration.
# Added some MSFS-like keyboard input handling.
#
# Revision 1.9  1997/05/30 23:26:19  curt
# Added elevator/aileron controls.
#
# Revision 1.8  1997/05/30 19:27:02  curt
# The LaRCsim flight model is starting to look like it is working.
#
# Revision 1.7  1997/05/29 22:39:50  curt
# Working on incorporating the LaRCsim flight model.
#
# Revision 1.6  1997/05/29 02:33:23  curt
# Updated to reflect changing interfaces in other "modules."
#
# Revision 1.5  1997/05/27 17:44:32  curt
# Renamed & rearranged variables and routines.   Added some initial simple
# timer/alarm routines so the flight model can be updated on a regular interval.
#
# Revision 1.4  1997/05/23 15:40:26  curt
# Added GNU copyright headers.
# Fog now works!
#
# Revision 1.3  1997/05/23 00:35:13  curt
# Trying to get fog to work ...
#
# Revision 1.2  1997/05/21 15:57:52  curt
# Renamed due to added GLUT support.
#
# Revision 1.1  1997/05/16 16:05:51  curt
# Initial revision.
#

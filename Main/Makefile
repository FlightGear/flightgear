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


TARGET=fg0

CFILES = fg_init.c views.c $(INTERFACE_FILES)
OFILES = $(CFILES:.c=.o)
AFILES = ../Aircraft/libAircraft.a ../Cockpit/libCockpit.a \
	../Controls/libControls.a ../Flight/libFlight.a \
	../Joystick/libJoystick.a ../Flight/LaRCsim/libLaRCsim.a \
	../Flight/Slew/libSlew.a ../Scenery/libScenery.a \
	../Time/libTime.a ../Weather/libWeather.a ../Math/libMath.a


include ../make.inc


CFLAGS = $(FG_CFLAGS) $(INTERFACE_FLAGS)
LIBS =  $(INTERFACE_LIBS) $(GRAPHICS_LIBS) -lm -lfl


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES) $(AFILES)
	$(CC) -o $(TARGET) $(OFILES) $(AFILES) $(LIBS)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) $(TARGET).exe lib*.a *.os2 *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

include depend

GLUTmain.o:
	$(CC) $(CFLAGS) -c GLUTmain.c -o $@

GLUTkey.o:
	$(CC) $(CFLAGS) -c GLUTkey.c -o $@

GLTKkey.o:
	$(CC) $(CFLAGS) -c GLTKkey.c -o $@

fg_init.o:
	$(CC) $(CFLAGS) -c fg_init.c -o $@

views.o:
	$(CC) $(CFLAGS) -c views.c -o $@


#---------------------------------------------------------------------------
# $Log$
# Revision 1.34  1997/10/25 03:24:21  curt
# Incorporated sun, moon, and star positioning code contributed by Durk Talsma.
#
# Revision 1.33  1997/09/04 02:17:35  curt
# Shufflin' stuff.
#
# Revision 1.32  1997/08/27 21:31:27  curt
# Added views.[ch]
#
# Revision 1.31  1997/08/25 20:27:23  curt
# Merged in initial HUD and Joystick code.
#
# Revision 1.30  1997/08/22 21:34:40  curt
# Doing a bit of reorganizing and house cleaning.
#
# Revision 1.29  1997/08/04 20:25:15  curt
# Organizational tweaking.
#
# Revision 1.28  1997/08/02 18:45:01  curt
# Renamed GLmain.c GLUTmain.c
#
# Revision 1.27  1997/07/31 22:52:39  curt
# Working on redoing internal coordinate systems & scenery transformations.
#
# Revision 1.26  1997/07/30 16:12:42  curt
# Moved fg_random routines from Util/ to Math/
#
# Revision 1.25  1997/07/20 02:19:11  curt
# First stab at a system to generate os2 makefiles automatically.
#
# Revision 1.24  1997/07/19 23:04:47  curt
# Added an initial weather section.
#
# Revision 1.23  1997/07/19 22:34:03  curt
# Moved PI definitions to ../constants.h
# Moved random() stuff to ../Utils/ and renamed fg_random()
#
# Revision 1.22  1997/07/18 23:41:25  curt
# Tweaks for building with Cygnus Win32 compiler.
#
# Revision 1.21  1997/07/05 20:43:34  curt
# renamed mat3 directory to Math so we could add other math related routines.
#
# Revision 1.20  1997/06/29 21:19:17  curt
# Working on scenery management system.
#
# Revision 1.19  1997/06/27 21:38:09  curt
# Working on Makefile structure.
#
# Revision 1.18  1997/06/27 20:03:36  curt
# Working on Makefile structure.
#
# Revision 1.17  1997/06/26 22:14:54  curt
# Beginning work on a scenery management system.
#
# Revision 1.16  1997/06/26 19:08:33  curt
# Restructuring make, adding automatic "make dep" support.
#
# Revision 1.15  1997/06/25 15:39:47  curt
# Minor changes to compile with rsxnt/win32.
#
# Revision 1.14  1997/06/21 17:52:23  curt
# Continue directory shuffling ... everything should be compilable/runnable
# again.
#
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

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

TARGET = fg-$(FG_VERSION)

CFILES = fg_init.c fg_debug.c views.c $(INTERFACE_FILES)

FGLIBS = -lAircraft -lAstro -lCockpit -lControls -lFlight \
	-lJoystick -lLaRCsim -lSlew -lScenery -lTime -lWeather -lMath \
	$(NULL)


LCDEFS = -DGLUT
LLDFLAGS =
LDLIBS = $(FGLIBS) $(FG_DEBUG_LIBS) \
	$(INTERFACE_LIBS) $(GRAPHICS_LIBS) -lm

include $(FG_ROOT_SRC)/commondefs

#---------------------------------------------------------------------------
# Rule for TARGET
#---------------------------------------------------------------------------

$(TARGET)$(EXT): $(OBJECTS) $(FG_ROOT_LIB)/stamp_libs
	$(CC) -o $(TARGET)$(EXT) $(OBJECTS) $(LDFLAGS) $(LDLIBS)
	$(LN) $(TARGET)$(EXT) fg$(FG_VERSION_MAJOR)$(EXT)

include $(COMMONRULES)

#---------------------------------------------------------------------------
# $Log$
# Revision 1.45  1998/01/27 00:47:57  curt
# Incorporated Paul Bleisch's <bleisch@chromatic.com> new debug message
# system and commandline/config file processing code.
#
# Revision 1.44  1998/01/22 22:04:00  curt
# Tweaked extention stuff.
#
# Revision 1.43  1998/01/22 02:59:37  curt
# Changed #ifdef FILE_H to #ifdef _FILE_H
#
# Revision 1.42  1998/01/21 21:11:34  curt
# Misc. tweaks.
#
# Revision 1.41  1998/01/19 19:27:08  curt
# Merged in make system changes from Bob Kuehne <rpk@sgi.com>
# This should simplify things tremendously.
#
# Revision 1.1  1998/01/07 03:16:15  curt
# Moved from .../Src/Scenery/ to .../Src/Astro/
#
# Revision 1.26  1997/12/19 16:45:01  curt
# Working on scene rendering order and options.
#
# Revision 1.25  1997/12/17 23:13:45  curt
# Began working on rendering the sky.
#
# Revision 1.24  1997/11/25 19:25:33  curt
# Changes to integrate Durk's moon/sun code updates + clean up.
#
# Revision 1.23  1997/10/28 21:00:20  curt
# Changing to new terrain format.
#
# Revision 1.22  1997/10/25 03:30:07  curt
# Misc. tweaks.
#
# Revision 1.21  1997/10/25 03:18:26  curt
# Incorporated sun, moon, and planet position and rendering code contributed
# by Durk Talsma.
#
# Revision 1.20  1997/09/22 14:44:21  curt
# Continuing to try to align stars correctly.
#
# Revision 1.19  1997/08/27 03:30:23  curt
# Changed naming scheme of basic shared structures.
#
# Revision 1.18  1997/08/02 19:10:12  curt
# Incorporated mesh2GL.c into mesh.c
#
# Revision 1.17  1997/07/23 21:52:23  curt
# Put comments around the text after an #endif for increased portability.
#
# Revision 1.16  1997/07/20 02:19:11  curt
# First stab at a system to generate os2 makefiles automatically.
#
# Revision 1.15  1997/07/12 02:24:47  curt
# Added ranlib.
#
# Revision 1.14  1997/06/29 21:16:47  curt
# More twiddling with the Scenery Management system.
#
# Revision 1.13  1997/06/27 21:38:10  curt
# Working on Makefile structure.
#
# Revision 1.12  1997/06/27 20:03:37  curt
# Working on Makefile structure.
#
# Revision 1.11  1997/06/26 22:14:57  curt
# Beginning work on a scenery management system.
#
# Revision 1.10  1997/06/26 19:08:34  curt
# Restructuring make, adding automatic "make dep" support.
#
# Revision 1.9  1997/06/25 15:39:48  curt
# Minor changes to compile with rsxnt/win32.
#
# Revision 1.8  1997/06/21 17:58:07  curt
# directory shuffling ...
#
# Revision 1.1  1997/06/21 17:39:28  curt
# Moved to the ParseScn subdirectory.
#
# Revision 1.6  1997/06/21 17:12:55  curt
# Capitalized subdirectory names.
#
# Revision 1.5  1997/05/31 19:16:29  curt
# Elevator trim added.
#
# Revision 1.4  1997/05/27 17:48:50  curt
# Added -f flag to flex to generate a "fast" scanner.
#
# Revision 1.3  1997/05/23 15:40:40  curt
# Added GNU copyright headers.
#
# Revision 1.2  1997/05/17 00:17:01  curt
# Cosmetic changes.
#
# Revision 1.1  1997/05/16 16:07:02  curt
# Initial revision.
#

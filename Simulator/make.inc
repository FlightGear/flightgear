# Hey Emacs, this is a Makefile. -*- Mode: Makefile -*-
#
# Common Makefile section
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


#---------------------------------------------------------------------------
# Define the version
#---------------------------------------------------------------------------

FG_VERSION_MAJOR = 0
FG_VERSION_MINOR = 19
FG_VERSION = $(FG_VERSION_MAJOR).$(FG_VERSION_MINOR)


#---------------------------------------------------------------------------
# Choose your weapons
#---------------------------------------------------------------------------

CC = gcc
FLEX = flex -f -L
BISON = bison -v --no-lines
AR = ar
RANLIB = ranlib
RM = rm
LN = ln -sf
TAR = tar


#---------------------------------------------------------------------------
# Global Compile Options
#
# You may set FG_CFLAGS to include any of the following options depending on 
# your environment:
#
# -g           - Compile with debugging symbols
#
# -Wall        - Enable full compiler warnings
#
# -O2          - Enable compiler optimization
#
# -O3 -fomit-frame-pointer -funroll-all-loops -ffast-math
#              - Other potential compiler optimization flags
#
# -
#---------------------------------------------------------------------------

GLOBAL_CFLAGS = -g -Wall -DVERSION=\"$(FG_VERSION)\"


#---------------------------------------------------------------------------
# Platform specific compile options, these should be set with FG_CFLAGS
# below.  These have been predefined for the supported platforms below.
#
# -DNO_PRINTF -  Disable all printf()'s.  Works by replacing the printf
#                fuction with an empty function.
#
# -DUSE_ITIMER - Use setitimer(), getitimer(), and signal() to mimic
#                a real time system and call the flight model routines
#                at a regular interval, rather than between screen updates
#                which can be highly variable.  This can make the flight
#                model calculations much smoother.
#
# -DUSE_FTIME -  Use ftime() to get an accurate current time instead of
#                gettimeofday()
#
# -DUSE_RAND  -  Use rand() instead of random()
#
#---------------------------------------------------------------------------


#---------------------------------------------------------------------------
# Debugging options.
#
# Uncomment the following two lines to enable OpenGL function call tracing.
# This mechanism is provided courtesy of Steve Baker <sbaker@link.com>
#---------------------------------------------------------------------------

# FG_DEBUG_FLAGS = -DXGL_TRACE
# FG_DEBUG_LIBS = ../XGL/libXGL.a


#---------------------------------------------------------------------------
# Uncomment one of the following sections depending on your system
#
# You may set FG_GRAPHICS to include any of the following options depending
# on your environment:
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Linux/Mesa with the GLUT toolkit
#
INTERFACE_FLAGS = -DGLUT
INTERFACE_LIBS = -lglut
INTERFACE_FILES = GLUTmain.c GLUTkey.c
MESA_LIBS = -L/usr/lib/mesa -lMesatk -lMesaaux -lMesaGLU -lMesaGL
X11_LIBS =  -L/usr/X11R6/lib -lXext -lXmu -lXi -lX11
GRAPHICS_LIBS = $(MESA_LIBS) $(X11_LIBS)
FG_CFLAGS = $(GLOBAL_CFLAGS) $(FG_DEBUG_FLAGS)
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# SGI IRIX with the GLUT toolkit 
#     (Surprisingly, this also works on our SunOS 4.x machine with the 
#     way we have Mesa & Glut installed.)
#
# INTERFACE_FLAGS = -DGLUT
# INTERFACE_LIBS = -lglut
# INTERFACE_FILES = GLUTmain.c GLUTkey.c
# GRAPHICS_LIBS = -lGLU -lGL -lXmu -lX11
# FG_CFLAGS = $(GLOBAL_CFLAGS) $(FG_DEBUG_FLAGS)
# TAR = gtar
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Sun/Solaris with the GLUT toolkit
#
# VERSION=\"$(VERSION)\"
# INTERFACE_FLAGS = -DGLUT
# INTERFACE_LIBS = -lglut
# INTERFACE_FILES = GLUTmain.c GLUTkey.c
# GRAPHICS_LIBS =  -L/opt/X11R6/lib -lGLU -lGL -lXext -lXmu -lXi -lX11 -lsocket
# FG_CFLAGS = $(GLOBAL_CFLAGS) $(FG_DEBUG_FLAGS)
# TAR = gtar
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Cygnus Win32 (gcc based) with a static version of the GLUT toolkit
#
# INTERFACE_FLAGS = -DGLUT 
# INTERFACE_LIBS = ../Win32/libglut.a
# INTERFACE_FILES = GLUTmain.c GLUTkey.c
# GRAPHICS_LIBS = -lglu32 -lopengl32 -luser32 -lgdi32
# FG_CFLAGS = $(GLOBAL_CFLAGS) $(FG_DEBUG_FLAGS) -DWIN32 -DUSE_RAND
# LN = ren
#---------------------------------------------------------------------------


#---------------------------------------------------------------------------
# $Log$
# Revision 1.26  1997/12/17 23:13:18  curt
# Changes and tweaks to go to version 0.19 (Plus tweaks to Solaris support)
#
# Revision 1.25  1997/12/15 23:54:26  curt
# Add xgl wrappers for debugging.
# Generate terrain normals on the fly.
#
# Revision 1.24  1997/12/12 21:41:24  curt
# More light/material property tweaking ... still a ways off.
#
# Revision 1.23  1997/12/12 19:52:32  curt
# Solaris tweaks.
#
# Revision 1.22  1997/12/11 04:43:53  curt
# Fixed sun vector and lighting problems.  I thing the moon is now lit
# correctly.
#
# Revision 1.21  1997/12/10 01:19:42  curt
# Tweaks for verion 0.15 release.
#
# Revision 1.20  1997/12/09 04:25:25  curt
# Working on adding a global lighting params structure.
#
# Revision 1.19  1997/11/25 19:25:27  curt
# Changes to integrate Durk's moon/sun code updates + clean up.
#
# Revision 1.18  1997/10/28 19:35:41  curt
# Organizational changes.
#
# Revision 1.17  1997/10/25 03:30:02  curt
# Misc. tweaks.
#
# Revision 1.16  1997/10/25 03:24:21  curt
# Incorporated sun, moon, and star positioning code contributed by Durk Talsma.
#
# Revision 1.15  1997/09/23 00:29:27  curt
# Tweaks to get things to compile with gcc-win32.
#
# Revision 1.14  1997/09/16 22:14:47  curt
# Tweaked time of day lighting equations.  Don't draw stars during the day.
#
# Revision 1.13  1997/09/04 02:17:19  curt
# Shufflin' stuff.
#
# Revision 1.12  1997/08/25 20:27:21  curt
# Merged in initial HUD and Joystick code.
#
# Revision 1.11  1997/08/22 21:34:33  curt
# Doing a bit of reorganizing and house cleaning.
#
# Revision 1.10  1997/08/16 12:22:19  curt
# Tweaks for new version.
#
# Revision 1.9  1997/07/19 22:41:35  curt
# tweaks
#
# Revision 1.8  1997/07/18 23:41:20  curt
# Tweaks for building with Cygnus Win32 compiler.
#
# Revision 1.7  1997/07/18 14:28:34  curt
# Hacked in some support for wind/turbulence.
#
# Revision 1.6  1997/07/16 20:04:42  curt
# Minor tweaks to aid Win32 port.
#
# Revision 1.5  1997/07/12 02:25:53  curt
# Added ranlib.
#
# Revision 1.4  1997/07/12 02:13:04  curt
# Add ftime() support for those that don't have gettimeofday()
#
# Revision 1.3  1997/07/09 21:31:08  curt
# Working on making the ground "hard."
#
# Revision 1.2  1997/07/07 20:59:48  curt
# Working on scenery transformations to enable us to fly fluidly over the
# poles with no discontinuity/distortion in scenery.
#
# Revision 1.1  1997/06/27 21:38:00  curt
# Working on Makefile structure.
#

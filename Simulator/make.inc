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


VERSION = 0.12

#---------------------------------------------------------------------------
# Choose your weapons
#---------------------------------------------------------------------------

CC = gcc
FLEX = flex -f -L
BISON = bison -v --no-lines
AR = ar
RANLIB = ranlib


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
#---------------------------------------------------------------------------

GLOBAL_CFLAGS = -g -Wall -DVERSION=\"$(VERSION)\"


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
# Uncomment one of the following sections depending on your system
#
# You may set FG_GRAPHICS to include any of the following options depending
# on your environment:
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
# FG_CFLAGS = $(GLOBAL_CFLAGS)
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
FG_CFLAGS = $(GLOBAL_CFLAGS)
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Cygnus Win32 (gcc based) with a static version of the GLUT toolkit
#
# INTERFACE_FLAGS = -DGLUT 
# INTERFACE_LIBS = ../Win32/libglut.a
# INTERFACE_FILES = GLUTmain.c GLUTkey.c
# GRAPHICS_LIBS = -lglu32 -lopengl32 -luser32 -lgdi32
# FG_CFLAGS = $(GLOBAL_CFLAGS) -DWIN32 -DUSE_RAND
#---------------------------------------------------------------------------


#---------------------------------------------------------------------------
# $Log$
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

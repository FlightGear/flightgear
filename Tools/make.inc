# Hey Emacs, this is a Makefile. -*- Mode: Makefile -*-
#
# Common FGTools Makefile section
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


VERSION = 0.02

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
# Revision 1.2  1997/12/10 01:18:26  curt
# Initial revision.
#
# Revision 1.1  1997/10/20 19:52:18  curt
# Initial revision.
#

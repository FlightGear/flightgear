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
# Choose your weapons
#---------------------------------------------------------------------------

CC = gcc
FLEX = flex -f -L
BISON = bison -v --no-lines
AR = ar
RANLIB = echo


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
# -DUSE_ITIMER - Use setitimer(), getitimer(), and signal() to mimic
#                a real time system and call the flight model routines
#                at a regular interval, rather than between screen updates
#                which can be highly variable.  This can make the flight
#                model calculations much smoother.
#
# -DUSE_FTIME -  Use ftime() to get an accurate current time instead of
#                gettimeofday()
#
#---------------------------------------------------------------------------

FG_CFLAGS = -g -Wall


#---------------------------------------------------------------------------
# Uncomment one of the following sections depending on your system
#
# You may set FG_GRAPHICS to include any of the following options depending
# on your environment:
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# SGI IRIX with the GLUT toolkit (surprisingly, this also works on our 
#                                 SunOS 4.x machine with the way we have
#                                 Mesa & Glut installed.)
#
# INTERFACE_FLAGS = -DGLUT
# INTERFACE_LIBS = -lglut
# INTERFACE_FILES = GLUTkey.c
# GRAPHICS_LIBS = -lGLU -lGL -lXmu -lX11
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Linux/Mesa with the GLUT toolkit
#
INTERFACE_FLAGS = -DGLUT
INTERFACE_LIBS = -lglut
INTERFACE_FILES = GLUTkey.c
MESA_LIBS = -L/usr/lib/mesa -lMesatk -lMesaaux -lMesaGLU -lMesaGL
X11_LIBS =  -L/usr/X11R6/lib -lXext -lXmu -lXi -lX11
GRAPHICS_LIBS = $(MESA_LIBS) $(X11_LIBS)
#---------------------------------------------------------------------------

#---------------------------------------------------------------------------
# Windows 95/NT with the GLUT toolkit
#
# INTERFACE_FLAGS = -DGLUT
# INTERFACE_LIBS = -lglut
# INTERFACE_FILES = GLUTkey.c
# GRAPHICS_LIBS = -lGLU -lGL
#---------------------------------------------------------------------------


#---------------------------------------------------------------------------
# $Log$
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

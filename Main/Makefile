#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET=viewer

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
GRAPHICS_LIBS = -lGLU -lGL -lXmu -lX11

# For Mesa
# MESA_LIBS = -L/usr/lib/mesa -lMesaaux -lMesatk -lMesaGLU -lMesaGL
# X11_LIBS =  -L/usr/X11R6/lib -lXext -lXmu -lXi -lX11
# GRAPHICS_LIBS = $(MESA_LIBS) $(X11_LIBS)


CFLAGS = $(STD_CFLAGS) $(INTERFACE_FLAGS)
LIBS =  $(INTERFACE_LIBS) $(GRAPHICS_LIBS) -lm -lfl

CFILES = GLmain.c $(INTERFACE_FILES) mesh2GL.c
OFILES = $(CFILES:.c=.o)
AFILES = ../flight/libflight.a ../aircraft/libaircraft.a ../scenery/libscenery.a



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

GLmain.o: GLmain.c GLUTkey.h ../aircraft/aircraft.h ../scenery/scenery.h
	$(CC) $(CFLAGS) $(INCLUDES) -c GLmain.c

GLUTkey.o: GLUTkey.c GLUTkey.h ../aircraft/aircraft.h
	$(CC) $(CFLAGS) $(INCLUDES) -c GLUTkey.c

GLTKkey.o: GLTKkey.c GLTKkey.h ../aircraft/aircraft.h
	$(CC) $(CFLAGS) $(INCLUDES) -c GLTKkey.c

mesh2GL.o: mesh2GL.c ../scenery/mesh.h
	$(CC) $(CFLAGS) $(INCLUDES) -c mesh2GL.c


#---------------------------------------------------------------------------
# $Log$
# Revision 1.2  1997/05/21 15:57:52  curt
# Renamed due to added GLUT support.
#
# Revision 1.1  1997/05/16 16:05:51  curt
# Initial revision.
#

#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET=viewer

CFILES = gltkmain.c gltkkey.c mesh2ogl.c
OFILES = $(CFILES:.c=.o)
AFILES = ../flight/libflight.a ../aircraft/libaircraft.a ../scenery/libscenery.a

CC = gcc
CFLAGS = -g -Wall
# CFLAGS = -O2 -Wall

INCLUDES = -I/usr/include/mesa

MESA_LIBS = -L/usr/lib/mesa -lMesaaux -lMesatk -lMesaGLU -lMesaGL
X11_LIBS =  -L/usr/X11R6/lib -lX11 -lXext
LIBS =  $(MESA_LIBS) $(X11_LIBS) -lm -lfl



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

gltkmain.o: gltkmain.c gltkkey.h ../aircraft/aircraft.h ../scenery/scenery.h
	$(CC) $(CFLAGS) $(INCLUDES) -c gltkmain.c

gltkkey.o: gltkkey.c gltkkey.h ../aircraft/aircraft.h
	$(CC) $(CFLAGS) $(INCLUDES) -c gltkkey.c

mesh2ogl.o: mesh2ogl.c ../scenery/mesh.h
	$(CC) $(CFLAGS) $(INCLUDES) -c mesh2ogl.c


#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/05/16 16:05:51  curt
# Initial revision.
#

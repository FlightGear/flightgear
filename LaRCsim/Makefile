#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET=libLaRCsim.a

LaRCsimFILES = atmos_62.c ls_accel.c ls_aux.c ls_geodesy.c ls_gravity.c \
	ls_step.c ls_model.c default_model_routines.c ls_init.c ls_sync.c \

NavionFILES = navion_aero.c navion_engine.c navion_gear.c navion_init.c

InterfaceFILES = ls_interface.c

CFILES =  $(LaRCsimFILES) $(NavionFILES) $(InterfaceFILES)

OFILES = $(CFILES:.c=.o)

CC = gcc
CFLAGS = -g
# CFLAGS = -O2

AR = ar

INCLUDES = 

LIBS = -lm


#---------------------------------------------------------------------------
# Primary Targets
#---------------------------------------------------------------------------

$(TARGET): $(OFILES)
	$(AR) rv $(TARGET) $(OFILES)

simtest: $(TARGET) LaRCsim.o
	$(CC) -o simtest LaRCsim.o libLaRCsim.a $(LIBS)

all: $(TARGET)

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------



#---------------------------------------------------------------------------
# $Log$
# Revision 1.1  1997/05/29 00:09:52  curt
# Initial Flight Gear revision.
#
# Revision 1.1  1997/05/16 16:04:44  curt
# Initial revision.
#

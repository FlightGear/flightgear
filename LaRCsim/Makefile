#---------------------------------------------------------------------------
# Makefile
#
# Written by Curtis Olson, started May 1997.
#
# $Id$
# (Log is kept at end of this file)
#---------------------------------------------------------------------------


TARGET = libLaRCsim.a

LaRCsimFILES = atmos_62.c ls_accel.c ls_aux.c ls_geodesy.c ls_gravity.c \
	ls_step.c ls_model.c default_model_routines.c ls_init.c ls_sync.c

NavionFILES = navion_aero.c navion_engine.c navion_gear.c navion_init.c

InterfaceFILES = ls_interface.c

CFILES =  $(LaRCsimFILES) $(NavionFILES) $(InterfaceFILES)

OFILES = $(CFILES:.c=.o)

CC = gcc
CFLAGS = $(FG_CFLAGS)

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

# depend:
# 	$(CC) -MM *.c > depend

clean:
	rm -f *.o $(TARGET) *~ core


#---------------------------------------------------------------------------
# Secondary Targets
#---------------------------------------------------------------------------

include depend


#---------------------------------------------------------------------------
# $Log$
# Revision 1.5  1997/06/27 20:03:34  curt
# Working on Makefile structure.
#
# Revision 1.4  1997/06/26 19:08:30  curt
# Restructuring make, adding automatic "make dep" support.
#
# Revision 1.3  1997/06/21 17:12:50  curt
# Capitalized subdirectory names.
#
# Revision 1.2  1997/06/17 16:52:02  curt
# Timer interval stuff now uses gettimeofday() instead of ftime()
#
# Revision 1.1  1997/05/29 00:09:52  curt
# Initial Flight Gear revision.
#
# Revision 1.1  1997/05/16 16:04:44  curt
# Initial revision.
#

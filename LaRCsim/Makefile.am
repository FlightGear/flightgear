libdir	= ${exec_prefix}/lib

LaRCsimFILES = atmos_62.c ls_accel.c ls_aux.c ls_geodesy.c ls_gravity.c \
        ls_step.c ls_matrix.c ls_model.c default_model_routines.c ls_init.c
#	ls_trim.c

NavionFILES = navion_aero.c navion_engine.c navion_gear.c navion_init.c
InterfaceFILES = ls_interface.c

lib_LTLIBRARIES = libLaRCsim.la
libLaRCsim_la_SOURCES = \
	$(LaRCsimFILES) \
	$(NavionFILES) \
	$(InterfaceFILES)

INCLUDES += -I../..

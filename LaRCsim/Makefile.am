libdir	= ${exec_prefix}/lib

lib_LTLIBRARIES = libLaRCsim.la

libLaRCsim_la_SOURCES = \
	atmos_62.c atmos_62.h \
	default_model_routines.c default_model_routines.h \
	ls_accel.c ls_accel.h \
	ls_aux.c ls_aux.h \
	ls_cockpit.h ls_constants.h ls_generic.h \
	ls_geodesy.c ls_geodesy.h \
	ls_gravity.c ls_gravity.h \
	ls_init.c ls_init.h \
	ls_matrix.c ls_matrix.h \
	ls_model.c ls_model.h \
	ls_sim_control.h \
        ls_step.c ls_step.h \
	ls_sym.h ls_types.h \
	navion_aero.c navion_engine.c navion_gear.c \
	navion_init.c navion_init.h \
	ls_interface.c ls_interface.h

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Simulator

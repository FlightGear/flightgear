EXTRA_DIST = c172_aero.c c172_engine.c c172_gear.c c172_init.c navion_init.h

if ENABLE_C172
AIRCRAFT_MODEL = c172_aero.c c172_engine.c c172_gear.c c172_init.c navion_init.h
else
AIRCRAFT_MODEL = navion_aero.c navion_engine.c navion_gear.c navion_init.c navion_init.h
endif

# AIRCRAFT_MODEL = cherokee_aero.c cherokee_engine.c cherokee_gear.c cherokee_init.c navion_init.h

noinst_LIBRARIES = libLaRCsim.a

libLaRCsim_a_SOURCES = \
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
	$(AIRCRAFT_MODEL) \
	ls_interface.c ls_interface.h

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib -I$(top_builddir)/Simulator

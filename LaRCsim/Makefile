ARLIBRARY = libLaRCsim.a
TARGETS = $(ARLIBRARY)

LaRCsimFILES = atmos_62.c ls_accel.c ls_aux.c ls_geodesy.c ls_gravity.c \
        ls_step.c ls_model.c default_model_routines.c ls_init.c # ls_sync.c
NavionFILES = navion_aero.c navion_engine.c navion_gear.c navion_init.c
InterfaceFILES = ls_interface.c
CFILES =  $(LaRCsimFILES) $(NavionFILES) $(InterfaceFILES)
CXXFILES = 

LDIRT = $(FG_ROOT_LIB)/$(ARLIBRARY)

include $(FG_ROOT_SRC)/commondefs

include $(COMMONRULES)

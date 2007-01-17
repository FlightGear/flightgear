# Link the yasim executable against the individual object files rather
# than libYASim.  The library references other stuff in FlightGear,
# and some linkers (Irix) do dependency checks per-library instead of
# per-object.
#
# I think that it's permissible to list the same source files more
# than once in a Makefile.am.  Hopefully this doesn't break anything.

SHARED_SOURCE_FILES = \
        Airplane.cpp Airplane.hpp \
        Atmosphere.cpp Atmosphere.hpp \
        BodyEnvironment.hpp \
        ControlMap.cpp ControlMap.hpp \
        FGFDM.cpp FGFDM.hpp \
        Gear.cpp Gear.hpp \
        Glue.cpp Glue.hpp \
        Ground.cpp Ground.hpp \
        Hitch.cpp Hitch.hpp \
        Hook.cpp Hook.hpp \
        Launchbar.cpp Launchbar.hpp \
        Integrator.cpp Integrator.hpp \
        Jet.cpp Jet.hpp \
        Math.cpp Math.hpp \
        Model.cpp Model.hpp \
        PropEngine.cpp PropEngine.hpp \
        Propeller.cpp Propeller.hpp \
        Engine.hpp \
        PistonEngine.cpp PistonEngine.hpp \
        TurbineEngine.cpp TurbineEngine.hpp \
        RigidBody.cpp RigidBody.hpp \
        Rotor.cpp Rotor.hpp \
        Rotorpart.cpp Rotorpart.hpp \
        SimpleJet.cpp SimpleJet.hpp \
        Surface.cpp Surface.hpp \
        Thruster.cpp Thruster.hpp \
        Vector.hpp \
        Wing.cpp Wing.hpp \
        Turbulence.cpp Turbulence.hpp

noinst_LIBRARIES = libYASim.a

libYASim_a_SOURCES = YASim.cxx YASim.hxx FGGround.cpp FGGround.hpp $(SHARED_SOURCE_FILES)

bin_PROGRAMS = yasim
noinst_PROGRAMS = proptest

yasim_SOURCES = yasim-test.cpp $(SHARED_SOURCE_FILES)

yasim_LDADD = -lsgxml -lsgprops -lsgdebug -lsgmisc -lsgstructure $(base_LIBS)

proptest_SOURCES = proptest.cpp $(SHARED_SOURCE_FILES)
proptest_LDADD = -lsgxml -lsgprops -lsgdebug -lsgmisc -lsgstructure $(base_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src

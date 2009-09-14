if ENABLE_PLIB_JOYSTICK
DEFS += -DENABLE_PLIB_JOYSTICK
else
DEFS += -DENABLE_GLUT_JOYSTICK
endif

noinst_LIBRARIES = libJoystick.a

libJoystick_a_SOURCES = joystick.cxx joystick.hxx

INCLUDES += -I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Lib/plib/include \
	-I$(top_builddir)/Simulator

noinst_LIBRARIES = libJoystick.a

libJoystick_a_SOURCES = joystick.cxx joystick.hxx

noinst_PROGRAMS = js_demo

js_demo_SOURCES = js_demo.cxx

js_demo_LDADD = $(audio_LIBS)

INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src

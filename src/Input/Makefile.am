AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkgdatadir)\"

noinst_LIBRARIES = libInput.a

libInput_a_SOURCES = input.cxx input.hxx

bin_PROGRAMS = js_demo fgjs

js_demo_SOURCES = js_demo.cxx

js_demo_LDADD = -lplibjs $(base_LIBS) $(joystick_LIBS) -lplibul

fgjs_SOURCES = fgjs.cxx jsinput.cxx jsinput.h jssuper.cxx jssuper.h

fgjs_LDADD = -lplibjs $(base_LIBS) $(joystick_LIBS) -lplibul \
	-lsgprops -lsgmisc -lsgio -lsgdebug -lsgstructure -lsgxml -lz

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/src/Main

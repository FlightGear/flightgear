noinst_LIBRARIES = libInput.a

libInput_a_SOURCES = input.cxx input.hxx

bin_PROGRAMS = js_demo fgjs

js_demo_SOURCES = js_demo.cxx

js_demo_LDADD = -lplibjs $(base_LIBS) $(audio_LIBS) -lplibul

fgjs_SOURCES = fgjs.cxx jsinput.cxx jsinput.h jssuper.cxx jssuper.h

fgjs_LDADD = -lplibjs $(base_LIBS) $(audio_LIBS) -lplibul

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src

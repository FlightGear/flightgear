noinst_LIBRARIES = libInput.a

libInput_a_SOURCES = input.cxx input.hxx

bin_PROGRAMS = js_demo fgjs

js_demo_SOURCES = js_demo.cxx

js_demo_LDADD = $(audio_LIBS)

fgjs_SOURCES = fgjs.cxx jsinput.cxx jssuper.cxx

fgjs_LDADD = $(audio_LIBS)

INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src

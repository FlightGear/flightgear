noinst_LIBRARIES = libAutopilot.a

libAutopilot_a_SOURCES = \
	auto_gui.cxx auto_gui.hxx \
	newauto.cxx newauto.hxx

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif

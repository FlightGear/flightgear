noinst_LIBRARIES = libNetworkOLK.a

libNetworkOLK_a_SOURCES = \
	net_send.cxx net_hud.cxx network.cxx network.h fgd.h

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif

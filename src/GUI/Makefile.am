noinst_LIBRARIES = libGUI.a

if ENABLE_NETWORK_OLK
NETWORK_SRCS = net_dlg.cxx net_dlg.hxx 
else
NETWORK_SRCS = 
endif

libGUI_a_SOURCES = \
	apt_dlg.cxx apt_dlg.hxx \
	gui.cxx gui.h \
	gui_local.cxx gui_local.hxx \
	mouse.cxx \
	$(NETWORK_SRCS) \
	prop_picker.cxx prop_picker.hxx \
	sgVec3Slider.cxx sgVec3Slider.hxx \
	trackball.c trackball.h

INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src

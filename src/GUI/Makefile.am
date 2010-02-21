noinst_LIBRARIES = libGUI.a
noinst_PROGRAMS = layout-test

if HAVE_FRAMEWORK_PLIB
layout_test_PLIB_FW = $(plib_FRAMEWORK)
else
layout_test_PLIB_LIBS = -lplibpw -lplibpu -lplibfnt -lplibul
endif

if HAVE_FRAMEWORK_OSG
layout_test_OSG_LIBS = $(openthreads_FRAMEWORK)
else
layout_test_OSG_LIBS = $(openthreads_LIBS)
endif

libGUI_a_SOURCES = \
        new_gui.cxx new_gui.hxx \
        dialog.cxx dialog.hxx \
        menubar.cxx menubar.hxx \
        gui.cxx gui.h gui_funcs.cxx \
        fonts.cxx \
        AirportList.cxx AirportList.hxx \
        property_list.cxx property_list.hxx \
        layout.cxx layout-props.cxx layout.hxx \
        SafeTexFont.cxx SafeTexFont.hxx \
        WaypointList.cxx WaypointList.hxx
        
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src

layout_test_SOURCES = layout-test.cxx

layout_test_LDADD = libGUI.a \
                    -lsgprops -lsgdebug -lsgstructure -lsgmisc -lsgxml -lsgtiming \
                    $(layout_test_PLIB_LIBS) $(opengl_LIBS)

# mainly for Macs
layout_test_LDFLAGS = $(layout_test_PLIB_FW) $(layout_test_OSG_LIBS)

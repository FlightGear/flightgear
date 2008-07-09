noinst_LIBRARIES = libGUI.a
noinst_PROGRAMS = layout-test

libGUI_a_SOURCES = \
        new_gui.cxx new_gui.hxx \
        dialog.cxx dialog.hxx \
	menubar.cxx menubar.hxx \
	gui.cxx gui.h gui_funcs.cxx \
	mouse.cxx fonts.cxx \
	AirportList.cxx AirportList.hxx \
        property_list.cxx property_list.hxx \
        layout.cxx layout-props.cxx layout.hxx \
	SafeTexFont.cxx SafeTexFont.hxx

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src

layout_test_SOURCES = layout-test.cxx

layout_test_LDADD = libGUI.a \
                    -lsgprops -lsgdebug -lsgstructure -lsgmisc -lsgxml \
                    -lplibpw -lplibpu -lplibfnt -lplibul $(opengl_LIBS)

noinst_LIBRARIES = libGUI.a
noinst_PROGRAMS = layout-test

libGUI_a_SOURCES = \
        new_gui.cxx new_gui.hxx \
        dialog.cxx dialog.hxx \
	menubar.cxx menubar.hxx \
	gui.cxx gui.h gui_funcs.cxx \
	gui_local.cxx gui_local.hxx \
	mouse.cxx fonts.cxx \
	preset_dlg.cxx preset_dlg.hxx \
	prop_picker.cxx prop_picker.hxx \
	sgVec3Slider.cxx sgVec3Slider.hxx \
	trackball.c trackball.h \
	puList.cxx puList.hxx \
	AirportList.cxx AirportList.hxx \
        layout.cxx layout-props.cxx layout.hxx

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src

layout_test_SOURCES = layout-test.cxx

layout_test_LDADD = libGUI.a \
                    -lsgprops -lsgdebug -lsgstructure -lsgmisc -lsgxml \
                    -lplibpw -lplibpu -lplibfnt -lplibul $(opengl_LIBS)

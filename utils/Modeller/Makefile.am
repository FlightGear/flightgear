noinst_PROGRAMS = threedconvert animassist normalmap

threedconvert_SOURCES = 3dconvert.cxx
threedconvert_LDADD = -lplibssg -lplibsg -lplibul \
			$(opengl_LIBS) $(joystick_LIBS)

animassist_SOURCES = animassist.c
animassist_LDADD = $(base_LIBS)

normalmap_SOURCES = normalmap.cxx
normalmap_LDADD = -lsgscreen $(opengl_LIBS) $(base_LIBS) -lz

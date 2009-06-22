noinst_PROGRAMS = animassist normalmap photomodel

# threedconvert_SOURCES = 3dconvert.cxx
# threedconvert_LDADD = -lplibssg -lplibsg -lplibul \
# 			$(opengl_LIBS) $(joystick_LIBS)

animassist_SOURCES = animassist.c
animassist_LDADD = $(base_LIBS)

normalmap_SOURCES = normalmap.cxx texture.cxx texture.hxx colours.h
normalmap_LDADD = -lsgscreen $(opengl_LIBS) $(base_LIBS) -lz

photomodel_SOURCES = photomodel.cxx
photomodel_LDADD = $(base_LIBS) -lsgbucket -lsgmath -lsgmisc -lsgdebug \
  		    -lsgstructure

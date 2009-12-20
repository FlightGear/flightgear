bin_PROGRAMS = est-epsilon gl-info alcinfo

noinst_PROGRAMS = test-gethostname test-mktime test-text test-up test-env-map

if HAVE_FRAMEWORK_PLIB
test_up_LDFLAGS = $(plib_FRAMEWORK)
else
test_up_PLIB_LIBS = -lplibsg -lplibul
endif

est_epsilon_SOURCES = est-epsilon.c
est_epsilon_LDADD =  $(opengl_LIBS)

gl_info_SOURCES = gl-info.c
gl_info_LDADD = $(opengl_LIBS)

alcinfo_SOURCES = alcinfo.cxx
alcinfo_LDADD = $(openal_LIBS)

test_env_map_SOURCES = test-env-map.cxx
test_env_map_LDADD = $(opengl_LIBS)

test_gethostname_SOURCES = test-gethostname.cxx

test_mktime_SOURCES = test-mktime.cxx

test_text_SOURCES = test-text.cxx

test_up_SOURCES = test-up.cxx
test_up_LDADD = $(test_up_PLIB_LIBS) -lsgmath -lsgxml -lsgmisc -lsgdebug -lsgstructure -lsgtiming -lz $(base_LIBS)

bin_PROGRAMS = est-epsilon gl-info

noinst_PROGRAMS = test-gethostname test-mktime test-text test-up test-env-map

est_epsilon_SOURCES = est-epsilon.c

gl_info_SOURCES = gl-info.c
gl_info_LDADD = $(opengl_LIBS)

test_env_map_SOURCES = test-env-map.cxx
test_env_map_LDADD = $(opengl_LIBS)

test_gethostname_SOURCES = test-gethostname.cxx

test_mktime_SOURCES = test-mktime.cxx

test_text_SOURCES = test-text.cxx

test_up_SOURCES = test-up.cxx
test_up_LDADD = -lsgmath -lsgxml -lsgmisc -lsgdebug -lsgstructure -lsgtiming -lplibsg -lplibul -lz $(base_LIBS)

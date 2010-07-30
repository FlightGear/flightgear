bin_PROGRAMS = est-epsilon gl-info alcinfo

noinst_PROGRAMS = test-gethostname test-mktime test-text test-env-map

if HAVE_FRAMEWORK_PLIB
test_up_LDFLAGS = $(plib_FRAMEWORK)
else
test_up_PLIB_LIBS = -lplibsg -lplibul
endif

est_epsilon_SOURCES = est-epsilon.cxx
est_epsilon_LDADD =  $(opengl_LIBS)

gl_info_SOURCES = gl-info.cxx
gl_info_LDADD = $(opengl_LIBS)

alcinfo_SOURCES = alcinfo.cxx
alcinfo_LDADD = $(openal_LIBS)

test_env_map_SOURCES = test-env-map.cxx
test_env_map_LDADD = $(opengl_LIBS)

test_gethostname_SOURCES = test-gethostname.cxx

test_mktime_SOURCES = test-mktime.cxx

test_text_SOURCES = test-text.cxx


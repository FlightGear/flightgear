if HAVE_DAYLIGHT
DEFS += -DHAVE_DAYLIGHT
endif

if HAVE_TIMEZONE
DEFS += -DHAVE_TIMEZONE
endif

bin_PROGRAMS = est-epsilon gl-info test-mktime

est_epsilon_SOURCES = est-epsilon.c
est_epsilon_LDADD = $(base_LIBS)

gl_info_SOURCES = gl-info.c
gl_info_LDADD = $(opengl_LIBS) $(base_LIBS)

test_mktime_SOURCES = test-mktime.cxx
test_mktime_LDADD = $(base_LIBS)
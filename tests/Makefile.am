bin_PROGRAMS = est-epsilon gl-info test-gethostname test-mktime test-text \
	test-up

est_epsilon_SOURCES = est-epsilon.c
est_epsilon_LDADD = $(base_LIBS)

gl_info_SOURCES = gl-info.c
gl_info_LDADD = $(opengl_LIBS)

test_gethostname_SOURCES = test-gethostname.cxx
test_gethostname_LDADD = $(base_LIBS)

test_mktime_SOURCES = test-mktime.cxx
test_mktime_LDADD = $(base_LIBS)

test_text_SOURCES = test-text.cxx
test_text_LDADD = $(base_LIBS)

test_up_SOURCES = test-up.cxx
test_up_LDADD = -lsgmath -lsgdebug -lplibsg $(base_LIBS)

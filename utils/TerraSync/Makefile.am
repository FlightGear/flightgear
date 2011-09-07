EXTRA_DIST = README.txt

bin_PROGRAMS = terrasync

terrasync_SOURCES = terrasync.cxx

AM_CPPFLAGS = $(svn_CPPFLAGS)

terrasync_LDADD = -lsgio -lsgstructure -lsgmisc -lsgdebug -lsgthreads $(network_LIBS) $(svn_LIBS)

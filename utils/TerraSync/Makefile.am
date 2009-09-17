EXTRA_DIST = README.txt

bin_PROGRAMS = terrasync

terrasync_SOURCES = terrasync.cxx

AM_CPPFLAGS = $(svn_CPPFLAGS)

if HAVE_FRAMEWORK_PLIB
terrasync_LDFLAGS = $(plib_FRAMEWORK)
else
terrasync_PLIB_LIBS = -lplibnet -lplibul  
endif

terrasync_LDADD = $(terrasync_PLIB_LIBS) -lsgmisc -lsgdebug $(network_LIBS) $(svn_LIBS)

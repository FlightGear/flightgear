noinst_HEADERS = xml_cache.h xml.h

noinst_LIBRARIES = libxmlconf.a

libxmlconf_a_SOURCES = xml_cache.c xml.c

noinst_PROGRAMS = testxml printxml xmlgrep

testxml_SOURCES = testxml.c
testxml_LDADD = libxmlconf.a

printxml_SOURCES = $(xml_SOURCE) printxml.c
printxml_LDADD = libxmlconf.a

xmlgrep_SOURCES = $(xml_SOURCE) xmlgrep.c xml.c xml.h
xmlgrep_LDADD = libxmlconf.a

INCLUDES = -DSRC_DIR=\"$(top_srcdir)/utils/xmlgrep\" -DXML_USE_NODECACHE

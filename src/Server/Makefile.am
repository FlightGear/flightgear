noinst_LIBRARIES = libServer.a
noinst_PROGRAMS = buftest msgtest

if ENABLE_MSERVER_AS

MSERVER_AS = \
	msg_0001_hello.cxx \
	msg_0001_hello.hxx \
	messagebuf.cxx \
	messagebuf.hxx \
	message.cxx \
	message.hxx

MSERVER_BUF = buftest.cxx

MSERVER_MSG = msgtest.cxx
	
else

MSERVER_AS =

MSERVER_BUF = 

MSERVER_MSG = 

endif

libServer_a_SOURCES = $(MSERVER_AS) 

buftest_SOURCES = $(MSERVER_BUF)
buftest_LDADD = libServer.a

msgtest_SOURCES = $(MSERVER_MSG)
msgtest_LDADD = libServer.a

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src

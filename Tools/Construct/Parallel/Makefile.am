bin_PROGRAMS = server client

server_SOURCES = server.cxx

server_LDADD = 

client_SOURCES = client.cxx

client_LDADD = \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Misc/libMisc.a

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Tools/Lib \
	-I$(top_builddir)/Tools/Construct

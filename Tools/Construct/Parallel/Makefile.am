bin_PROGRAMS = fgfs-tools-server fgfs-tools-client

fgfs_tools_server_SOURCES = server.cxx

fgfs_tools_server_LDADD = \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Misc/libMisc.a

fgfs_tools_client_SOURCES = client.cxx

fgfs_tools_client_LDADD = \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Misc/libMisc.a

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Tools/Lib \
	-I$(top_builddir)/Tools/Construct

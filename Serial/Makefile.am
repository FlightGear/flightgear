bin_PROGRAMS = testserial

noinst_LIBRARIES = libSerial.a

libSerial_a_SOURCES = serial.cxx serial.hxx

testserial_SOURCES = testserial.cxx

testserial_LDADD = \
	$(top_builddir)/Lib/Serial/libSerial.a \
	$(top_builddir)/Lib/Debug/libDebug.a

INCLUDES += -I$(top_builddir) -I$(top_builddir)/Lib

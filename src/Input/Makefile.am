AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkgdatadir)\"

noinst_LIBRARIES = libInput.a

if WITH_EVENTINPUT
libInput_Event_SOURCES = FGEventInput.cxx FGEventInput.hxx FGLinuxEventInput.cxx FGLinuxEventInput.hxx
libInput_Event_INCLUDES = -I/usr/include/dbus-1.0 -I/usr/lib64/dbus-1.0/include -I/usr/lib/dbus-1.0/include 
else
libInput_Event_SOURCES = 
libInput_Event_INCLUDES = 
endif

libInput_a_SOURCES = input.cxx input.hxx FGCommonInput.cxx FGCommonInput.hxx \
	FGDeviceConfigurationMap.cxx FGDeviceConfigurationMap.hxx \
	FGButton.cxx FGButton.hxx \
	FGMouseInput.cxx FGMouseInput.hxx \
	FGKeyboardInput.cxx FGKeyboardInput.hxx \
	FGJoystickInput.cxx FGJoystickInput.hxx \
        $(libInput_Event_SOURCES)

bin_PROGRAMS = js_demo fgjs

js_demo_SOURCES = js_demo.cxx

js_demo_LDADD = -lplibjs $(base_LIBS) $(joystick_LIBS) -lplibul

fgjs_SOURCES = fgjs.cxx jsinput.cxx jsinput.h jssuper.cxx jssuper.h

fgjs_LDADD = -lplibjs -lplibul $(base_LIBS) $(joystick_LIBS) \
	-lsgprops -lsgmisc -lsgio -lsgdebug -lsgstructure -lsgxml -lz

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/src/Main \
	$(libInput_Event_INCLUDES)

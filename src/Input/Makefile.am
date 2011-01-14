AM_CXXFLAGS = -DPKGLIBDIR=\"$(pkgdatadir)\"

noinst_LIBRARIES = libInput.a

if WITH_EVENTINPUT
EXTRA_libInput_a_SOURCES = FGLinuxEventInput.cxx FGLinuxEventInput.hxx \
	FGMacOSXEventInput.cxx FGMacOSXEventInput.hxx
libInput_Event_SOURCES = FGEventInput.cxx FGEventInput.hxx 
libInput_Event_INCLUDES = $(eventinput_INCLUDES)
libInput_a_LIBADD = $(eventinput_EXTRA_OBJS)
libInput_a_DEPENDENCIES = $(eventinput_EXTRA_OBJS)
else
libInput_Event_SOURCES = 
libInput_Event_INCLUDES = 
endif

if HAVE_FRAMEWORK_PLIB
js_demo_LDFLAGS = $(plib_FRAMEWORK)
fgjs_LDFLAGS = $(plib_FRAMEWORK) 
else
js_demo_LDFLAGS =
fgjs_LDFLAGS=
endif

if HAVE_FRAMEWORK_OSG
fgjs_LDFLAGS = $(openthreads_FRAMEWORK)
else
fgjs_LDFLAGS = $(openthreads_LIBS)
endif

libInput_a_SOURCES = input.cxx input.hxx FGCommonInput.cxx FGCommonInput.hxx \
	FGDeviceConfigurationMap.cxx FGDeviceConfigurationMap.hxx \
	FGButton.cxx FGButton.hxx \
	FGMouseInput.cxx FGMouseInput.hxx \
	FGKeyboardInput.cxx FGKeyboardInput.hxx \
	FGJoystickInput.cxx FGJoystickInput.hxx \
	js.cxx jsBSD.cxx jsLinux.cxx jsMacOSX.cxx jsNone.cxx FGjs.hxx \
        $(libInput_Event_SOURCES)

bin_PROGRAMS = js_demo fgjs

js_demo_SOURCES = js_demo.cxx 

js_demo_LDADD = $(base_LIBS) -L. -lInput -lplibul

fgjs_SOURCES = fgjs.cxx jsinput.cxx jsinput.h jssuper.cxx jssuper.h

fgjs_LDADD = $(base_LIBS) $(joystick_LIBS) -L. -lInput -lplibul \
	-lsgprops -lsgmisc -lsgio -lsgdebug -lsgstructure -lsgxml -lz $(network_LIBS)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/src/Main \
	$(libInput_Event_INCLUDES)

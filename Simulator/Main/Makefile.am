if ENABLE_AUDIO_SUPPORT
DEFS += -DENABLE_AUDIO_SUPPORT
AUDIO_LIBS = -lsl -lsm
endif

if ENABLE_IRIX_AUDIO
LIBS += -laudio
endif

if ENABLE_WIN32_AUDIO
LIBS += -lwinmm
endif

if ENABLE_LINUX_JOYSTICK
DEFS += -DENABLE_LINUX_JOYSTICK
else
DEFS += -DENABLE_GLUT_JOYSTICK
endif

if ENABLE_XMESA_FX
DEFS += -DXMESA -DFX
endif

if ENABLE_UNIX_SERIAL
SERIAL_LIBS = $(top_builddir)/Lib/Serial/libSerial.a
else
SERIAL_LIBS =
endif

EXTRA_DIST = 3dfx.sh runfgfs.in runfgfs.bat.in

bin_PROGRAMS = fgfs

bin_SCRIPTS = runfgfs runfgfs.bat

fgfs_SOURCES = \
	GLUTkey.cxx GLUTkey.hxx GLUTmain.cxx \
	fg_init.cxx fg_init.hxx \
	fg_serial.cxx fg_serial.hxx \
	options.cxx options.hxx \
	splash.cxx splash.hxx \
	views.cxx views.hxx

fgfs_LDADD = \
	$(top_builddir)/Simulator/Aircraft/libAircraft.a \
	$(top_builddir)/Simulator/Astro/libAstro.a \
	$(top_builddir)/Simulator/Autopilot/libAutopilot.a \
	$(top_builddir)/Simulator/Cockpit/libCockpit.a \
	$(top_builddir)/Simulator/Controls/libControls.a \
	$(top_builddir)/Simulator/FDM/libFlight.a \
	$(top_builddir)/Simulator/FDM/External/libExternal.a \
	$(top_builddir)/Simulator/FDM/JSBsim/libJSBsim.a \
	$(top_builddir)/Simulator/FDM/LaRCsim/libLaRCsim.a \
	$(top_builddir)/Simulator/FDM/Slew/libSlew.a \
	$(top_builddir)/Simulator/GUI/libGUI.a \
	$(top_builddir)/Simulator/Scenery/libScenery.a \
	$(top_builddir)/Simulator/Airports/libAirports.a \
	$(top_builddir)/Simulator/Objects/libObjects.a \
	$(top_builddir)/Simulator/Time/libTime.a \
	$(top_builddir)/Simulator/Weather/libWeather.a \
	$(top_builddir)/Simulator/Joystick/libJoystick.a \
	$(AUDIO_LIBS) \
	$(SERIAL_LIBS) \
	$(top_builddir)/Lib/Math/libMath.a \
	$(top_builddir)/Lib/Bucket/libBucket.a \
	$(top_builddir)/Lib/Debug/libDebug.a \
	-lpu -lfnt \
	$(top_builddir)/Lib/zlib/libz.a \
	$(top_builddir)/Lib/Misc/libMisc.a \
	$(opengl_LIBS)

INCLUDES += \
	-I$(top_builddir) \
	-I$(top_builddir)/Lib \
	-I$(top_builddir)/Lib/plib/include \
	-I$(top_builddir)/Simulator

if ENABLE_WEATHERCM
WEATHER_DIR = WeatherCM
else
WEATHER_DIR = Environment
endif

if HAVE_PLIB_PSL
SCRIPTING_DIRS = Scripting
else
SCRIPTING_DIRS =
endif

if ENABLE_MPLAYER_AS
MPLAYER_DIRS = MultiPlayer
else
MPLAYER_DIRS =
endif

SUBDIRS = \
        Include \
        Aircraft \
        Airports \
        ATC \
        Autopilot \
        Cockpit \
        Controls \
        FDM \
        GUI \
        Input \
	Instrumentation \
        Model \
        Navaids \
        Network \
	$(MPLAYER_DIRS) \
        Objects \
	Replay \
        Scenery \
        $(SCRIPTING_DIRS) \
        Sound \
        Systems \
        Time \
        $(WEATHER_DIR) \
        Main

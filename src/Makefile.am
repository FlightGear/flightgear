if ENABLE_WEATHERCM
WEATHER_DIR = WeatherCM
else
WEATHER_DIR = Environment
endif

if ENABLE_NETWORK_OLK
NETWORK_DIRS = Network NetworkOLK
else
NETWORK_DIRS = Network
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
        $(NETWORK_DIRS) \
        Objects \
        Scenery \
        Sound \
        Systems \
        Time \
        $(WEATHER_DIR) \
        Main

if ENABLE_NEW_ENVIRONMENT
WEATHER_DIR = Environment
else
WEATHER_DIR = WeatherCM
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
        Model \
	Navaids \
        $(NETWORK_DIRS) \
        Objects \
        Scenery \
	Sound \
        Time \
        $(WEATHER_DIR) \
        Main

if ENABLE_OLD_WEATHER
WEATHER_DIR = Weather
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
        Autopilot \
        Cockpit \
        Controls \
        FDM \
        GUI \
        Joystick \
	Navaids \
        $(NETWORK_DIRS) \
        Objects \
        Scenery \
	Sound \
        Time \
        $(WEATHER_DIR) \
        Main

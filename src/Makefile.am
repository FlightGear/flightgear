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
        Astro \
        Autopilot \
        Clouds \
        Cockpit \
        Controls \
        FDM \
        GUI \
        Joystick \
        $(NETWORK_DIRS) \
        Objects \
        Scenery \
	Sky \
        Time \
        $(WEATHER_DIR) \
        Main

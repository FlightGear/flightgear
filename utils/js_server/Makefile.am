noinst_PROGRAMS = js_server

if HAVE_FRAMEWORK_PLIB
js_server_LDFLAGS = $(plib_FRAMEWORK)
else
js_server_PLIB_LIBS = -lplibjs -lplibnet -lplibul  
endif

js_server_SOURCES = js_server.cxx
js_server_LDADD = $(js_server_PLIB_LIBS) $(base_LIBS) $(joystick_LIBS) $(network_LIBS)

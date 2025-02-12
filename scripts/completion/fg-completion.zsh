#compdef fgfs

local _fgfs_root
local _fgfs_options

local state


_fgfs_root=${FG_ROOT:-/usr/local/share/FlightGear}


_fgfs_options=(
	'--addon=[Specify a path to addon. Multiple instances can be used]:Directories:_directories' \
	'--adf1=[Set the ADF1 radio frequency, optionally preceded by a card rotation]' \
	'--adf2=[Set the ADF2 radio frequency, optionally preceded by a card rotation]' \
	'--aero=[Select aircraft aerodynamics model to load]' \
	'--ai-models[Enable/disable the artifical traffic subsystem]:Boolean values:(true false 1 0 yes no)' \
		'--disable-ai-models[Disable the artifical traffic subsystem]' \
		'--enable-ai-models[Enable the artifical traffic]' \
	'--aircraft=[Select an aircraft profile]:Aircraft:->aircraft' \
	'--vehicle=[Select an aircraft profile]:Aircraft:->vehicle' \
	'--aircraft-dir=[Aircraft directory relative to the path of the executable]:Aircraft directory:_directories' \
	'--airport=[Specify starting position relative to an airport]:Airport:->airport' \
	'--ai-scenario=[Add and enable a new scenario]:AI scenario:->ai-scenario' \
	'--allow-nasal-from-sockets[Enable/disable allowing executing Nasal scripts from sockets]:Boolean values:(true false 1 0 yes no)' \
	'--enable-allow-nasal-from-sockets[Enable allowing executing Nasal scripts from sockets]' \
	'--disable-allow-nasal-from-sockets[Disable allowing executing Nasal scripts from sockets]' \
	'--allow-nasal-read=[Allow Nasal scripts to read files from directories]:Directories:_directories' \
	'--altitude=[Starting altitude]' \
	'--anti-alias-hud[Enable/disable anti-aliased HUD]:Boolean values:(true false 1 0 yes no)' \
		'--disable-anti-alias-hud[Disable anti-aliased HUD]' \
		'--enable-anti-alias-hud[Enable anti-aliased HUD]' \
	'--aspect-ratio-multiplier=[Specify a multiplier for the aspect ratio]' \
	'--atcsim=[Open connection using the ATC sim protocol (atc610x)]' \
	'--atlas=[Open connection using the Atlas protocol]' \
	'--auto-coordination[Enable/disable auto coordination]:Boolean values:(true false 1 0 yes no)' \
		'--enable-auto-coordination[Enable auto coordination]' \
		'--disable-auto-coordination[Disable auto coordination]' \
	'--AV400=[Emit the Garmin AV400 protocol]' \
	'--AV400Sim=[Emit the set of AV400 strings]' \
	'--AV400WSimA=[Open connection for "A" channel using Garmin WAAS GPS protocol]' \
	'--AV400WSimB=[Open connection for "B" channel using Garmin WAAS GPS protocol]' \
	'--bpp=[Specify the bits per pixel]:Bits per pixel:(16 24 32)' \
	'--browser-app=[Specify path to your web browser]:Directories:_directories' \
	'--callsign=[Assign a unique name to a player]' \
	'--carrier=[Specify starting position on an AI carrier]:AI carrier:(Antonio Clemenceau Eisenhower Foch Kuznetsov Liaoning Nimitz San Truman Vinson)' \
	'--carrier-position=[Specify which starting position on an AI carrier]:Park position:->parkpos' \
	'--ceiling=[Create an overcast ceiling, optionally with a specific thickness]' \
	'--clock-freeze[Enable/disable clock freeze]:Boolean values:(true false 1 0 yes no)' \
		'--disable-clock-freeze[Clock advances normally]' \
		'--enable-clock-freeze[Do not advance clock]' \
	'--clouds[Enable/disable 2D (flat) cloud layers]:Boolean values:(true false 1 0 yes no)' \
		'--enable-clouds[Enable 2D (flat) cloud layers]' \
		'--disable-clouds[Disable 2D (flat) cloud layers]' \
	'--clouds3d[Enable/disable 3D (volumetric) cloud layers]:Boolean values:(true false 1 0 yes no)' \
		'--enable-clouds3d[Enable 3D (volumetric) cloud layers]' \
		'--disable-clouds3d[Disable 3D (volumetric) cloud layers]' \
	'--com1=[Set the COM1 radio frequency]' \
	'--com2=[Set the COM2 radio frequency]' \
	'--compositor=[Specify the path to XML file for multi-pass rendering]' \
		'--config=[Load additional properties from file]:Directories:_files' \
		'--data=[Specify an additional base data directory (FGData)]:Directories:_directories' \
	'--developer[Enable/Disable developer mode]:Boolean values:(true false 1 0 yes no)' \
		'--enable-developer[Enable developer mode]' \
		'--disable-developer[Disable developer mode]' \
	'--distance-attenuation[Enable/disable runway light distance attenuation]:Boolean values:(true false 1 0 yes no)' \
		'--disable-distance-attenuation[Disable runway light distance attenuation]' \
		'--enable-distance-attenuation[Enable runway light distance attenuation]' \
	'--dme=[Slave the ADF to one of the NAV radios, or set its internal frequency]' \
	'--download-dir=[Store aircraft and scenery downloaded via the simulator in path]:Directories:_directories' \
	'--failure=[Fail the pitot, static, vacuum, or electrical system]:Failure system:(pitot static vaccum electical)'
	'--fdm=[Select the core flight dynamics model]:Core flight dynamics model:(jsb yasim magic balloon ada external)' \
	'--fg-aircraft=[Specify additional aircraft directory path(s)]:Directories:_directories' \
	'--fgcom[Enable/disable built-in FGCom]:Boolean values:(true false 1 0 yes no)' \
		'--enable-fgcom[Enable built-in FGCom]' \
		'--disable-fgcom[Disable built-in FGCom]' \
	'--fg-root=[Specify the root data path]:Directories:_directories' \
	'--fg-scenery=[Specify the base scenery path]:Directories:_directories' \
	'--fgviewer[Use a model viewer rather than load the entire simulator]' \
	'--fix=[Specify starting position relative to a fix]:FIX:->fix' \
	'--flarm=[Open connection using the Flarm protocol]' \
	'--flight-plan=[Read all waypoints from a file]:Waypoints file:_files' \
	'--fog-disable[Disable fog/haze]' \
	'--fog-fastest[Enable fastest fog/haze]' \
	'--fog-nicest[Enable nicest fog/haze]' \
	'--fov=[Specify field of view angle]' \
	'--fpe[Enable/Disable aborting on encountering a floating point exception]:Boolean values:(true false 1 0 yes no)' \
		'--enable-fpe[Enable aborting on encountering a floating point exception]' \
		'--disable-fpe[Disable aborting on encountering a floating point exception]' \
	'--freeze[Enable/disable simulation freeze]:Boolean values:(true false 1 0 yes no)' \
		'--disable-freeze[Start in a running state]' \
		'--enable-freeze[Start in a frozen state]' \
	'--fuel-freeze[Enable/disable fuel freeze]:Boolean values:(true false 1 0 yes no)' \
		'--disable-fuel-freeze[Fuel is consumed normally]' \
		'--enable-fuel-freeze[Fuel tank quantity forced to remain constant]' \
	'--fullscreen[Enable/disable fullscreen mode]:Boolean values:(true false 1 0 yes no)' \
		'--disable-fullscreen[Disable fullscreen mode]' \
		'--enable-fullscreen[Enable fullscreen mode]' \
	'--garmin=[Open connection using the Garmin GPS protocol]' \
	'--generic=[Open connection using a predefined communication interface]' \
	'--geometry=[Specify window geometry (640x480, etc)]' \
	'--glideslope=[Specify flight path angle (in degrees)]' \
	'--graphics-preset=[Specify a graphic preset]:Graphic presets:(minimal-quality low-quality medium-quality high-quality ultra-quality)' \
	'--gui[Enable/disable GUI (disabling GUI enables headless mode)]:Boolean values:(true false 1 0 yes no)' \
		'--enable-gui[Enable GUI]' \
		'--disable-gui[Disable GUI (disabling GUI enables headless mode)]' \
	'--heading=[Specify heading (yaw) angle (Psi)]' \
	'--roll=[Specify roll angle (Phi)]' \
	'--pitch=[Specify pitch angle (Theta)]' \
	'(-h --help)'{-h,--help}'[Show the most relevant command line options]' \
	'--hold-short[Enable/disable move to hold short in MP]:Boolean values:(true false 1 0 yes no)' \
		'--enable-hold-short[Enable move to hold short in MP]' \
		'--disable-hold-short[Disable move to hold short in MP]' \
	'--horizon-effect[Enable/Disable celestial body growth illusion near the horizon]:Boolean values:(true false 1 0 yes no)' \
		'--disable-horizon-effect[Disable celestial body growth illusion near the horizon]' \
		'--enable-horizon-effect[Enable celestial body growth illusion near the horizon]' \
	'--httpd=[Enable http server on the specified port]' \
	'--hud[Enable/disable Heads Up Display (HUD)]:Boolean values:(true false 1 0 yes no)' \
		'--disable-hud[Disable Heads Up Display (HUD)]' \
		'--enable-hud[Enable Heads Up Display (HUD)]' \
	'--hud-3d[Enable/disable 3D HUD]:Boolean values:(true false 1 0 yes no)' \
		'--disable-hud-3d[Disable 3D HUD]' \
		'--enable-hud-3d[Enable 3D HUD]' \
	'--hud-culled[Hud displays percentage of triangles culled]' \
	'--hud-tris[Hud displays number of triangles rendered]' \
	'--igc=[Open connection using the International Gliding Commission (IGC) protocol]' \
	'--ignore-autosave[Enable/disable ignoring the autosave file]:Boolean values:(true false 1 0 yes no)' \
		'--enable-ignore-autosave[Enable ignoring the autosave file]' \
		'--disable-ignore-autosave[Disable ignoring the autosave file]' \
	'--in-air[Start in air (implied when using --altitude)]' \
	'--on-ground[Start at ground level (default)]' \
	'--joyclient=[Open connection to an Agwagon joystick]' \
	'--jpg-httpd=[Enable screen shot http server on the specified port]' \
	'--jsbsim-output-directive-file=[Log JSBSim properties]:File:_files' \
	'--jsclient=[Open connection to a remote joystick]' \
	'--json-report[Enable/Disable printing a report in JSON format]:Boolean values:(true false 1 0 yes no)' \
	'--language=[Select the language for this session]:Language:(ca de en es fr it nl pl pt ru tr sk zh)' \
	'--lat=[Starting latitude (in degrees)]' \
	'--lon=[Starting longitude (in degrees)]' \
	'--launcher[Enable/disable Qt Launcher]:Boolean values:(true false 1 0 yes no)' \
		'--enable-launcher[Enable Qt Launcher]' \
		'--disable-launcher[Disable Qt Launcher]' \
	'--livery=[Select aircraft livery]' \
	'--load-tape=[Load recording of earlier flightgear session]' \
	'--load-tape-create-video[Enable/disable encode video while replaying tape]:Boolean values:(true false 1 0 yes no)' \
		'--enable-load-tape-create-video[Enable encode video while replaying tape]' \
		'--disable-load-tape-create-video[Disable encode video while replaying tape]' \
	'--load-tape-fixed-dt=[Set fixed-dt mode while replaying tape specified by --load-tape]' \
	'--lod-levels=[Specify the detail levels]' \
	'--lod-range-mult=[Specify the range multiplier]' \
	'--lod-res=[Specify the resolution of the terrain grid]' \
	'--lod-texturing=[Specify the method of texturing the terrain]:LOD texturing:(bluemarble raster debug)' \
	'--log-class=[Specify which logging class(es) to use]:Log class:(none ai aircraft astro atc autopilot clipper cockpit environment event flight general gl gui headless input instrumentation io math nasal navaid network osg particles sound systems terrain terrasync undefined view all)' \
	'--log-dir=[Save the logs in the given directory]:Directories:_directories' \
	'--log-level=[Specify which loggin level to use]:Log level:(bulk debug info warn alert)' \
	'--mach=[Specify initial mach number]' \
	'--materials-file=[Specify the materials file used to render the scenery]' \
	'--max-fps=[Maximum frame rate in Hz]' \
	'--metar=[Pass a METAR string to set up static weather]' \
	'--min-status=[Allows you to define a minimum status level for all listed aircraft]:Minimum status level:(alpha beta early-production production)' \
	'--model-hz=[Run the FDM this rate (iterations per second)]' \
	'--mouse-pointer[Enable/disable extra mouse pointer]:Boolean values:(true false 1 0 yes no)' \
		'--disable-mouse-pointer[Disable extra mouse pointer]' \
		'--enable-mouse-pointer[Enable extra mouse pointer]' \
	'--multiplay=[Specify multipilot communication settings ({in|out},hz,address,port)]' \
	'--native=[Open connection using the FG Native protocol]' \
	'--native-ctrls=[Open connection using the FG Native Controls protocol]' \
	'--native-fdm=[Open connection using the FG Native FDM protocol]' \
	'--native-gui=[Open connection using the FG Native GUI protocol]' \
	'--nav1=[Set the NAV1 radio frequency, optionally preceded by a radial]' \
	'--nav2=[Set the NAV2 radio frequency, optionally preceded by a radial]' \
	'--ndb=[Specify starting position relative to an NDB]:NDB:->ndb' \
	'--nmea=[Open connection using the NMEA protocol]' \
	'--no-default-config[Enable/Disable not loading any default config files unless explicitly specified with --config]:Boolean values:(true false 1 0 yes no)' \
	'--offset-azimuth=[Specify heading to reference point (in degrees)]' \
	'--offset-distance=[Specify distance to reference point (in miles)]' \
	'--opengc=[Open connection using the OpenGC protocol]' \
	'--panel[Enable/disable instrument panel]:Boolean values:(true false 1 0 yes no)' \
		'--disable-panel[Disable instrument panel]' \
		'--enable-panel[Enable instrument panel]' \
	'--parking-id=[Specify parking position]' \
	'--parkpos=[Specify parking position]' \
	'--prop\:[Set the given property to the given value]' \
	'--prop\:browser=[Open the properties dialog immediately on the given property]' \
	'--props=[Open connection using the interactive property manager]' \
	'--proxy=[Specify which proxy server (and port) to use (user:pwd@host:port)]' \
	'--pve=[Open connection using the PVE protocol]' \
	'--random-objects[Enable/disable random scenery objects (buildings, etc.)]:Boolean values:(true false 1 0 yes no)' \
		'--disable-random-objects[Exclude random scenery objects (buildings, etc.)]' \
		'--enable-random-objects[Include random scenery objects (buildings, etc.)]' \
	'--random-wind[Set up random wind direction and speed]' \
	'--ray=[Open connection using the Ray Woodworth motion chair protocol]' \
	'--read-only[Enable/disable folder $FG_HOME read-only]:Boolean values:(true false 1 0 yes no)' \
		'--enable-read-only[Enable folder $FG_HOME read-only]' \
		'--disable-read-only[Disable folder $FG_HOME read-only]' \
	'--real-weather-fetch[Enable/disbale METAR based real weather fetching]:Boolean values:(true false 1 0 yes no)' \
		'--disable-real-weather-fetch[Disbale METAR based real weather fetching]' \
		'--enable-real-weather-fetch[Enable METAR based real weather fetching]' \
	'--restart-launcher[Enable/Disable automatic opening of the Launcher when exiting FlightGear]:Boolean values:(true false 1 0 yes no)' \
		'--enable-restart-launcher[Enable automatic opening of the Launcher when exiting FlightGear]' \
		'--disable-restart-launcher[Disable automatic opening of the Launcher when exiting FlightGear]' \
	'--restore-defaults[Enable/disable resetting all user settings to their defaults]:Boolean values:(true false 1 0 yes no)' \
		'--enable-restore-defaults[Enable resetting all user settings to their defaults]' \
		'--disable-restore-defaults[Disable resetting all user settings to their defaults]' \
	'--roc=[Specify initial climb rate]' \
	'--rul=[Open connection using the RUL protocol]' \
	'--runway=[Specify starting runway (must also specify an airport)]:Runway:->runway' \
	'--save-on-exit[Enable/disable saving preferences at program exit]:Boolean values:(true false 1 0 yes no)' \
		'--enable-save-on-exit[Allow saving preferences at program exit]' \
		'--disable-save-on-exit[Do not save preferences upon program exit]' \
	'--sentry[Enable/disable sending crash and error reports to the development team]:Boolean values:(true false 1 0 yes no)' \
		'--enable-sentry[Enable sending crash and error reports to the development team]' \
		'--disable-sentry[Disable sending crash and error reports to the development team]' \
	'--shading-flat[Enable flat shading]' \
	'--shading-smooth[Enable smooth shading]' \
	'--show-aircraft[Print a list of the currently available aircraft types]:Boolean values:(true false 1 0 yes no)' \
	'--show-sound-devices[Enable/disable displaying the list of audio devices]:Boolean values:(true false 1 0 yes no)' \
	'--sound[Enable/disable sound effects]:Boolean values:(true false 1 0 yes no)' \
		'--disable-sound[Disable sound effects]' \
		'--enable-sound[Enable sound effects]' \
	'--sound-device=[Explicitly specify the audio device to use]' \
	'--specular-highlight[Enable/disable specular reflections on textured objects]:Boolean values:(true false 1 0 yes no)' \
		'--disable-specular-highlight[Disable specular reflections on textured objects]' \
		'--enable-specular-highlight[Enable specular reflections on textured objects]' \
	'--speed=[Run the FDM n times faster than real time]' \
	'--splash-screen[Enable/disable splash screen]:Boolean values:(true false 1 0 yes no)' \
		'--disable-splash-screen[Disable splash screen]' \
		'--enable-splash-screen[Enable splash screen]' \
	'--start-date-gmt=[Specify a starting date/time with respect to Greenwich Mean Time (yyyy:mm:dd:hh:mm:ss)]' \
	'--start-date-lat=[Specify a starting date/time with respect to Local Aircraft Time (yyyy:mm:dd:hh:mm:ss)]' \
	'--start-date-sys=[Specify a starting date/time with respect to system time (yyyy:mm:dd:hh:mm:ss)]' \
	'--state=[Specify the initial state of the aircraft]' \
	'--telnet=[Enable telnet server on the specified port]' \
	'--terrain-engine=[Specify the terrain engine you want to use]:Terrain engine:(tilecache pagedLOD)' \
	'--terrasync[Enable/disable automatic scenery downloads/updates]:Boolean values:(true false 1 0 yes no)' \
		'--enable-terrasync[Enable automatic scenery downloads/updates]' \
		'--disable-terrasync[Disable automatic scenery downloads/updates]' \
	'--terrasync-dir=[Specify the TerraSync scenery path]:Directories:_directories' \
	'--texture-cache[Enable/disable texture cache (DDS)]:Boolean values:(true false 1 0 yes no)' \
		'--enable-texture-cache[Enable texture cache (DDS)]' \
		'--disable-texture-cache[Disable texture cache (DDS)]' \
	'--texture-cache-dir=[Specify the DDS texture cache directory]:Directories:_directories' \
	'--texture-filtering=[Specify anisotropic filtering of terrain textures]:Anisotropic filtering:(1 2 4 8 16)' \
	'--time-match-local[Synchronize time with local real-world time]' \
	'--time-match-real[Synchronize time with real-world time]' \
	'--timeofday=[Specify a time of day]:Time of day:(real dawn morning noon afternoon dusk evening midnight)' \
	'--time-offset=[Add this time offset (+/-hh:mm:ss)]' \
	'--trace-read=[Trace the reads for a property]' \
	'--trace-write=[Trace the writes for a property]' \
	'--trim[Trim the model (only with --fdm=jsbsim)]' \
	'--notrim[Do NOT attempt to trim the model (only with --fdm=jsbsim)]' \
	'--turbulence=[Specify turbulence from 0.0 (calm) to 1.0 (severe)]' \
	'--uninstall[Remove $FG_HOME directory]' \
	'--units-feet[Use feet for distances]' \
	'--units-meters[Use meters for distances]' \
	'--vc=[Specify initial airspeed (in knots)]' \
	'--verbose[Show all command line options when combined --help or -h]' \
	'--version[FlightGear version]' \
	'--view-offset=[Specify the default forward view direction as an offset from straight ahead]' \
	'--visibility=[Specify initial visibility (in meters)]' \
	'--visibility-miles=[Specify initial visibility (in miles)]' \
	'--vor=[Specify starting position relative to a VOR]:VOR:->vor' \
	'--vr[Enable/disable VR]:Boolean values:(true false 1 0 yes no)' \
		'--disable-vr[Disable VR]' \
		'--enable-vr[Enable VR]' \
	'--wind=[Specify wind coming from DIR (degrees) - SPEED (knots) - (DIR@SPEED)]' \
	'--wireframe[Enable/disable wireframe drawing mode]:Boolean values:(true false 1 0 yes no)' \
		'--disable-wireframe[Disable wireframe drawing mode]' \
		'--enable-wireframe[Enable wireframe drawing mode]' \
	'--wp=[Specify a waypoint for the GC autopilot]' \
	'--uBody=[Specify velocity along the body X axis]' \
	'--vBody=[Specify velocity along the body Y axis]' \
	'--wBody=[Specify velocity along the body Z axis]' \
	'--vNorth=[Specify velocity along a South-North axis]' \
	'--vEast=[Specify velocity along a West-East axis]' \
	'--vDown=[Specify velocity along a vertical axis]'
)


_fgfs_ai_scenario() {
	local i
	local result

	if ! zstyle -a ":completion:${curcontext}:" fgfs ai_scenario; then
		(( $+_cache_ai_scenario )) ||
			for i in $_fgfs_root/AI/*.xml; do
				i=${i%.xml}
				_cache_ai_scenario+=( ${i##*/} )
			done
		result=( "$_cache_ai_scenario[@]" )
	fi

	compadd -a "$@" - result
}


_fgfs_aircraft() {
	local i
	local result

	if ! zstyle -a ":completion:${curcontext}:" fgfs aircraft; then
		(( $+_cache_aircraft )) ||
			for i in $_fgfs_root/Aircraft/*/*-set.xml; do
				i=${i%-set.xml}
				_cache_aircraft+=( ${i##*/} )
			done

		result=( "$_cache_aircraft[@]" )
	fi

	compadd -a "$@" - result
}


_fgfs_airport() {
	local line
	local result

	if ! zstyle -a ":completion:${curcontext}:" fgfs airport; then
		(( $+_cache_airport )) ||
			gunzip -c $_fgfs_root/Airports/apt.dat.gz |
				while read line; do
					if [[ $line = "" ]]; then
						read line
						_cache_airport+=( $line[(w)5] )
					fi
				done

		result=( "$_cache_airport[@]" )
	fi

	compadd -a "$@" - result
}


_fgfs_runway() {
	local airport
	local line
	local result

	[[ $words == *--airport=(#b)([a-zA-Z]#)* ]] && airport=$match[1]

	if [[ $airport = "" ]]; then
		_message "Please choose airport !"

		return
	fi

	if ! zstyle -a ":completion:${curcontext}:" fgfs runway_$airport; then
		(( $+_cache_runway )) ||
			gunzip -c $_fgfs_root/Airports/apt.dat.gz |
				while read line; do
					if [[ $line = "" ]]; then
						read line
						name=( $line[(w)5] )
						if [[ $name = $airport ]]; then
							while read line; do
								_cache_runway+=( $line[(w)4] )
								break
							done
							break
						fi
					fi
				done

		_cache_airport_name=$airport

		result=( "$_cache_runway[@]" );
	fi

	compadd -a "$@" - result
}


_arguments -C -s "$_fgfs_options[@]" && return 0


case $state in
	ai-scenario)
		_fgfs_ai_scenario && return 0
	;;

	aircraft|vehicle)
		_fgfs_aircraft && return 0
	;;

	airport)
		_fgfs_airport && return 0
	;;

	runway)
		_fgfs_runway && return 0
	;;

	parkpos)
	;;

	vor)
	;;

	ndb)
	;;

	fix)
	;;
esac



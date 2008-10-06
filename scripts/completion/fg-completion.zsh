#compdef fgfs

local _fgfs_root
local _fgfs_options

local state


_fgfs_root=${FG_ROOT:-/usr/local/share/FlightGear}


_fgfs_options=(
	'(-h --help)'{-h,--help}'[Show the most relevant command line options]' \
	'--version[FlightGear version]' \
	'--verbose[Show all command line options when combined --help or -h]' \
	'--disable-intro-music[Disable introduction music]' \
	'--enable-intro-music[Enable introduction music]' \
	'--units-feet[Use feet for distances]' \
	'--units-meters[Use meters for distances]' \
	'--disable-sound[Disable sound effects]' \
	'--enable-sound[Enable sound effects]' \
	'--disable-panel[Disable instrument panel]' \
	'--enable-panel[Enable instrument panel]' \
	'--disable-hud[Disable Heads Up Display (HUD)]' \
	'--enable-hud[Enable Heads Up Display (HUD)]' \
	'--disable-anti-alias-hud[Disable anti-aliased HUD]' \
	'--enable-anti-alias-hud[Enable anti-aliased HUD]' \
	'--disable-hud-3d[Disable 3D HUD]' \
	'--enable-hud-3d[Enable 3D HUD]' \
	'--hud-tris[Hud displays number of triangles rendered]' \
	'--hud-culled[Hud displays percentage of triangles culled]' \
	'--disable-random-objects[Exclude random scenery objects (buildings, etc.)]' \
	'--enable-random-objects[Include random scenery objects (buildings, etc.)]' \
	'--disable-ai-models[Disable the artifical traffic subsystem]' \
	'--enable-ai-models[Enable the artifical traffic]' \
	'--disable-freeze[Start in a running state]' \
	'--enable-freeze[Start in a frozen state]' \
	'--disable-fuel-freeze[Fuel is consumed normally]' \
	'--enable-fuel-freeze[Fuel tank quantity forced to remain constant]' \
	'--disable-clock-freeze[Clock advances normally]' \
	'--enable-clock-freeze[Do not advance clock]' \
	'--disable-splash-screen[Disable splash screen]' \
	'--enable-splash-screen[Enable splash screen]' \
	'--disable-mouse-pointer[Disable extra mouse pointer]' \
	'--enable-mouse-pointer[Enable extra mouse pointer]' \
	'--fog-disable[Disable fog/haze]' \
	'--fog-fastest[Enable fastest fog/haze]' \
	'--fog-nicest[Enable nicest fog/haze]' \
	'--disable-enhanced-lighting[Disable enhanced runway lighting]' \
	'--enable-enhanced-lighting[Enable enhanced runway lighting]' \
	'--disable-distance-attenuation[Disable runway light distance attenuation]' \
	'--enable-distance-attenuation[Enable runway light distance attenuation]' \
	'--disable-specular-highlight[Disable specular reflections on textured objects]' \
	'--enable-specular-highlight[Enable specular reflections on textured objects]' \
	'--disable-fullscreen[Disable fullscreen mode]' \
	'--enable-fullscreen[Enable fullscreen mode]' \
	'--disable-game-mode[Disable full-screen game mode]' \
	'--enable-game-mode[Enable full-screen game mode]' \
	'--shading-flat[Enable flat shading]' \
	'--shading-smooth[Enable smooth shading]' \
	'--disable-skyblend[Disable sky blending]' \
	'--enable-skyblend[Enable sky blending]' \
	'--disable-textures[Disable textures]' \
	'--enable-textures[Enable textures]' \
	'--disable-wireframe[Disable wireframe drawing mode]' \
	'--enable-wireframe[Enable wireframe drawing mode]' \
	'--notrim[Do NOT attempt to trim the model (only with fdm=jsbsim)]' \
	'--on-ground[Start at ground level (default)]' \
	'--in-air[Start in air (implied when using --altitude)]' \
	'--enable-auto-coordination[Enable auto coordination]' \
	'--disable-auto-coordination[Disable auto coordination]' \
	'--show-aircraft[Print a list of the currently available aircraft types]' \
	'--time-match-real[Synchronize time with real-world time]' \
	'--time-match-local[Synchronize time with local real-world time]' \
	'--disable-real-weather-fetch[Disbale METAR based real weather fetching]' \
	'--enable-real-weather-fetch[Enable METAR based real weather fetching]' \
	'--disable-horizon-effect[Disable celestial body growth illusion near the horizon]' \
	'--enable-horizon-effect[Enable celestial body growth illusion near the horizon]' \
	'--enable-clouds[Enable 2D (flat) cloud layers]' \
	'--disable-clouds[Disable 2D (flat) cloud layers]' \
	'--enable-clouds3d[Enable 3D (volumetric) cloud layers]' \
	'--disable-clouds3d[Disable 3D (volumetric) cloud layers]' \
	'--atc610x[Enable atc610x interface]' \
	'--enable-save-on-exit[Allow saving preferences at program exit]' \
	'--disable-save-on-exit[Do not save preferences upon program exit]' \
	'--ai-scenario=[Add and enable a new scenario]:AI scenario:->ai-scenario' \
	'--fg-root=[Specify the root data path]:Directories:_directories' \
	'--fg-scenery=[Specify the base scenery path]:Directories:_directories' \
	'--language=[Select the language for this session]:Language:->language' \
	'--control=[Primary control mode]:Primary control mode:(joystick keyboard mouse)' \
	'--browser-app=[Specify path to your web browser]:Directories:_directories' \
	'--config=[Load additional properties from path]' \
	'--failure=[Fail the pitot, static, vacuum, or electrical system]:Failure system:(pitot static vaccum electical)'
	'--bpp=[Specify the bits per pixel]' \
	'--fov=[Specify field of view angle]' \
	'--callsign=[Assign a unique name to a player]' \
	'--aspect-ratio-multiplier=[Specify a multiplier for the aspect ratio]' \
	'--geometry=[Specify window geometry (640x480, etc)]' \
	'--view-offset=[Specify the default forward view direction as an offset from straight ahead]' \
	'--aircraft=[Select an aircraft profile]:Aircraft:->aircraft' \
	'--min-status=[Allows you to define a minimum status level for all listed aircraft]:Minimum status level:(alpha beta early-production production)' \
	'--fdm=[Select the core flight dynamics model]:Core flight dynamics model:(jsb larcsim yasim magic balloon ada external)' \
	'--aero=[Select aircraft aerodynamics model to load]' \
	'--model-hz=[Run the FDM this rate (iterations per second)]' \
	'--speed=[Run the FDM n times faster than real time]' \
	'--aircraft-dir=[Aircraft directory relative to the path of the executable]:Aircraft directory:_directories' \
	'--timeofday=[Specify a time of day]:Time of day:(real dawn morning noon afternoon dusk evening midnight)' \
	'--time-offset=[Add this time offset (+/-hh:mm:ss)]' \
	'--start-date-sys=[Specify a starting date/time with respect to system time (yyyy:mm:dd:hh:mm:ss)]' \
	'--start-date-gmt=[Specify a starting date/time with respect to Greenwich Mean Time (yyyy:mm:dd:hh:mm:ss)]' \
	'--start-date-lat=[Specify a starting date/time with respect to Local Aircraft Time (yyyy:mm:dd:hh:mm:ss)]' \
	'--airport=[Specify starting position relative to an airport]:Airport:->airport' \
	'--runway=[Specify starting runway (must also specify an airport)]:Runway:->runway' \
	'--carrier=[Specify starting position on an AI carrier]:AI carrier:(Nimitz Eisenhower Foch)' \
	'--parkpos=[Specify which starting position on an AI carrier]:Park position:->parkpos' \
	'--vor=[Specify starting position relative to a VOR]:VOR:->vor' \
	'--ndb=[Specify starting position relative to an NDB]:NDB:->ndb' \
	'--fix=[Specify starting position relative to a fix]:FIX:->fix' \
	'--offset-distance=[Specify distance to reference point (in miles)]' \
	'--offset-azimuth=[Specify heading to reference point (in degrees)]' \
	'--lon=[Starting longitude (in degrees)]' \
	'--lat=[Starting latitude (in degrees)]' \
	'--altitude=[Starting altitude]' \
	'--heading=[Specify heading (yaw) angle (Psi)]' \
	'--roll=[Specify roll angle (Phi)]' \
	'--pitch=[Specify pitch angle (Theta)]' \
	'--uBody=[Specify velocity along the body X axis]' \
	'--vBody=[Specify velocity along the body Y axis]' \
	'--wBody=[Specify velocity along the body Z axis]' \
	'--vc=[Specify initial airspeed (in knots)]' \
	'--mach=[Specify initial mach number]' \
	'--glideslope=[Specify flight path angle (in degrees)]' \
	'--roc=[Specify initial climb rate]' \
	'--wp=[Specify a waypoint for the GC autopilot]' \
	'--flight-plan=[Read all waypoints from a file]:Waypoints file:_files' \
	'--nav1=[Set the NAV1 radio frequency, optionally preceded by a radial]' \
	'--nav2=[Set the NAV2 radio frequency, optionally preceded by a radial]' \
	'--adf=[Set the ADF radio frequency, optionally preceded by a card rotation]' \
	'--dme=[Slave the ADF to one of the NAV radios, or set its internal frequency]' \
	'--visibility=[Specify initial visibility (in meters)]' \
	'--visibility-miles=[Specify initial visibility (in miles)]' \
	'--wind=[Specify wind coming from DIR (degrees) - SPEED (knots) - (DIR@SPEED)]' \
	'--turbulence=[Specify turbulence from 0.0 (calm) to 1.0 (severe)]' \
	'--ceiling=[Create an overcast ceiling, optionally with a specific thickness]' \
	'--multiplay=[Specify multipilot communication settings ({in|out},hz,address,port)]' \
	'--proxy=[Specify which proxy server (and port) to use (user:pwd@host:port)]' \
	'--httpd=[Enable http server on the specified port]' \
	'--telnet=[Enable telnet server on the specified port]' \
	'--jpg-httpd=[Enable screen shot http server on the specified port]' \
	'--generic=[Open connection using a predefined communication interface]' \
	'--garmin=[Open connection using the Garmin GPS protocol]' \
	'--joyclient=[Open connection to an Agwagon joystick]' \
	'--jsclient=[Open connection to a remote joystick]' \
	'--native-ctrls=[Open connection using the FG Native Controls protocol]' \
	'--native-fdm=[Open connection using the FG Native FDM protocol]' \
	'--native=[Open connection using the FG Native protocol]' \
	'--nmea=[Open connection using the NMEA protocol]' \
	'--opengc=[Open connection using the OpenGC protocol]' \
	'--props=[Open connection using the interactive property manager]' \
	'--pve=[Open connection using the PVE protocol]' \
	'--ray=[Open connection using the Ray Woodworth motion chair protocol]' \
	'--rul=[Open connection using the RUL protocol]' \
	'--log-level=[Specify which loggin level to use]:Log level:(bulk debug info warn alert)' \
	'--trace-read=[Trace the reads for a property]' \
	'--trace-write=[Trace the writes for a property]' \
	'--season=[Specify the startup season]:Season:(summer winter)' \
	'--vehicle=[Select a vehicle profile]:Vehicle:->vehicle' \
	'--prop:[]'
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

	language)
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



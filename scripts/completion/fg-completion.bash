#!/bin/bash
# Tab completion for FlightGear command line options. To use it put
# this in your ~/.bashrc file:
#
#   export FG_HOME=$HOME/.fgfs/
#   [ -e $FG_HOME/fg-completion.bash ] && source $FG_HOME/fg-completion.bash
#
# Defining FG_HOME is only required if you don't like the default
# "$HOME/.fgfs/". The script doesn't offer all available airports
# for the --airport option, but only those listed in a file
# $FG_HOME/airport.list if available, or a short default list otherwise.
#
# After installing new aircraft you have to rebuild the aircraft list
# by typing   $ fgfs --aircraft=?<TAB>

__fgfs_root=${FG_ROOT:-/usr/local/share/FlightGear}
__fgfs_home=${FG_HOME:-$HOME/.fgfs}
[ -d "$__fgfs_home" ] || mkdir -p "$__fgfs_home"


__fgfs_ac_list="$__fgfs_home/aircraft.list"
__fgfs_apt_list="$__fgfs_home/airport.list"

__fgfs_options="
	--help
	--verbose
	--disable-intro-music
	--enable-intro-music
	--units-feet
	--units-meters
	--disable-sound
	--enable-sound
	--disable-panel
	--enable-panel
	--disable-hud
	--enable-hud
	--disable-anti-alias-hud
	--enable-anti-alias-hud
	--disable-hud-3d
	--enable-hud-3d
	--hud-tris
	--hud-culled
	--disable-random-objects
	--enable-random-objects
	--disable-ai-models
	--enable-ai-models
	--disable-freeze
	--enable-freeze
	--disable-fuel-freeze
	--enable-fuel-freeze
	--disable-clock-freeze
	--enable-clock-freeze
	--disable-splash-screen
	--enable-splash-screen
	--disable-mouse-pointer
	--enable-mouse-pointer
	--fog-disable
	--fog-fastest
	--fog-nicest
	--disable-enhanced-lighting
	--enable-enhanced-lighting
	--disable-distance-attenuation
	--enable-distance-attenuation
	--disable-specular-highlight
	--enable-specular-highlight
	--disable-fullscreen
	--enable-fullscreen
	--disable-game-mode
	--enable-game-mode
	--shading-flat
	--shading-smooth
	--disable-skyblend
	--enable-skyblend
	--disable-textures
	--enable-textures
	--disable-wireframe
	--enable-wireframe
	--notrim
	--on-ground
	--in-air
	--enable-auto-coordination
	--disable-auto-coordination
	--show-aircraft
	--time-match-real
	--time-match-local
	--disable-real-weather-fetch
	--enable-real-weather-fetch
	--disable-horizon-effect
	--enable-horizon-effect
	--enable-clouds
	--disable-clouds
	--enable-clouds3d
	--disable-clouds3d
	--atc610x
	--enable-save-on-exit
	--disable-save-on-exit
	--ai-scenario=
	--fg-root=
	--fg-scenery=
	--language=
	--control=
	--browser-app=
	--config=
	--failure=
	--bpp=
	--fov=
	--callsign=
	--aspect-ratio-multiplier=
	--geometry=
	--view-offset=
	--aircraft=
	--min-status=
	--fdm=
	--aero=
	--model-hz=
	--speed=
	--aircraft-dir=
	--timeofday=
	--time-offset=
	--start-date-sys=
	--start-date-gmt=
	--start-date-lat=
	--airport=
	--runway=
	--carrier=
	--parkpos=
	--vor=
	--ndb=
	--fix=
	--offset-distance=
	--offset-azimuth=
	--lon=
	--lat=
	--altitude=
	--heading=
	--roll=
	--pitch=
	--uBody=
	--vBody=
	--wBody=
	--vc=
	--mach=
	--glideslope=
	--roc=
	--wp=
	--flight-plan=
	--nav1=
	--nav2=
	--adf=
	--dme=
	--visibility=
	--visibility-miles=
	--wind=
	--turbulence=
	--ceiling=
	--multiplay=
	--proxy=
	--httpd=
	--telnet=
	--jpg-httpd=
	--generic=
	--garmin=
	--joyclient=
	--jsclient=
	--native-ctrls=
	--native-fdm=
	--native=
	--nmea=
	--opengc=
	--props=
	--pve=
	--ray=
	--rul=
	--log-level=
	--trace-read=
	--trace-write=
	--season=
	--vehicle=
	--prop:
"


if [ ${BASH_VERSINFO[0]} -eq 2 ] && [[ ${BASH_VERSINFO[1]} = "05b" ]] \
		|| [ ${BASH_VERSINFO[0]} -gt 2 ]; then
	__fgfs_nospace="-o nospace"
fi

shopt -s progcomp


__fgfs_write_ac_list() {
	rm -f $__fgfs_ac_list
	for i in $__fgfs_root/Aircraft/*/*-set.xml; do
		i=${i%-set.xml}
		echo ${i##*/} >>$__fgfs_ac_list
	done
}


[ -e $__fgfs_ac_list ] || __fgfs_write_ac_list


__fgfs_ai_scenario() {
	local i
	for i in $__fgfs_root/AI/*.xml; do
		i=${i%.xml}
		echo ${i##*/}
	done
}


__fgfs_aircraft() {
	while read i; do
		echo $i
	done <$__fgfs_ac_list
}


__fgfs_offer() {
	local i
	for i in "$@"; do
		[ "$i" == "${i%=}" ] && i="$i "
		echo "$i"
	done
}


__fgfs_options=$(__fgfs_offer $__fgfs_options)


__fgfs() {
	COMPREPLY=()
	local IFS=$'\n'$'\t' cur=${COMP_WORDS[COMP_CWORD]} alt

	case "$cur" in
	--ai-scenario=*)
		alt=$(__fgfs_offer $(__fgfs_ai_scenario))
		;;
	--aircraft=\?)
		__fgfs_write_ac_list
		;;
	--aircraft=*|--vehicle=*)
		alt=$(__fgfs_offer $(__fgfs_aircraft))
		;;
	--airport=*)
		if [ -e "$__fgfs_apt_list" ]; then
			alt=$(cat "$__fgfs_apt_list")
		else
			alt=$(__fgfs_offer khaf kpao koak kmry knuq ksjc kccr ksns krhv klvk o62 lpma)
		fi
		;;
	--carrier=*)
		alt=$(__fgfs_offer Nimitz Eisenhower Foch)
		;;
	--control=*)
		alt=$(__fgfs_offer joystick keyboard mouse)
		;;
	--failure=*)
		alt=$(__fgfs_offer pitot static vacuum electrical)
		;;
	--fdm=*)
		alt=$(__fgfs_offer jsbsim yasim uiuc larcsim ufo magic)
		;;
	--geometry=*)
		alt=$(__fgfs_offer 640x480 800x600 1024x768 1152x864 1600x1200)
		;;
	--log-level=*)
		alt=$(__fgfs_offer bulk debug info warn alert)
		;;
	--min-status=*)
		alt=$(__fgfs_offer alpha beta early-production production)
		;;
	--parkpos=*)
		alt=$(__fgfs_offer cat-1 cat-2 cat-3 cat-4 park-1)
		;;
	--season=*)
		alt=$(__fgfs_offer summer winter)
		;;
	--timeofday=*)
		alt=$(__fgfs_offer real dawn morning noon afternoon dusk evening midnight)
		;;
	--prop:*)
		return
		;;
	*)
		alt="$__fgfs_options"
		;;
	esac

	COMPREPLY=($(compgen -W "$alt" -- ${cur#*=}))
}


complete -o default $__fgfs_nospace -F __fgfs fgfs signs fgfsterra


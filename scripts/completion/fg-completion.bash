# © 2014 Raphael Dümig ( duemig at in dot tum dot de ) forum handle: "hamster"
# distributed under the terms of the GPLv3

local_config_file="$HOME/.fgfsrc"

_get_opt()
{
    local opt len
    opt=$1
    len=`expr ${#opt} + 3`
    
    if [ -e "$local_config_file" ]
    then
        stored_opts="$(cat "$local_config_file")"
    fi
    
    for word in $stored_opts "${words[@]}"
    do
        if [ "${word:0:$len}" == "--$opt=" ]
        then
            # TODO: we have to get rid of at least the most important escape sequences!!!
            value="$(echo "${word:$len}" | sed 's/\\ / /g')"
        fi
    done
}

_get_opts()
{
    local opt len
    opt=$1
    len=`expr ${#opt} + 3`
    value=""
    
    if [ -e "$local_config_file" ]
    then
        stored_opts="$(cat "$local_config_file")"
    fi
    
    for word in "$stored_opts ${words[@]}"
    do
        if [ "${word:0:$len}" == "--$opt=" ]
        then
            value+="$(echo "${word:$len}" | sed 's/\\ / /g') "
        fi
    done
}

_get_fg_root()
{
    local value
    _get_opt "fg-root"
    
    # value on the command line?
    if [ -n "$value" ]
    then
        root="$value"
        return 0
    # value stored in environment variable $FG_ROOT?
    elif [ -n "$FG_ROOT" ]
    then
        root="$FG_ROOT"
        return 0
    # no clue! search at the most common places
    else
        for path in /usr{/local,}/share{/games,}/{F,f}light{G,g}ear{/data,} {~,}/Applications/FlightGear.app/Contents/Resources/data
        do
            if [ -e "$path" ]
            then
                root="$path"
                break
            fi
        done
    fi
}

_get_fg_scenery()
{
    local value
    _get_opt "fg-scenery"
    
    # value on command line?
    if [ -n "$value" ]
    then
        scenery="$value"
    # value stored in environment variable $FG_SCENERY?
    elif [ -n "$FG_SCENERY" ]
    then
        scenery="$FG_SCENERY"
    # no clue! try default:
    else
        local root
        _get_fg_root
        scenery="$root/Scenery"
    fi

    return 0
}

_get_fg_aircraft()
{
    local value
    _get_opt "fg-aircraft"
    
    # value on command line?
    if [ -n "$value" ]
    then
        aircraft_dir="$value"
    # value stored in environment variable $FG_AIRCRAFT?
    elif [ -n "$FG_AIRCRAFT" ]
    then
        aircraft_dir="$FG_AIRCRAFT"
    # no clue! try default:
    else
        local root
        _get_fg_root
        aircraft_dir="$root/Aircraft"
    fi
}

_get_airport()
{
    local value
    _get_opt "airport"
    
    if [ -z "$value" ]
    then
        airport="KSFO"
    else
        airport=$value
    fi
}

_get_carrier()
{
    local value
    _get_opt "carrier"
    
    carrier=$value
}

_get_scenarios()
{
    local value
    _get_opts "ai-scenario"
    
    scenarios="$value clemenceau_demo eisenhower_demo foch_demo kuznetsov_demo liaoning_demo nimitz_demo sanantonio_demo truman_demo vinson_demo"
}

_get_fdm()
{
    local value
    _get_opt "fdm"
    
    fdm=$value
}


_fgfs() 
{
    local cur prev words cword split
    _init_completion -s || return

    local root airport aircraft_dir carrier scenarios scenery value fdm
    
    # auto-completion for values of keys ( --key=value )
    case "$prev" in
        --fg-aircraft|\
        --fg-root|\
        --fg-scenery|\
        --flight-plan|\
        --terrasync-dir|\
        --materials-file|\
        --config|\
        --browser-app|\
        --data|\
        --addon|\
        --download-dir|\
        --log-dir|\
        --load-tape|\
        --texture-cache-dir|\
        --jsbsim-output-directive-file|\
        --allow-nasal-read)
            # completion of filesystem path
            _filedir
            return 0 ;;
        --ai-scenario)
            # list of scenarios in $FG_ROOT/AI
            _get_fg_root
            scenarios="$(find "$root/AI" -maxdepth 1 -iname '*.xml' -printf '%f\n' | sed -e 's/.xml$//')"
            
            COMPREPLY=( $(compgen -W "${scenarios}" -- ${cur}) )
            return 0 ;;
        --aircraft|--vehicle)
            # list of aircrafts in $FG_AIRCRAFT
            _get_fg_aircraft
            aircrafts="$(find "$aircraft_dir" -maxdepth 2 -mindepth 2 -iname '*-set.xml' -printf '%f\n' | sed -e 's/-set.xml$//')"
            
            COMPREPLY=( $(compgen -W "${aircrafts}" -- ${cur}) )
            return 0 ;;
        --airport)
            _get_fg_root
            _get_fg_scenery
            # is an index file present in the scenery?
            if [ -e "$scenery/Airports/index.txt" ]
            then
                COMPREPLY=( $(compgen -W "$(awk 'BEGIN { FS="|"; } { print $1 }' "$scenery/Airports/index.txt")" -- ${cur}) )
                return 0
            # or at least the apt.dat file?
            elif [ -e "$root/Airports/apt.dat.gz" ]
            then
                COMPREPLY=( $(compgen -W "$(zcat "$root/Airports/apt.dat.gz" | awk '/^1 / { print $5 }')" -- ${cur}) )
                return 0
            fi
            
            return 0 ;;
        --carrier)
            _get_fg_root
            _get_scenarios
            
            carriers=""
            
            for scenario in $scenarios
            do
                carriers+="$(awk -- '
BEGIN { carrier=0; name=""; }
/<type>/ {
    a=index($0,"<type>")+6; s=index(substr($0,a),"</type>")-1;
    if(substr($0,a,s) == "carrier") carrier=1;
}
/<name>/ {
    if(carrier) {
        a=index($0,"<name>")+6; s=index(substr($0,a),"</name>")-1;
        print substr($0,a,s);
        carrier=0;
    }
}
/<\/entry>/ {
    carrier=0;
    name="";
}' "$root/AI/$scenario.xml") "
            done
            
            COMPREPLY=( $(compgen -W "${carriers}" ${cur}) )
            return 0 ;;
        --runway)
            _get_fg_scenery
            _get_airport
            
            # try to find a thresholds file
            path="$scenery/Airports/${airport:0:1}/${airport:1:1}/${airport:2:1}"
            
            if [ -e "$path/$airport.threshold.xml" ]
            then
                runways="$(awk -- '
/<rwy>/ {
    a=index($0,"<rwy>")+5;
    s=index(substr($0,a),"</rwy>")-1;
    print substr($0,a,s)
}' "$path/$airport.threshold.xml")"
            fi
            
            COMPREPLY=( $(compgen -W "${runways}" -- ${cur}) )
            return 0 ;;
        --carrier-position)
            # try to find out the name of the carrier or the ID of the airport
            _get_carrier
            
            _get_fg_root
            _get_scenarios
            
            for scenario in $scenarios
            do
                positions="$(awk -v carrier_name="$carrier" '
BEGIN { carrier=0; name=0; parkpos=0; }
/<type>/ {
    a=index($0,"<type>")+6; s=index(substr($0,a),"</type>")-1;
    if(substr($0,a,s) == "carrier") carrier=1;
}
/<parking-pos>/ {
    parkpos=(carrier && name);
}
/<name>/ {
    a=index($0,"<name>")+6; s=index(substr($0,a),"</name>")-1;
    if(parkpos) print substr($0,a,s);
    else if(carrier) name=(substr($0,a,s) == carrier_name);
}
/<\/parking-pos>/ {
    parkpos=0;
}
/<\/entry>/ {
    carrier=name=parkpos=0;
}' "$root/AI/$scenario.xml")"
                    
                if [ -n "$positions" ]
                then
                    break
                fi
            done
            
            COMPREPLY=( $(compgen -W "${positions} abeam FLOLS" -- ${cur}) )
            return 0 ;;
        --parkpos|--parking-id)
            _get_fg_scenery
            _get_airport
            
            # search for the groundnet or parking file
            path="$scenery/Airports/${airport:0:1}/${airport:1:1}/${airport:2:1}"
            
            if [ -e "$path/$airport.groundnet.xml" ]
            then
                file="$airport.groundnet.xml"
            elif [ -e "$path/$airport.parking.xml" ]
            then
                file="$airport.parking.xml"
            else
                # no file found => do not try to analyze it!
                return 0
            fi
            
            # build a list of the parking positions at that airport
            positions="$(awk -- '
/<Parking/ {
    pos_active=1;
    name=number="";
}
/name="/ {
    if(pos_active) {
        a=index($0,"name=\"")+6;
        s=index(substr($0,a),"\"")-1;
        name=substr($0,a,s);
    }
}
/number="/ {
    if(pos_active) {
        a=index($0,"number=\"")+8;
        s=index(substr($0,a),"\"")-1;
        number=substr($0,a,s);
    }
}
/\/>/ {
    if(pos_active == 1 && (name!="" || number!="")) print name number;
    pos_active=0;
    name=number="";
}' "$path/$file")"

            COMPREPLY=( $(compgen -W "${positions}" -- ${cur}) )
            return 0 ;;
        --failure)
            COMPREPLY=( $(compgen -W "pitot static vacuum electrical" -- ${cur}) )
            return 0 ;;
        --fix)
            _get_fg_root
            COMPREPLY=( $(compgen -W "$(zcat "$root/Navaids/fix.dat.gz" | awk '{ print substr($3,0,5) }')" -- ${cur}) )
            return 0 ;;
        --fdm)
            COMPREPLY=( $(compgen -W "ada acms aisim balloon jsb magic network pipe ufo yasim external null" -- ${cur}) )
            return 0 ;;
        --min-status)
            COMPREPLY=( $(compgen -W "alpha beta early-production production advanced-production" -- ${cur}) )
            return 0 ;;
        --log-class)
            COMPREPLY=( $(compgen -W "none ai aircraft astro atc autopilot clipper cockpit environment event flight \
                                     general gl gui headless input instrumentation io math nasal navaid network osg \
                                     particles sound systems terrain terrasync undefined view all" -- ${cur}) )
            return 0 ;;
        --log-level)
            COMPREPLY=( $(compgen -W "bulk debug info warn alert" -- ${cur}) )
            return 0 ;;
        --ndb)
            _get_fg_root
            COMPREPLY=( $(compgen -W "$(zcat "$root/Navaids/nav.dat.gz" | awk '/^2 / { print $8 }')" -- ${cur}) )
            return 0 ;;
        --ndb-frequency)
            _get_fg_root
            _get_opt "ndb"
            COMPREPLY=( $(compgen -W "$(zcat "$root/Navaids/nav.dat.gz" | awk -v nav=$value '/^2 / { if($8 == nav) print $5 }')" -- ${cur}) )
            return 0 ;;
        --dme)
            COMPREPLY=( $(compgen -W "nav1 nav2" -- ${cur}) )
            return 0 ;;
        --texture-filtering)
            COMPREPLY=( $(compgen -W "1 2 4 8 16" -- ${cur}) )
            return 0 ;;
        --timeofday)
            COMPREPLY=( $(compgen -W "real dawn morning noon afternoon dusk evening midnight" -- ${cur}) )
            return 0 ;;
        --view-offset)
            COMPREPLY=( $(compgen -W "LEFT RIGHT CENTER" -- ${cur}) )
            return 0 ;;
        --vor)
            _get_fg_root
            COMPREPLY=( $(compgen -W "$(zcat "$root/Navaids/nav.dat.gz" | awk '/^3 / { print $8 }')" -- ${cur}) )
            return 0 ;;
        --vor-frequency)
            _get_fg_root
            _get_opt "vor"
            COMPREPLY=( $(compgen -W "$(zcat "$root/Navaids/nav.dat.gz" | awk -v nav=$value '/^3 / { if($8 == nav) print substr($5,0,3) "." substr($5,4) }')" -- ${cur}) )
            return 0 ;;
        --bpp)
            COMPREPLY=( $(compgen -W "16 24 32" -- ${cur}) )
            return 0 ;;
        --graphics-preset)
            COMPREPLY=( $(compgen -W "minimal-quality low-quality medium-quality high-quality ultra-quality" -- ${cur}) )
            return 0 ;;
        --terrain-engine)
            COMPREPLY=( $(compgen -W "tilecache pagedLOD" -- ${cur}) )
            return 0 ;;
        --lod-texturing)
            COMPREPLY=( $(compgen -W "bluemarble raster debug" -- ${cur}) )
            return 0 ;;
        --aero)
            return 0 ;;
        --compositor)
            COMPREPLY=( $(compgen -W "Compositor/default Compositor/Classic/classic Compositor/HDR/hdr" -- ${cur}) )
            return 0 ;;
        --language)
            COMPREPLY=( $(compgen -W "ca de en es fr it nl pl pt ru tr sk zh TEST" -- ${cur}) )
            return 0 ;;
        --version|\
        --terrasync|\
        --launcher|\
        --splash-screen|\
        --mouse-pointer|\
        --no-default-config|\
        --read-only|\
        --ignore-autosave|\
        --save-on-exit|\
        --restore-defaults|\
        --gui|\
        --panel|\
        --freeze|\
        --fuel-freeze|\
        --clock-freeze|\
        --ai-models|\
        --ai-traffic|\
        --vr|\
        --restart-launcher|\
        --load-tape-create-video|\
        --auto-coordination|\
        --trim|\
        --notrim|\
        --on-ground|\
        --in-air|\
        --show-sound-devices|\
        --sound|\
        --horizon-effect|\
        --distance-attenuation|\
        --specular-highlight|\
        --clouds|\
        --clouds3d|\
        --texture-cache|\
        --fullscreen|\
        --random-objects|\
        --wireframe|\
        --hud|\
        --anti-alias-hud|\
        --hud-3d|\
        --allow-nasal-from-sockets|\
        --sentry|\
        --fgcom|\
        --hold-short|\
        --real-weather-fetch|\
        --horizon-effect|\
        --console|\
        --developer|\
        --fpe|\
        --json-report)
            COMPREPLY=( $(compgen -W "true false 1 0 yes no" -- ${cur}) )
            # Without `return 0` to allow not giving the value true/false/1/0/yes/no
            # and instead giving another option starting with --
            # return 0 ;; <- skip it
    esac
    
    case "${cur}" in
        -*)
            # These options are not included in the -h -v output so we add them manually:
            EnableDisableOptions="--enable-terrasync --disable-terrasync \
                                  --enable-launcher --disable-launcher \
                                  --enable-splash-screen --disable-splash-screen \
                                  --enable-mouse-pointer --disable-mouse-pointer \
                                  --enable-read-only --disable-read-only \
                                  --enable-ignore-autosave --disable-ignore-autosave \
                                  --enable-save-on-exit --disable-save-on-exit \
                                  --enable-restore-defaults --disable-restore-defaults \
                                  --enable-gui --disable-gui \
                                  --enable-panel --disable-panel \
                                  --enable-freeze --disable-freeze \
                                  --enable-fuel-freeze --disable-fuel-freeze \
                                  --enable-clock-freeze --disable-clock-freeze \
                                  --enable-ai-models --disable-ai-models \
                                  --enable-ai-traffic --disable-ai-traffic \
                                  --enable-vr --disable-vr \
                                  --enable-restart-launcher --disable-restart-launcher \
                                  --enable-load-tape-create-video --disable-load-tape-create-video \
                                  --enable-auto-coordination --disable-auto-coordination \
                                  --enable-sound --disable-sound \
                                  --enable-horizon-effect --disable-horizon-effect \
                                  --enable-distance-attenuation --disable-distance-attenuation \
                                  --enable-specular-highlight --disable-specular-highlight \
                                  --enable-clouds --disable-clouds \
                                  --enable-clouds3d --disable-clouds3d \
                                  --enable-texture-cache --disable-texture-cache \
                                  --enable-fullscreen --disable-fullscreen \
                                  --enable-random-objects --disable-random-objects \
                                  --enable-wireframe --disable-wireframe \
                                  --enable-hud --disable-hud \
                                  --enable-anti-alias-hud --disable-anti-alias-hud \
                                  --enable-hud-3d --disable-hud-3d \
                                  --enable-allow-nasal-from-sockets --disable-allow-nasal-from-sockets \
                                  --enable-sentry --disable-sentry \
                                  --enable-fgcom --disable-fgcom \
                                  --enable-hold-short --disable-hold-short \
                                  --enable-real-weather-fetch --disable-real-weather-fetch \
                                  --enable-horizon-effect --disable-horizon-effect \
                                  --enable-developer --disable-developer \
                                  --enable-fpe --disable-fpe";
            # auto completion for options
            COMPREPLY=( $(compgen -W "$(_parse_help fgfs "--help --verbose") ${EnableDisableOptions}" -- ${cur}) )
            # no whitespace after keys
            [[ $COMPREPLY == *= ]] && compopt -o nospace ;;
        *)
            return 0
    esac
    
    return 0
}
complete -F _fgfs fgfs

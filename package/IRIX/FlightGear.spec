product FlightGear
    id "FlightGear Flight Simulator"
    image sw
        id "Software"
        version 00090400
        order 9999
        subsys base_R4k default
            id "Base Software (optimized for R4k)"
            replaces FlightGear.sw.base 0 00090399
            prereq (
                ifl_eoe.sw.c++ 1319002020 maxint
            )
            prereq (
                fw_libjpeg.sw.lib 1235274920 maxint
                fw_libz.sw.lib 1278119120 maxint
            )
            exp FlightGear.sw.base_R4k
        endsubsys
        subsys base_R10k
            id "Base Software (optimized for R10k)"
            replaces FlightGear.sw.base 0 00090399
            prereq (
                ifl_eoe.sw.c++ 1319002020 maxint
            )
            prereq (
                fw_libjpeg.sw.lib 1235274920 maxint
                fw_libz.sw.lib 1278119120 maxint
            )
            exp FlightGear.sw.base_R10k
        endsubsys
        subsys optional default
            id "Optional Software"
            replaces FlightGear.sw.base 0 00090399
            prereq (
                FlightGear.sw.base_R4k 00090400 maxint
            )
            prereq (
                FlightGear.sw.base_R10k 00090400 maxint
            )
            exp FlightGear.sw.optional
        endsubsys
        subsys admin default
            id "Graphical Administration Tools"
            replaces FlightGear.sw.base 0 00090399
            prereq (
                FlightGear.sw.base_R4k 00090400 maxint
                fw_fltk.sw.lib 1278985320 maxint
            )
            prereq (
                FlightGear.sw.base_R10k 00090400 maxint
                fw_fltk.sw.lib 1278985320 maxint
            )
            exp FlightGear.sw.admin
        endsubsys
        subsys terrasync
            id "Terrain download Synchronization utility"
            replaces FlightGear.sw.base 0 00090399
            prereq (
                FlightGear.sw.base_R4k 00090400 maxint
                fw_wget.sw.wget 1278327620 maxint
            )
            prereq (
                FlightGear.sw.base_R10k 00090400 maxint
                fw_wget.sw.wget 1278327620 maxint
            )
            exp FlightGear.sw.terrasync
        endsubsys
        subsys servers
            id "Remote Joystick Server"
            replaces self
            exp FlightGear.sw.servers
        endsubsys
    endimage
    image man
        id "Man Pages"
        version 00090400
        order 9999
        subsys manpages default
            id "Man Pages"
            replaces self
            exp FlightGear.man.manpages
        endsubsys
    endimage
endproduct

function(setup_fgfs_bundle target)
    execute_process(COMMAND date +%Y
        OUTPUT_VARIABLE CURRENT_YEAR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # in our local CMakeModules dir
    set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST FlightGearBundleInfo.plist.in
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.flightgear.mac-nightly"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${FLIGHTGEAR_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING "FlightGear ${FLIGHTGEAR_VERSION} Nightly"
        MACOSX_BUNDLE_BUNDLE_VERSION ${FLIGHTGEAR_VERSION}
        MACOSX_BUNDLE_COPYRIGHT "FlightGear ${FLIGHTGEAR_VERSION} © 1997-${CURRENT_YEAR}, The FlightGear Project. Licensed under the GNU Public License version 2."
        MACOSX_BUNDLE_INFO_STRING "Nightly build of FlightGear ${FLIGHTGEAR_VERSION} for testing and development"
        MACOSX_BUNDLE_BUNDLE_NAME "FlightGear"
        MACOSX_BUNDLE_ICON_FILE "FlightGear.icns"
    )
endfunction()

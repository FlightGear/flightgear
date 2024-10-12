function(setup_fgfs_bundle target)

    string(TIMESTAMP currentYear "%Y")

    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME "FlightGear"
        MACOSX_RPATH TRUE
        MACOSX_FRAMEWORK_IDENTIFIER com.flightgear.org
        MACOSX_BUNDLE_INFO_PLIST FlightGearBundleInfo.plist.in

        MACOSX_BUNDLE_BUNDLE_NAME "FlightGear"
        MACOSX_BUNDLE_INFO_STRING "FlightGear, the open-source, cross-platform flight simulator. http://www.flightgear.org/"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.flightgear.mac"
        MACOSX_BUNDLE_ICON_FILE "FlightGear.icns"
        MACOSX_BUNDLE_COPYRIGHT "FlightGear ${FLIGHTGEAR_VERSION} © 1996-${currentYear}, The FlightGear project. Licensed under the GNU Public License version 2."
        MACOSX_BUNDLE_BUNDLE_VERSION ${FLIGHTGEAR_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING "FlightGear ${FLIGHTGEAR_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${FLIGHTGEAR_VERSION}
    )
endfunction()

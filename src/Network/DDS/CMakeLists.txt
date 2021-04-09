include(FlightGearComponent)

set(SOURCES
        dds_ctrls.c
        dds_gui.c
        dds_fdm.c
        dds_props.c
	)

set(HEADERS
        dds_ctrls.h
        dds_fdm.h
        dds_gui.h
        dds_props.h
	)

add_executable(fg_dds_log
    WIN32
    MACOSX_BUNDLE
    fg_dds_log.cpp
    ${SOURCES}
)
setup_fgfs_libraries(fg_dds_log)

add_executable(fg_dds_prop
    WIN32
    MACOSX_BUNDLE
    fg_dds_prop.cpp
    ${SOURCES}
)
setup_fgfs_libraries(fg_dds_prop)


flightgear_component(Network "${SOURCES}" "${HEADERS}")

include(FlightGearComponent)

set(SOURCES
        dds_ctrls.c
        dds_gui.c
        dds_fdm.c
	)

set(HEADERS
        dds_ctrls.h
        dds_fdm.h
        dds_gui.h
	)

add_executable(fgfdm_log
    WIN32
    MACOSX_BUNDLE
    fgfdm_log.c
    ${SOURCES}
)
target_link_libraries(fgfdm_log CycloneDDS::ddsc)

flightgear_component(Network "${SOURCES}" "${HEADERS}")

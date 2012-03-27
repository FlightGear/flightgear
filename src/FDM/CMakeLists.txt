include(FlightGearComponent)

if(SP_FDMS)
set(SP_FDM_SOURCES
	SP/ACMS.cxx
	SP/ADA.cxx
	SP/Balloon.cxx
	SP/BalloonSim.cpp
	SP/MagicCarpet.cxx
	)
endif()

set(UIUC_SOURCES
	uiuc_1DdataFileReader.cpp
	uiuc_1Dinterpolation.cpp
	uiuc_2DdataFileReader.cpp
	uiuc_2Dinterpolation.cpp
	uiuc_3Dinterpolation.cpp
	uiuc_aerodeflections.cpp
	uiuc_alh_ap.cpp
	uiuc_auto_pilot.cpp
	uiuc_betaprobe.cpp
	uiuc_coef_drag.cpp
	uiuc_coef_lift.cpp
	uiuc_coef_pitch.cpp
	uiuc_coef_roll.cpp
	uiuc_coef_sideforce.cpp
	uiuc_coef_yaw.cpp
	uiuc_coefficients.cpp
	uiuc_controlInput.cpp
	uiuc_convert.cpp
	uiuc_engine.cpp
	uiuc_find_position.cpp
	uiuc_flapdata.cpp
	uiuc_fog.cpp
	uiuc_gear.cpp
	uiuc_get_flapper.cpp
	uiuc_getwind.cpp
	uiuc_hh_ap.cpp
	uiuc_ice.cpp
	uiuc_iceboot.cpp
	uiuc_iced_nonlin.cpp
	uiuc_icing_demo.cpp
	uiuc_initializemaps.cpp
	uiuc_map_CD.cpp
	uiuc_map_CL.cpp
	uiuc_map_CY.cpp
	uiuc_map_Cm.cpp
	uiuc_map_Cn.cpp
	uiuc_map_Croll.cpp
	uiuc_map_controlSurface.cpp
	uiuc_map_engine.cpp
	uiuc_map_fog.cpp
	uiuc_map_gear.cpp
	uiuc_map_geometry.cpp
	uiuc_map_ice.cpp
	uiuc_map_init.cpp
	uiuc_map_keyword.cpp
	uiuc_map_mass.cpp
	uiuc_map_misc.cpp
	uiuc_map_record1.cpp
	uiuc_map_record2.cpp
	uiuc_map_record3.cpp
	uiuc_map_record4.cpp
	uiuc_map_record5.cpp
	uiuc_map_record6.cpp
	uiuc_menu.cpp
	uiuc_menu_CD.cpp
	uiuc_menu_CL.cpp
	uiuc_menu_CY.cpp
	uiuc_menu_Cm.cpp
	uiuc_menu_Cn.cpp
	uiuc_menu_Croll.cpp
	uiuc_menu_controlSurface.cpp
	uiuc_menu_engine.cpp
	uiuc_menu_fog.cpp
	uiuc_menu_functions.cpp
	uiuc_menu_gear.cpp
	uiuc_menu_geometry.cpp
	uiuc_menu_ice.cpp
	uiuc_menu_init.cpp
	uiuc_menu_mass.cpp
	uiuc_menu_misc.cpp
	uiuc_menu_record.cpp
	uiuc_pah_ap.cpp
	uiuc_parsefile.cpp
	uiuc_rah_ap.cpp
	uiuc_recorder.cpp
	uiuc_warnings_errors.cpp
	uiuc_wrapper.cpp
	)
	
set(LARCSIM_SOURCES
    atmos_62.c
    basic_aero.c
    basic_engine.c
    basic_gear.c
    basic_init.c
    c172_aero.c
    c172_engine.c
    c172_gear.c
    c172_init.c
    cherokee_aero.c
    cherokee_engine.c
    cherokee_gear.c
    cherokee_init.c
    default_model_routines.c
    ls_accel.c
    ls_aux.c
    ls_geodesy.c
    ls_gravity.c
    ls_init.c
    ls_interface.c
    ls_matrix.c
    ls_model.c
    ls_step.c
    navion_aero.c
    navion_engine.c
    navion_gear.c
    navion_init.c
    uiuc_aero.c
    IO360.cxx
    LaRCsim.cxx
    LaRCsimIC.cxx
    )
	
set(SOURCES
	NullFDM.cxx
	UFO.cxx
	fdm_shell.cxx
	flight.cxx
	flightProperties.cxx
	TankProperties.cxx
	groundcache.cxx
	${SP_FDM_SOURCES}
	ExternalNet/ExternalNet.cxx
	ExternalPipe/ExternalPipe.cxx
	)

if(ENABLE_UIUC_MODEL)
	foreach(component ${UIUC_SOURCES})
		list(APPEND SOURCES "UIUCModel/${component}")
	endforeach()
endif()

if(ENABLE_LARCSIM)
	foreach(component ${LARCSIM_SOURCES})
		list(APPEND SOURCES "LaRCsim/${component}")
	endforeach()
endif()

flightgear_component(FDM "${SOURCES}")

if(ENABLE_YASIM)
	add_subdirectory(YASim)
endif()


if(ENABLE_JSBSIM)
    add_subdirectory(JSBSim)
    
    # FIXME - push this down once JSBSim doesn't expose private headers
    include_directories(${PROJECT_SOURCE_DIR}/src/FDM/JSBSim)
endif()


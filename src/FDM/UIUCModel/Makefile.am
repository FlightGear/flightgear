

noinst_LIBRARIES = libUIUCModel.a

libUIUCModel_a_SOURCES = \
                        uiuc_1DdataFileReader.cpp uiuc_1DdataFileReader.h \
                        uiuc_1Dinterpolation.cpp uiuc_1Dinterpolation.h \
                        uiuc_2DdataFileReader.cpp uiuc_2DdataFileReader.h \
                        uiuc_2Dinterpolation.cpp uiuc_2Dinterpolation.h \
			uiuc_3Dinterpolation.cpp uiuc_3Dinterpolation.h \
                        uiuc_aerodeflections.cpp uiuc_aerodeflections.h \
			uiuc_aircraftdir.h uiuc_aircraft.h \
                        uiuc_betaprobe.cpp uiuc_betaprobe.h \
                        uiuc_coef_drag.cpp uiuc_coef_drag.h \
                        uiuc_coef_lift.cpp uiuc_coef_lift.h \
                        uiuc_coef_pitch.cpp uiuc_coef_pitch.h \
                        uiuc_coef_roll.cpp uiuc_coef_roll.h \
                        uiuc_coef_sideforce.cpp uiuc_coef_sideforce.h \
                        uiuc_coef_yaw.cpp uiuc_coef_yaw.h \
                        uiuc_coefficients.cpp uiuc_coefficients.h \
			uiuc_controlInput.cpp uiuc_controlInput.h \
                        uiuc_convert.cpp uiuc_convert.h \
                        uiuc_engine.cpp uiuc_engine.h \
			uiuc_fog.cpp uiuc_fog.h \
			uiuc_gear.cpp uiuc_gear.h\
                        uiuc_ice.cpp uiuc_ice.h \
                        uiuc_initializemaps.cpp uiuc_initializemaps.h \
			uiuc_map_CD.cpp uiuc_map_CD.h \
			uiuc_map_CL.cpp uiuc_map_CL.h \
			uiuc_map_CY.cpp uiuc_map_CY.h \
			uiuc_map_Cm.cpp uiuc_map_Cm.h \
			uiuc_map_Cn.cpp uiuc_map_Cn.h \
			uiuc_map_Croll.cpp uiuc_map_Croll.h \
			uiuc_map_controlSurface.cpp uiuc_map_controlSurface.h \
			uiuc_map_engine.cpp uiuc_map_engine.h \
			uiuc_map_fog.cpp uiuc_map_fog.h \
			uiuc_map_geometry.cpp uiuc_map_geometry.h \
			uiuc_map_ice.cpp uiuc_map_ice.h \
                        uiuc_map_gear.cpp uiuc_map_gear.h \
			uiuc_map_init.cpp uiuc_map_init.h \
			uiuc_map_keyword.cpp uiuc_map_keyword.h \
			uiuc_map_mass.cpp uiuc_map_mass.h \
			uiuc_map_misc.cpp uiuc_map_misc.h \
			uiuc_map_record1.cpp uiuc_map_record1.h \
			uiuc_map_record2.cpp uiuc_map_record2.h \
			uiuc_map_record3.cpp uiuc_map_record3.h \
			uiuc_map_record4.cpp uiuc_map_record4.h \
			uiuc_map_record5.cpp uiuc_map_record5.h \
                        uiuc_menu.cpp uiuc_menu.h \
                        uiuc_parsefile.cpp uiuc_parsefile.h \
                        uiuc_recorder.cpp uiuc_recorder.h \
                        uiuc_warnings_errors.cpp uiuc_warnings_errors.h \
                        uiuc_wrapper.cpp uiuc_wrapper.h

if OLD_AUTOMAKE
INCLUDES += -I$(top_srcdir) -I$(top_srcdir)/src
else
INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/src
endif

find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
if(GLUT_FOUND)
	message(STATUS "found glut")

	add_executable(fgpanel main.cxx
		ApplicationProperties.hxx
		FGGLApplication.cxx 
		FGGLApplication.hxx
		FGPanelApplication.cxx 
		FGPanelApplication.hxx
		FGPNGTextureLoader.cxx 
		FGPNGTextureLoader.hxx
		FGRGBTextureLoader.cxx
		FGRGBTextureLoader.hxx 
		FGPanelProtocol.cxx
		FGPanelProtocol.hxx
		FGFontCache.cxx
		FGFontCache.hxx
		panel.cxx 
		panel.hxx
		panel_io.cxx
		panel_io.hxx
	)

	target_link_libraries(fgpanel 
		${PNG_LIBRARIES}
		${GLUT_LIBRARIES}
		${SIMGEAR_CORE_LIBRARIES}
		${OPENGL_LIBRARIES}
		${PLIB_LIBRARIES}
		${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	)

	include_directories(${PNG_INCLUDE_DIR})

	install(TARGETS fgpanel RUNTIME DESTINATION bin)
else()
	message(STATUS "glut NOT found, can't build fgpanel")
endif()

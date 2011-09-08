#FIXME: does this work in windows?
find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
if(GLUT_FOUND)
	message(STATUS "found glut")

	add_executable(fgpanel main.cxx
		FGGLApplication.cxx 
		FGPanelApplication.cxx 
		FGPNGTextureLoader.cxx 
		FGRGBTextureLoader.cxx 
		FGPanelProtocol.cxx
		FGFontCache.cxx
		panel.cxx 
		panel_io.cxx)

	target_link_libraries(fgpanel 
		${PNG_LIBRARIES}
		${GLUT_LIBRARIES}
		${SIMGEAR_CORE_LIBRARIES}
		${OPENGL_LIBRARIES}
		${ZLIB_LIBRARIES}
		${PLIB_LIBRARIES}
		${RT_LIBRARY}
		)

	install(TARGETS fgpanel RUNTIME DESTINATION bin)
else()
	message(STATUS "glut NOT found, can't build fgpanel")
endif()

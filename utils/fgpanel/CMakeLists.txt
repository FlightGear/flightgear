#FIXME: does this work in windows?
find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

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
	${SIMGEAR_LIBRARIES}
	${OPENGL_LIBRARIES}
	${ZLIB_LIBRARIES}
	${PLIB_LIBRARIES}
	)

install(TARGETS fgpanel RUNTIME DESTINATION bin)

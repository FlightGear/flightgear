find_package(PNG)
find_package(OpenGL)
find_package(GLEW )
find_package(Freetype)

if ((NOT PNG_FOUND) OR (NOT OPENGL_FOUND) OR (NOT GLEW_FOUND) OR (NOT FREETYPE_FOUND))
  message(WARNING "FGPanel enabled, but some dependencies are missing")
  message(STATUS "libPNG: ${PNG_FOUND}")
  message(STATUS "OpenGL: ${OPENGL_FOUND}")
  message(STATUS "GLEW: ${GLEW_FOUND}")
  message(STATUS "Freetype: ${FREETYPE_FOUND}")
  return()
endif()

find_path(BCMHOST_INCLUDE_DIR
  NAMES bcm_host.h
  PATHS /opt/vc/include
  NO_DEFAULT_PATH
  )

include_directories(
  /usr/include/freetype2
  ${PNG_INCLUDE_DIR}
  )

if(${BCMHOST_INCLUDE_DIR} STREQUAL "BCMHOST_INCLUDE_DIR-NOTFOUND")
else()
  # CMAKE > 3.1 : target_sources(fgpanel
  set(TARGET_SOURCES
    GLES_utils.cxx
    GLES_utils.hxx
    )
endif()

add_executable(fgpanel
  main.cxx
  ApplicationProperties.hxx
  ApplicationProperties.cxx
  FGCroppedTexture.hxx
  FGCroppedTexture.cxx
  FGDummyTextureLoader.hxx
  FGDummyTextureLoader.cxx
  FGFontCache.cxx
  FGFontCache.hxx
  FGGLApplication.cxx
  FGGLApplication.hxx
  FGGroupLayer.cxx
  FGGroupLayer.hxx
  FGInstrumentLayer.cxx
  FGInstrumentLayer.hxx
  FGLayeredInstrument.cxx
  FGLayeredInstrument.hxx
  FGPanel.cxx
  FGPanel.hxx
  FGPanelApplication.cxx
  FGPanelApplication.hxx
  FGPanelInstrument.cxx
  FGPanelInstrument.hxx
  FGPanelProtocol.cxx
  FGPanelProtocol.hxx
  FGPanelTransformation.cxx
  FGPanelTransformation.hxx
  FGPNGTextureLoader.cxx
  FGPNGTextureLoader.hxx
  FGRGBTextureLoader.cxx
  FGRGBTextureLoader.hxx
  FGSwitchLayer.cxx
  FGSwitchLayer.hxx
  FGTextLayer.cxx
  FGTextLayer.hxx
  FGTexturedLayer.cxx
  FGTexturedLayer.hxx
  panel_io.cxx
  panel_io.hxx
  GL_utils.cxx
  GL_utils.hxx
  ${TARGET_SOURCES}
  )

target_link_libraries(fgpanel
  SimGearCore
  ${PNG_LIBRARIES}
  ${SIMGEAR_CORE_LIBRARIES}
  ${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
  -lfreetype
  )

if(${BCMHOST_INCLUDE_DIR} STREQUAL "BCMHOST_INCLUDE_DIR-NOTFOUND")
  find_package(GLUT REQUIRED)
  if(GLUT_FOUND)
    message(STATUS "found glut")

    target_link_libraries(fgpanel
      ${GLUT_LIBRARIES}
      -lGLEW ${OPENGL_LIBRARIES}
      )
  else(GLUT_FOUND)
    message(STATUS "glut NOT found, can't build fgpanel")
  endif(GLUT_FOUND)
else()
  message(STATUS "found Raspberry Pi")

  target_link_libraries(fgpanel
    -lGLESv2 -lEGL -lm -lbcm_host -L/opt/vc/lib
    )

  include_directories(
    ${BCMHOST_INCLUDE_DIR}
    ${BCMHOST_INCLUDE_DIR}/interface/vcos/pthreads
    ${BCMHOST_INCLUDE_DIR}/interface/vmcs_host/linux
    )

  add_definitions(-D_GLES2 -D_RPI)
endif()

install(TARGETS fgpanel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

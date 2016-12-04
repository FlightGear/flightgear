find_package(PNG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Freetype REQUIRED)

find_path(BCMHOST_INCLUDE_DIR
  NAMES bcm_host.h
  PATHS /opt/vc/include
  NO_DEFAULT_PATH
  )

include_directories(
  ${FREETYPE_INCLUDE_DIRS}
  ${PNG_INCLUDE_DIR}
  )

if(BCMHOST_INCLUDE_DIR)
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
  ${PNG_LIBRARIES}
  SimGearCore
  ${FREETYPE_LIBRARIES}
  )

if(BCMHOST_INCLUDE_DIR)
  message(STATUS "found Raspberry Pi")

  include_directories(
    ${BCMHOST_INCLUDE_DIR}
    ${BCMHOST_INCLUDE_DIR}/interface/vcos/pthreads
    ${BCMHOST_INCLUDE_DIR}/interface/vmcs_host/linux
    )

  target_link_libraries(fgpanel
    -lGLESv2 -lEGL -lm -lbcm_host -L/opt/vc/lib
    )

  add_definitions(-D_GLES2 -D_RPI)
else()
  find_package(GLUT)
  if(GLUT_FOUND)

    find_package(GLEW)
    if(GLEW_FOUND)

      include_directories(
        ${GLEW_INCLUDE_DIRS}
        )

      target_link_libraries(fgpanel
        ${GLUT_LIBRARIES}
        ${GLEW_LIBRARIES}
        ${OPENGL_LIBRARIES}
        )
    else(GLEW_FOUND)
      message(WARNING "GLEW NOT found, can't build FGPanel")
      set(WITH_FGPANEL 0)
    endif(GLEW_FOUND)
  else(GLUT_FOUND)
    message(WARNING "GLUT NOT found, can't build FGPanel")
    set(WITH_FGPANEL 0)
  endif(GLUT_FOUND)
endif()

if(WITH_FGPANEL)
  message(STATUS "FGPanel : ENABLED")
  install(TARGETS fgpanel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
  message(WARNING "FGPanel : DISABLED")
endif()

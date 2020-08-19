find_package(PNG QUIET)
find_package(OpenGL QUIET)
find_package(Freetype QUIET)

find_package(GLUT QUIET)
find_package(GLEW QUIET)

if(NOT ${GLUT_FOUND})
  message(WARNING "GLUT NOT found, can't build FGPanel")
  set(WITH_FGPANEL 0)
  return()
endif()

if(NOT ${GLEW_FOUND})
  message(WARNING "GLEW NOT found, can't build FGPanel")
  set(WITH_FGPANEL 0)
  return()
endif()

if((NOT PNG_FOUND) OR (NOT OPENGL_FOUND) OR (NOT FREETYPE_FOUND))
  message(WARNING "FGPanel enabled, but some dependencies are missing")
  message(STATUS "libPNG: ${PNG_FOUND}")
  message(STATUS "OpenGL: ${OPENGL_FOUND}")
  message(STATUS "Freetype: ${FREETYPE_FOUND}")
  return()
endif()

find_path(BCMHOST_INCLUDE_DIR
  NAMES bcm_host.h
  PATHS /opt/vc/include
  NO_DEFAULT_PATH
  )

set(TARGET_SOURCES
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
  )

add_executable(fgpanel ${TARGET_SOURCES})

target_link_libraries(fgpanel
  SimGearCore
  ${PNG_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  OpenGL::GL
    ${GLUT_LIBRARIES}
    ${GLEW_LIBRARIES}
  )

target_include_directories(fgpanel PUBLIC
  ${FREETYPE_INCLUDE_DIRS}
  ${PNG_INCLUDE_DIR}
  ${GLEW_INCLUDE_DIRS}
  )

  if(MSVC)
    target_compile_definitions(fgpanel PUBLIC
      -DFREEGLUT_LIB_PRAGMAS=0
      )
  endif(MSVC)

if(BCMHOST_INCLUDE_DIR)
  message(STATUS "found Raspberry Pi")

  add_executable(fgpanel-egl ${TARGET_SOURCES}
    GLES_utils.cxx
    GLES_utils.hxx
    )
  target_include_directories(fgpanel-egl PUBLIC
    ${FREETYPE_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIR}
    ${BCMHOST_INCLUDE_DIR}
    ${BCMHOST_INCLUDE_DIR}/interface/vcos/pthreads
    ${BCMHOST_INCLUDE_DIR}/interface/vmcs_host/linux
    )

  target_link_libraries(fgpanel-egl
    SimGearCore
    ${PNG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    -lbrcmGLESv2 -lbrcmEGL -lm -lbcm_host -L/opt/vc/lib
    )

  target_compile_definitions(fgpanel-egl PUBLIC
    -D_GLES2 -D_RPI
    )
  target_compile_definitions(fgpanel PUBLIC
    -D_RPI
    )
  message(STATUS "FGPanel (Raspberry Pi) : ENABLED")
  install(TARGETS fgpanel-egl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else(BCMHOST_INCLUDE_DIR)
  message(STATUS "FGPanel (Raspberry Pi) : DISABLED")
endif(BCMHOST_INCLUDE_DIR)




if(WITH_FGPANEL)
  message(STATUS "FGPanel : ENABLED")
  install(TARGETS fgpanel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else(WITH_FGPANEL)
  message(WARNING "FGPanel : DISABLED")
endif(WITH_FGPANEL)

find_package(PNG)
find_package(OpenGL)
find_package(Freetype)

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

include_directories(
  ${FREETYPE_INCLUDE_DIRS}
  ${PNG_INCLUDE_DIR}
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
  )

if(BCMHOST_INCLUDE_DIR)
  message(STATUS "found Raspberry Pi")

  add_executable(fgpanel-egl ${TARGET_SOURCES}
    GLES_utils.cxx
    GLES_utils.hxx
    )
  target_include_directories(fgpanel-egl PUBLIC
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

find_package(GLUT)
if(GLUT_FOUND)
  message(STATUS "found GLUT inc ${GLUT_INCLUDE_DIR}, lib ${GLUT_LIBRARIES}")
  if(MSVC)
    target_compile_definitions(fgpanel PUBLIC
      -DFREEGLUT_LIB_PRAGMAS=0
      )
  endif(MSVC)

  find_package(GLEW)
  if(GLEW_FOUND)
    message(STATUS "found GLEW inc ${GLEW_INCLUDE_DIR}, lib ${GLEW_LIBRARIES}")

    target_include_directories(fgpanel PUBLIC
      ${GLEW_INCLUDE_DIRS}
      )

    target_link_libraries(fgpanel
      ${OPENGL_LIBRARIES}
      ${GLUT_LIBRARIES}
      ${GLEW_LIBRARIES}
      )
  else(GLEW_FOUND)
    message(WARNING "GLEW NOT found, can't build FGPanel")
    set(WITH_FGPANEL 0)
  endif(GLEW_FOUND)
else(GLUT_FOUND)
  message(WARNING "GLUT NOT found, can't build FGPanel")
  set(WITH_FGPANEL 0)
endif(GLUT_FOUND)

if(WITH_FGPANEL)
  message(STATUS "FGPanel : ENABLED")
  install(TARGETS fgpanel RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
else(WITH_FGPANEL)
  message(WARNING "FGPanel : DISABLED")
endif(WITH_FGPANEL)

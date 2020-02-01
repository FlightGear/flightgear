cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

  # AUTOMOC/AUTOUIC on generated files policy.
  if(POLICY CMP0071)
      cmake_policy(SET CMP0071 NEW)
  endif()

include(GNUInstallDirs)

project(FGQCanvas)

find_package(Qt5 5.4 COMPONENTS Widgets WebSockets Gui Quick)

if (NOT Qt5WebSockets_FOUND OR NOT Qt5Quick_FOUND)
  message(WARNING "FGQCanvas utility requested, but QtWebSockets not found")
  message(STATUS "Check you have the development package for Qt5 WebSockets installed")
  return()
endif()

set(SOURCES
  main.cpp
  localprop.cpp
  localprop.h
  fgcanvaselement.cpp
  fgcanvaselement.h
  fgcanvasgroup.cpp
  fgcanvasgroup.h
  fgcanvaspaintcontext.cpp
  fgcanvaspaintcontext.h
  fgcanvaspath.cpp
  fgcanvaspath.h
  fgcanvastext.cpp
  fgcanvastext.h
  fgqcanvasimage.cpp
  fgqcanvasimage.h
  fgqcanvasmap.cpp
  fgqcanvasmap.h
  canvastreemodel.cpp
  canvastreemodel.h
  fgqcanvasfontcache.cpp
  fgqcanvasfontcache.h
  fgqcanvasimageloader.cpp
  fgqcanvasimageloader.h
  elementdatamodel.cpp
  elementdatamodel.h
  canvasitem.cpp
  canvasitem.h
  canvasconnection.cpp
  canvasconnection.h
  applicationcontroller.cpp
  applicationcontroller.h
  canvasdisplay.cpp
  canvasdisplay.h
  canvaspainteddisplay.cpp
  canvaspainteddisplay.h
  jsonutils.cpp
  jsonutils.h
  WindowData.cpp
  WindowData.h
)

qt5_add_resources(qrc_sources fgqcanvas_resources.qrc)

#qt5_wrap_ui(uic_sources temporarywidget.ui)

add_executable(fgqcanvas ${SOURCES} ${qrc_sources})

set_property(TARGET fgqcanvas PROPERTY AUTOMOC ON)
target_link_libraries(fgqcanvas Qt5::Core Qt5::Widgets Qt5::WebSockets Qt5::Quick)

target_include_directories(fgqcanvas PRIVATE ${PROJECT_SOURCE_DIR})

# so ui_foo.h files are found
target_include_directories(fgqcanvas PRIVATE ${PROJECT_BINARY_DIR})

target_include_directories(fgqcanvas PRIVATE ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
target_include_directories(fgqcanvas PRIVATE ${Qt5Quick_PRIVATE_INCLUDE_DIRS})

install(TARGETS fgqcanvas RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

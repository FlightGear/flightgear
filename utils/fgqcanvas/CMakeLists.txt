cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

include(GNUInstallDirs)

project(FGQCanvas)

find_package(Qt5 5.4 COMPONENTS Widgets WebSockets Gui Quick QuickWidgets)

if (NOT Qt5WebSockets_FOUND OR NOT Qt5QuickWidgets_FOUND)
  message(WARNING "FGQCanvas utility requested, but QtWebSockets/QtQuickWidgets not found")
  message(STATUS "Check you have the development package for Qt5 WebSockets/QuickWidgets installed")
  return()
endif()

set(SOURCES
  temporarywidget.cpp
  temporarywidget.h
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
  fgcanvaswidget.cpp
  fgcanvaswidget.h
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
)

qt5_wrap_ui(uic_sources temporarywidget.ui)

add_executable(fgqcanvas ${SOURCES} ${uic_sources})

set_property(TARGET fgqcanvas PROPERTY AUTOMOC ON)
target_link_libraries(fgqcanvas Qt5::Core Qt5::Widgets Qt5::WebSockets Qt5::Quick Qt5::QuickWidgets)

target_include_directories(fgqcanvas PRIVATE ${PROJECT_SOURCE_DIR})

# so ui_foo.h files are found
target_include_directories(fgqcanvas PRIVATE ${PROJECT_BINARY_DIR})

target_include_directories(fgqcanvas PRIVATE ${Qt5Gui_PRIVATE_INCLUDE_DIRS})

install(TARGETS fgqcanvas RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

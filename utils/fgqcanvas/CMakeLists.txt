cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

project(FGQCanvas)

find_package(Qt5 5.1 COMPONENTS Widgets WebSockets)
if (Qt5Widgets_FOUND)
  message(STATUS "  Qt5 version: ${Qt5Widgets_VERSION_STRING}")
  message(STATUS "  Qt5 include dir: ${Qt5Widgets_INCLUDE_DIRS}")
endif()


set(SOURCES
  temporarywidget.cpp
  main.cpp
  localprop.cpp
  fgcanvaselement.cpp
  fgcanvasgroup.cpp
  fgcanvaspaintcontext.cpp
  fgcanvaspath.cpp
  fgcanvastext.cpp
  fgqcanvasimage.cpp
  fgqcanvasmap.cpp
  fgcanvaswidget.cpp
  canvastreemodel.cpp
)

if (Qt5Widgets_FOUND)
qt5_wrap_ui(uic_sources temporarywidget.ui)

add_executable(fgqcanvas ${SOURCES} ${uic_sources})

set_property(TARGET fgqcanvas PROPERTY AUTOMOC ON)
target_link_libraries(fgqcanvas Qt5::Core Qt5::Widgets Qt5::WebSockets)

target_include_directories(fgqcanvas PRIVATE ${PROJECT_SOURCE_DIR})

# so ui_foo.h files are found
target_include_directories(fgqcanvas PRIVATE ${PROJECT_BINARY_DIR})
endif()

# Dependencies
find_package(OpenXR REQUIRED)

# Old OpenXR SDK versions don't find Threads but Threads::Threads is an
# INTERFACE link library of openxr_loader, so do so explicitly
if(OpenXR_VERSION VERSION_LESS 1.0.14)
    find_package(Threads REQUIRED)
endif()

# Public header files
set(osgXR_HEADERS
    include/osgXR/Action
    include/osgXR/ActionSet
    include/osgXR/CompositionLayer
    include/osgXR/CompositionLayerQuad
    include/osgXR/Export
    include/osgXR/InteractionProfile
    include/osgXR/Manager
    include/osgXR/Mirror
    include/osgXR/MirrorSettings
    include/osgXR/OpenXRDisplay
    include/osgXR/Settings
    include/osgXR/SubImage
    include/osgXR/Subaction
    include/osgXR/Swapchain
    include/osgXR/View
    include/osgXR/osgXR
)

# Source files
set(osgXR_SRCS
    OpenXR/Action.cpp
    OpenXR/ActionSet.cpp
    OpenXR/Compositor.cpp
    OpenXR/DebugUtilsMessenger.cpp
    OpenXR/EventHandler.cpp
    OpenXR/GraphicsBinding.cpp
    OpenXR/Instance.cpp
    OpenXR/InteractionProfile.cpp
    OpenXR/Path.cpp
    OpenXR/Quirks.cpp
    OpenXR/Session.cpp
    OpenXR/Space.cpp
    OpenXR/Swapchain.cpp
    OpenXR/SwapchainGroup.cpp
    OpenXR/System.cpp
    XRFramebuffer.cpp
    XRState.cpp
    XRRealizeOperation.cpp
    XRUpdateOperation.cpp
    Action.cpp
    ActionSet.cpp
    CompositionLayer.cpp
    CompositionLayerQuad.cpp
    DebugCallbackOsg.cpp
    FrameStore.cpp
    InteractionProfile.cpp
    Manager.cpp
    Mirror.cpp
    MirrorSettings.cpp
    OpenXRDisplay.cpp
    Settings.cpp
    Subaction.cpp
    Swapchain.cpp
    View.cpp
    osgXR.cpp
    projection.cpp
)

# Win32 graphics binding
if(WIN32)
    list(APPEND osgXR_SRCS
        OpenXR/GraphicsBindingWin32.cpp
    )
    add_compile_definitions(OSGXR_USE_WIN32)
endif()

# X11 graphics binding
find_package(X11)
if(X11_FOUND)
    list(APPEND osgXR_SRCS
        OpenXR/GraphicsBindingX11.cpp
    )
    add_compile_definitions(OSGXR_USE_X11)
endif()


# Build osgXR as a library
add_library(osgXR ${osgXR_LIBRARY_TYPE} ${osgXR_SRCS})

get_target_property(osgXR_TYPE osgXR TYPE)
if(osgXR_TYPE STREQUAL STATIC_LIBRARY)
    # Needed to switch OSGXR_EXPORT off on Windows
    set(OSGXR_STATIC_LIBRARY 1)
endif()
# Needed to switch OSGXR_EXPORT to dllexport on Windows
add_compile_definitions(OSGXR_LIBRARY)

# Generate a "generated/Version.h" header
set(osgXR_VERSION_HEADER "${PROJECT_BINARY_DIR}/include/generated/Version.h")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Version.h.in"
               "${osgXR_VERSION_HEADER}")

# Generate  "osgXR/Config" header
set(osgXR_CONFIG_HEADER "${PROJECT_BINARY_DIR}/include/osgXR/Config")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Config.in"
               "${osgXR_CONFIG_HEADER}")
list(APPEND osgXR_HEADERS "${osgXR_CONFIG_HEADER}")

# Ensure required C++ standards are available
target_compile_features(osgXR
                        # smart pointers
                        PUBLIC  cxx_std_11
                        # std::optional
                        PRIVATE cxx_std_17
)

# Enable compiler warnings
if(OSGXR_WARNINGS)
    target_compile_options(osgXR PRIVATE
                           $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                             -Wall -pedantic
                             -Wextra -Wno-missing-field-initializers
                                     -Wno-unused-parameter>
                           $<$<CXX_COMPILER_ID:MSVC>:
                             /W4>
    )
endif()

target_include_directories(osgXR
    PRIVATE
        ${PROJECT_BINARY_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
        ${OPENGL_INCLUDE_DIR}
        ${OPENSCENEGRAPH_INCLUDE_DIRS}
        ${OpenXR_INCLUDE_DIR}
    PUBLIC
        "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
        "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_link_libraries(osgXR
    PRIVATE
        ${OPENGL_LIBRARIES}
    PUBLIC
        ${OPENSCENEGRAPH_LIBRARIES}
        OpenXR::openxr_loader
)

set_target_properties(osgXR
    PROPERTIES
        VERSION         ${PROJECT_VERSION}
        SOVERSION       ${osgXR_SOVERSION}
        PUBLIC_HEADER   "${osgXR_HEADERS}"
        INTERFACE_osgXR_MAJOR_VERSION   ${osgXR_MAJOR_VERSION}
        INTERFACE_osgXR_MINOR_VERSION   ${osgXR_MINOR_VERSION}
)
set_property(TARGET osgXR APPEND PROPERTY
    COMPATIBLE_INTERFACE_STRING     osgXR_MAJOR_VERSION osgXR_MINOR_VERSION
)

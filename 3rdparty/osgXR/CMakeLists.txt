# Top level CMakeLists.txt
cmake_minimum_required(VERSION 3.11)

set(osgXR_MAJOR_VERSION 0)
set(osgXR_MINOR_VERSION 5)
set(osgXR_PATCH_VERSION 3)
set(osgXR_SOVERSION 6)

set(osgXR_VERSION "${osgXR_MAJOR_VERSION}.${osgXR_MINOR_VERSION}.${osgXR_PATCH_VERSION}")

project(osgXR
        VERSION     ${osgXR_VERSION}
        DESCRIPTION "OpenXR integration for OpenSceneGraph applications"
)

if(CMAKE_PROJECT_NAME STREQUAL osgXR)
    # Normal top level project build, install package components

    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)

    # Build options
    option(BUILD_SHARED_LIBS "Whether to build as a shared library" ON)
    option(BUILD_OSGXR_EXAMPLES "Enable to build osgXR examples" OFF)
    option(OSGXR_WARNINGS "Enable compiler warnings for osgXR" OFF)

    find_package(OpenGL REQUIRED)
    find_package(OpenSceneGraph REQUIRED COMPONENTS osgViewer osgUtil)

    # Source files in src/
    add_subdirectory(src)

    if(BUILD_OSGXR_EXAMPLES)
        add_subdirectory(examples)
    endif()

    set(INSTALL_INCDIR "${CMAKE_INSTALL_INCLUDEDIR}")

    # Preprocess pkgconfig file
    configure_file(osgXR.pc.in osgXR.pc @ONLY)

    # Preprocess package config
    configure_package_config_file(Config.cmake.in osgXRConfig.cmake
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/osgXR"
        PATH_VARS           INSTALL_INCDIR
    )
    # Build package version file
    write_basic_package_version_file(osgXRConfigVersion.cmake
        VERSION "${PROJECT_VERSION}"
        COMPATIBILITY SameMinorVersion
    )

    # Install library and headers
    install(TARGETS                   osgXR
            EXPORT                    osgXRTargets
            LIBRARY DESTINATION       "${CMAKE_INSTALL_LIBDIR}"
            PUBLIC_HEADER DESTINATION "${INSTALL_INCDIR}/osgXR"
            INCLUDES      DESTINATION "${INSTALL_INCDIR}"
    )
    # Install pkgconfig file
    install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/osgXR.pc"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig"
    )
    # Install export targets
    install(EXPORT      osgXRTargets
            FILE        osgXRTargets.cmake
            NAMESPACE   osgXR::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/osgXR"
    )
    # Install package config and version files
    install(FILES       "${CMAKE_CURRENT_BINARY_DIR}/osgXRConfig.cmake"
                        "${CMAKE_CURRENT_BINARY_DIR}/osgXRConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/osgXR"
    )

else()
    # Subproject build

    # Just build src/ as a static library
    set(osgXR_LIBRARY_TYPE STATIC)
    add_subdirectory(src)

endif()

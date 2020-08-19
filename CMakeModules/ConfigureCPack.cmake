# ConfigureCPack.cmake -- Configure CPack packaging

if(EXISTS ${PROJECT_SOURCE_DIR}/.gitignore)
    file(READ .gitignore CPACK_SOURCE_IGNORE_FILES)
else()
    # clean tar-balls do not contain SCM (.git/.gitignore/...) files.
    set(CPACK_SOURCE_IGNORE_FILES
        "Makefile.am;~$;${CPACK_SOURCE_IGNORE_FILES}")
endif()

list (APPEND CPACK_SOURCE_IGNORE_FILES "${PROJECT_SOURCE_DIR}/.git;\\\\.gitignore")

# split version string into components, note CMAKE_MATCH_0 is the entire regexp match
string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" CPACK_PACKAGE_VERSION ${FLIGHTGEAR_VERSION} )
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1}) 
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_RESOURCE_FILE_README  "${PROJECT_SOURCE_DIR}/README")

set(CPACK_SOURCE_GENERATOR TBZ2)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "flightgear-${FLIGHTGEAR_VERSION}" CACHE INTERNAL "tarball basename")

include (CPack)


set(name fgcom)

find_package(OpenAL REQUIRED)

# Copy positions.txt content in const char* _positionsData[];
file(READ utils/positions.txt POSITIONS_DATA)
string(REGEX REPLACE "\n" "\"%
    \"" POSITIONS_DATA ${POSITIONS_DATA})
string(REGEX REPLACE "%" "," POSITIONS_DATA ${POSITIONS_DATA})
file(WRITE positions.hxx "const char* _positionsData[] = {
    \"")
file(APPEND positions.hxx ${POSITIONS_DATA})
file(APPEND positions.hxx "\"
};")

if(MSVC)
  set(RESOURCE_FILE fgcom.rc)
endif(MSVC)

set(SOURCES fgcom.cxx)
set(HEADERS fgcom.hxx positions.hxx)

add_executable(${name}
  ${SOURCES}
  ${HEADERS}
  ${RESOURCE_FILE}
)

target_link_Libraries(${name}
                      iaxclient_lib
                      ${OPENAL_LIBRARY}
                      SimGearCore
)

install(TARGETS ${name} RUNTIME DESTINATION bin)

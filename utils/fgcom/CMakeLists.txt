set(name fgcom)

# Copy positions.txt content in const char* _positionsData[];
file(READ utils/positions.txt POSITIONS_DATA)
string(REGEX REPLACE "\n" "\"%
\"" POSITIONS_DATA ${POSITIONS_DATA})
string(REGEX REPLACE "%" "," POSITIONS_DATA ${POSITIONS_DATA})

set(out_file ${CMAKE_BINARY_DIR}/positions.hxx)

file(WRITE ${out_file} "const char* _positionsData[] = {\n\"")
file(APPEND ${out_file} ${POSITIONS_DATA})
file(APPEND ${out_file} "\"\n};")

if(MSVC)
  set(RESOURCE_FILE fgcom.rc)
endif(MSVC)

set(SOURCES fgcom_external.cxx)
set(HEADERS fgcom_external.hxx ${out_file})

add_executable(${name}
  ${SOURCES}
  ${HEADERS}
  ${RESOURCE_FILE}
)

target_link_Libraries(${name}
                      iaxclient_lib
                      SimGearCore)

install(TARGETS ${name} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

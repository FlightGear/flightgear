add_executable(fgelev fgelev.cxx)

target_link_libraries(fgelev SimGearScene SimGearCore)

install(TARGETS fgelev RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

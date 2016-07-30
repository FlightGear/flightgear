add_executable(demconvert demconvert.cxx )

target_link_libraries(demconvert
	SimGearScene SimGearCore
    ${GDAL_LIBRARY}
)

install(TARGETS demconvert RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

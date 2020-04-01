
set (FNT_SOURCES
	fnt.cxx
	fntTXF.cxx
	fntBitmap.cxx
)

add_library(PLIBFont STATIC ${FNT_SOURCES})

target_link_libraries(PLIBFont SimGearCore)

target_include_directories(PLIBFont PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})



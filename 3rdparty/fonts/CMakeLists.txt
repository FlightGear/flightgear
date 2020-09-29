
set (FNT_SOURCES
	fnt.cxx
	fntTXF.cxx
	fntBitmap.cxx
)

add_library(PLIBFont STATIC ${FNT_SOURCES})

target_link_libraries(PLIBFont SimGearCore )

# we use <osg/GL> in fnt.h, so we need to use it
target_include_directories(PLIBFont PUBLIC 
	${CMAKE_CURRENT_SOURCE_DIR} 
	${OPENSCENEGRAPH_INCLUDE_DIRS})
target_include_directories(PLIBFont PRIVATE ${PLIB_INCLUDE_DIR})

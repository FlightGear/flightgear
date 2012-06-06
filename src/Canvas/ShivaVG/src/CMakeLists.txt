include(CheckIncludeFile)
check_include_file("inttypes.h" HAVE_INTTYPES_H)

if(HAVE_INTTYPES_H)
  add_definitions(-DHAVE_INTTYPES_H)
endif()
add_definitions(-DVG_API_EXPORT)

INCLUDE_DIRECTORIES(
  ${OPENGL_INCLUDE_DIR}
  ../include
)

SET(INCROOT ../include/VG)
SET(SRCROOT .)

SET(ShivaVG_Src
    ${SRCROOT}/shArrays.c
    ${SRCROOT}/shArrays.h
    ${SRCROOT}/shContext.c
    ${SRCROOT}/shContext.h
    ${SRCROOT}/shExtensions.c
    ${SRCROOT}/shExtensions.h
    ${SRCROOT}/shGeometry.c
    ${SRCROOT}/shGeometry.h
    ${SRCROOT}/shImage.c
    ${SRCROOT}/shImage.h
    ${SRCROOT}/shPaint.c
    ${SRCROOT}/shPaint.h
    ${SRCROOT}/shParams.c
    ${SRCROOT}/shPath.c
    ${SRCROOT}/shPath.h
    ${SRCROOT}/shPipeline.c
    ${SRCROOT}/shVectors.c
    ${SRCROOT}/shVectors.h
    ${SRCROOT}/shVgu.c
)

ADD_LIBRARY(
    ShivaVG
    ${ShivaVG_Src}
)

TARGET_LINK_LIBRARIES(
    ShivaVG
    ${OPENGL_gl_LIBRARY}
    ${OPENGL_glu_LIBRARY}
)
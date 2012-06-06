#SET(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")

ADD_DEFINITIONS(
    -DVG_API_EXPORT
    -DHAVE_INTTYPES_H
)

INCLUDE_DIRECTORIES(
    ${OPENGL_INCLUDE_DIR}
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

#INSTALL(
#    TARGETS OpenVG
#    RUNTIME DESTINATION bin COMPONENT bin
#    LIBRARY DESTINATION lib${LIB_SUFFIX} COMPONENT bin
#    ARCHIVE DESTINATION lin${LIB_SUFFIX} COMPONENT devel
#)


set(SOURCES
    fgtraffic.cxx
        )

set(HEADERS
    fgtraffic.hxx
        )


add_executable(fgtraffic
    ${SOURCES} ${HEADERS}
)


install(TARGETS fgtraffic RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

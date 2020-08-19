
if (APPLE)
    install(TARGETS fgfs BUNDLE DESTINATION .)
else()
    install(TARGETS fgfs RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

if (TARGET sentry_crashpad::handler)
    if (APPLE)
        # install inside the bundle
        install(FILES $<TARGET_FILE:sentry_crashpad::handler> DESTINATION fgfs.app/Contents/MacOS OPTIONAL)
    else()
        # install in the bin-dir, next to the application binary
        install(FILES $<TARGET_FILE:sentry_crashpad::handler> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
    endif()
endif()

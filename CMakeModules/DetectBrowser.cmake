# DetectBrowser.cmake -- Detect web browser launcher application

# Set default command to open browser. Override with -DWEB_BROWSER=...
if (APPLE OR MSVC)
    # opening the web browser is hardcoded for Mac and Windows,
    # so this doesn't really have an effect...
    set(WEB_BROWSER "open")
else()
    if(CMAKE_SYSTEM_NAME MATCHES "Linux")
        # "xdg-open" provides run-time detection of user's preferred browser on (most) Linux.
        if (NOT LINUX_DISTRO MATCHES "Debian")
            set(WEB_BROWSER "xdg-open" CACHE STRING "Command to open web browser")
        else()
            # Debian is different: "sensible-browser" provides auto-detection
            set(WEB_BROWSER "sensible-browser" CACHE STRING "Command to open web browser")
        endif()
    else()
        # Default for non Linux/non Mac/non Windows platform...
        set(WEB_BROWSER "firefox" CACHE STRING "Command to open web browser")
    endif()
    message(STATUS "Web browser launcher command is: ${WEB_BROWSER}")
endif()

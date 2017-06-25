if(NOT WIN32) # Don't install man pages on Windows
    add_subdirectory(it)
    add_subdirectory(man1)
    add_subdirectory(man5)
endif(NOT WIN32)

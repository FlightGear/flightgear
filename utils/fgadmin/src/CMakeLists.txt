
fltk_wrap_ui(FGAdminUI fgadmin.fl)
add_library(FGAdminUI ${FGAdminUI_FLTK_UI_SRCS})

add_executable(fgadmin fgadmin_funcs.cxx main.cxx untarka.c)
add_dependencies(fgadmin FGAdminUI)

include_directories(${FLTK_INCLUDE_DIR})

target_link_libraries(fgadmin FGAdminUI
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_LIBRARIES}
	${FLTK_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	${ZLIB_LIBRARY}
)

install(TARGETS fgadmin RUNTIME DESTINATION bin)

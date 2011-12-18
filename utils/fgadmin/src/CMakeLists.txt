
fltk_wrap_ui(FGAdminUI fgadmin.fl)
add_library(FGAdminUI ${FGAdminUI_FLTK_UI_SRCS})

add_executable(fgadmin fgadmin_funcs.cxx main.cxx untarka.c)
add_dependencies(fgadmin FGAdminUI)

target_link_libraries(fgadmin FGAdminUI
	${SIMGEAR_CORE_LIBRARIES}
	${PLIB_LIBRARIES}
	${FLTK_LIBRARIES}
	${SIMGEAR_CORE_LIBRARY_DEPENDENCIES}
	${X11_Xinerama_LIB}
	${X11_Xft_LIB}
)

install(TARGETS fgadmin RUNTIME DESTINATION bin)

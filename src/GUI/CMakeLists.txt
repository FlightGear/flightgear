include(FlightGearComponent)

set(SOURCES
	AirportList.cxx
	CanvasWidget.cxx
	MapWidget.cxx
	WaypointList.cxx
	dialog.cxx
	FGPUIDialog.cxx
	fonts.cxx
	gui.cxx
	gui_funcs.cxx
	layout-props.cxx
	layout.cxx
	menubar.cxx
	FGPUIMenuBar.cxx
	new_gui.cxx
	property_list.cxx
	FGFontCache.cxx
	FGColor.cxx
    FileDialog.cxx
    PUIFileDialog.cxx
    MouseCursor.cxx
    MessageBox.cxx
	)

set(HEADERS
	AirportList.hxx
	CanvasWidget.hxx
	MapWidget.hxx
	WaypointList.hxx
	dialog.hxx
	FGPUIDialog.hxx
	gui.h
	layout.hxx
	menubar.hxx
	FGPUIMenuBar.hxx
	new_gui.hxx
	property_list.hxx
	FGFontCache.hxx
	FGColor.hxx
    FileDialog.hxx
    PUIFileDialog.hxx
    MouseCursor.hxx
    MessageBox.hxx
	)

if(WIN32)
	list(APPEND HEADERS WindowsMouseCursor.hxx
						FGWindowsMenuBar.hxx
						WindowsFileDialog.hxx)
	list(APPEND SOURCES WindowsMouseCursor.cxx
						FGWindowsMenuBar.cxx
						WindowsFileDialog.cxx)
endif()

if (APPLE)
    list(APPEND HEADERS FGCocoaMenuBar.hxx
        CocoaFileDialog.hxx
        CocoaMouseCursor.hxx
        CocoaHelpers.h
        CocoaHelpers_private.h)
    list(APPEND SOURCES FGCocoaMenuBar.mm
        CocoaFileDialog.mm
        CocoaMouseCursor.mm
        CocoaHelpers.mm)
endif()




if (HAVE_QT)
    qt5_wrap_ui(uic_sources Launcher.ui EditRatingsFilterDialog.ui)
    qt5_add_resources(qrc_sources resources.qrc)

    include_directories(${PROJECT_BINARY_DIR}/src/GUI)

    add_library(fglauncher QtLauncher.cxx
                            QtLauncher.hxx
                            AirportDiagram.cxx
                            AirportDiagram.hxx
                            EditRatingsFilterDialog.cxx
                            EditRatingsFilterDialog.hxx
                            ${uic_sources}
                            ${qrc_sources})

    set_property(TARGET fglauncher PROPERTY AUTOMOC ON)
    target_link_libraries(fglauncher Qt5::Core Qt5::Widgets )


endif()


flightgear_component(GUI "${SOURCES}" "${HEADERS}")

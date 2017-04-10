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
    qt5_wrap_ui(uic_sources Launcher.ui
                            EditRatingsFilterDialog.ui
                            SetupRootDialog.ui
                            AddCatalogDialog.ui
                            PathsDialog.ui
                            LocationWidget.ui
                            NoOfficialHangar.ui
                            InstallSceneryDialog.ui
                            UpdateAllAircraft.ui
                            )
    qt5_add_resources(qrc_sources resources.qrc)


    add_library(fglauncher QtLauncher.cxx
                            QtLauncher.hxx
                            LauncherMainWindow.hxx
                            LauncherMainWindow.cxx
                            BaseDiagram.cxx
                            BaseDiagram.hxx
                            AirportDiagram.cxx
                            AirportDiagram.hxx
                            NavaidDiagram.cxx
                            NavaidDiagram.hxx
                            EditRatingsFilterDialog.cxx
                            EditRatingsFilterDialog.hxx
                            ExtraSettingsSection.cxx
                            ExtraSettingsSection.hxx
                            SetupRootDialog.cxx
                            SetupRootDialog.hxx
                            AircraftItemDelegate.hxx
                            AircraftItemDelegate.cxx
                            AircraftModel.hxx
                            AircraftModel.cxx
                            CatalogListModel.cxx
                            CatalogListModel.hxx
                            AddCatalogDialog.cxx
                            AddCatalogDialog.hxx
                            PathsDialog.cxx
                            PathsDialog.hxx
                            LocationWidget.cxx
                            LocationWidget.hxx
                            QtMessageBox.cxx
                            QtMessageBox.hxx
                            QtFileDialog.cxx
                            QtFileDialog.hxx
                            InstallSceneryDialog.hxx
                            InstallSceneryDialog.cxx
                            PreviewWindow.cxx
                            PreviewWindow.hxx
                            SettingsSection.cxx
                            SettingsSection.hxx
                            SettingsSectionQML.cxx
                            SettingsSectionQML.hxx
                            AdvancedSettingsButton.h
                            AdvancedSettingsButton.cpp
                            ToolboxButton.cpp
                            ToolboxButton.h
                            LauncherArgumentTokenizer.cxx
                            LauncherArgumentTokenizer.hxx
                            AircraftSearchFilterModel.cxx
                            AircraftSearchFilterModel.hxx
                            DefaultAircraftLocator.cxx
                            DefaultAircraftLocator.hxx
                            SettingsWidgets.cxx
                            SettingsWidgets.hxx
                            LaunchConfig.cxx
                            LaunchConfig.hxx
                            ViewCommandLinePage.cxx
                            ViewCommandLinePage.hxx
                            MPServersModel.cpp
                            MPServersModel.h
                            ${uic_sources}
                            ${qrc_sources})

    set_property(TARGET fglauncher PROPERTY AUTOMOC ON)
    target_link_libraries(fglauncher Qt5::Core Qt5::Widgets Qt5::Network Qt5::Qml SimGearCore)
    target_include_directories(fglauncher PRIVATE ${PROJECT_BINARY_DIR}/src/GUI)
    #target_include_directories(fglauncher PRIVATE ${Qt5Qml_PRIVATE_INCLUDE_DIRS})


endif()


flightgear_component(GUI "${SOURCES}" "${HEADERS}")

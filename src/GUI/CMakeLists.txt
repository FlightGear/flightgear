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
    qt5_wrap_ui(uic_sources SetupRootDialog.ui
                            InstallSceneryDialog.ui
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
                            SetupRootDialog.cxx
                            SetupRootDialog.hxx
                            AircraftModel.hxx
                            AircraftModel.cxx
                            CatalogListModel.cxx
                            CatalogListModel.hxx
                            QtMessageBox.cxx
                            QtMessageBox.hxx
                            QtFileDialog.cxx
                            QtFileDialog.hxx
                            InstallSceneryDialog.hxx
                            InstallSceneryDialog.cxx
                            LocationController.cxx
                            LocationController.hxx
                            LauncherArgumentTokenizer.cxx
                            LauncherArgumentTokenizer.hxx
                            AircraftSearchFilterModel.cxx
                            AircraftSearchFilterModel.hxx
                            DefaultAircraftLocator.cxx
                            DefaultAircraftLocator.hxx
                            LaunchConfig.cxx
                            LaunchConfig.hxx
                            MPServersModel.cpp
                            MPServersModel.h
                            PathUrlHelper.cxx
                            PathUrlHelper.hxx
                            RecentAircraftModel.cxx
                            RecentAircraftModel.hxx
                            RecentLocationsModel.cxx
                            RecentLocationsModel.hxx
                            LauncherController.cxx
                            LauncherController.hxx
                            AddOnsController.cxx
                            AddOnsController.hxx
                            PixmapImageItem.cxx
                            PixmapImageItem.hxx
                            ${uic_sources}
                            ${qrc_sources}
                            ${qml_sources})

    set_property(TARGET fglauncher PROPERTY AUTOMOC ON)
    target_link_libraries(fglauncher Qt5::Core Qt5::Widgets Qt5::Network Qt5::Qml Qt5::Quick Qt5::Svg SimGearCore)
    target_include_directories(fglauncher PRIVATE ${PROJECT_BINARY_DIR}/src/GUI)

    add_library(fgqmlui QQuickDrawable.cxx
                        QQuickDrawable.hxx                    
                        QtQuickFGCanvasItem.cxx
                        QtQuickFGCanvasItem.hxx
                        PropertyItemModel.cxx
                        PropertyItemModel.hxx
                        FGQmlInstance.cxx
                        FGQmlInstance.hxx
                        FGQmlPropertyNode.cxx
                        FGQmlPropertyNode.hxx
                        QmlAircraftInfo.cxx
                        QmlAircraftInfo.hxx
                        LocalAircraftCache.cxx
                        LocalAircraftCache.hxx
                        PreviewImageItem.cxx
                        PreviewImageItem.hxx
                        ThumbnailImageItem.cxx
                        ThumbnailImageItem.hxx
                        PopupWindowTracker.cxx
                        PopupWindowTracker.hxx
                        QmlPositioned.hxx
                        QmlPositioned.cxx
                        QmlNavCacheWrapper.hxx
                        QmlNavCacheWrapper.cxx
                        QmlRadioButtonHelper.cxx
                        QmlRadioButtonHelper.hxx
                        UnitsModel.cxx
                        UnitsModel.hxx
                        )

    set_property(TARGET fgqmlui PROPERTY AUTOMOC ON)
    target_link_libraries(fgqmlui Qt5::Quick Qt5::Network Qt5::Qml SimGearCore)
    target_include_directories(fgqmlui PRIVATE ${PROJECT_BINARY_DIR}/src/GUI)
endif()


flightgear_component(GUI "${SOURCES}" "${HEADERS}")

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

    if (TARGET fgfs_qm_files)
      qt5_add_resources(qrc_sources ${PROJECT_BINARY_DIR}/translations.qrc)
    endif()

    add_library(fglauncher QtLauncher.cxx
                            QtLauncher.hxx
                            LauncherMainWindow.hxx
                            LauncherMainWindow.cxx
                            BaseDiagram.cxx
                            BaseDiagram.hxx
                            AirportDiagram.cxx
                            AirportDiagram.hxx
                            CarrierDiagram.cxx
                            CarrierDiagram.hxx
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
                            RecentAircraftModel.cxx
                            RecentAircraftModel.hxx
                            RecentLocationsModel.cxx
                            RecentLocationsModel.hxx
                            LauncherController.cxx
                            LauncherController.hxx
                            AddOnsController.cxx
                            AddOnsController.hxx
                            AddonsModel.cxx
                            AddonsModel.hxx
                            PixmapImageItem.cxx
                            PixmapImageItem.hxx
                            PathListModel.cxx
                            PathListModel.hxx
                            CarriersLocationModel.cxx
                            CarriersLocationModel.hxx
                            FavouriteAircraftData.cxx
                            FavouriteAircraftData.hxx
                            UpdateChecker.cxx
                            UpdateChecker.hxx
                            LauncherPackageDelegate.hxx
                            LauncherPackageDelegate.cxx
                            LauncherNotificationsController.hxx
                            LauncherNotificationsController.cxx
                            ${uic_sources}
                            ${qrc_sources}
                            ${qml_sources})

    set_property(TARGET fglauncher PROPERTY AUTOMOC ON)
    target_include_directories(fglauncher PRIVATE ${PROJECT_BINARY_DIR}/src/GUI)
    target_link_libraries(fglauncher Qt5::Core Qt5::Widgets Qt5::Network Qt5::Qml Qt5::Quick Qt5::Svg SimGearCore)

    if (ENABLE_QQ_UI)
      set(QQUI_SOURCES 
        QQuickDrawable.cxx
        QQuickDrawable.hxx)
    endif()

    add_library(fgqmlui 
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
                        QmlPropertyModel.hxx
                        QmlPropertyModel.cxx
                        QmlPositioned.hxx
                        QmlPositioned.cxx
                        QmlNavCacheWrapper.hxx
                        QmlNavCacheWrapper.cxx
                        QmlRadioButtonHelper.cxx
                        QmlRadioButtonHelper.hxx
                        QtHelpers.hxx
                        QtHelpers.cxx
                        UnitsModel.cxx
                        UnitsModel.hxx
                        NavaidSearchModel.hxx
                        NavaidSearchModel.cxx
                        FlightPlanController.cxx
                        FlightPlanController.hxx
                        RouteDiagram.cxx
                        RouteDiagram.hxx
                        StackController.cxx
                        StackController.hxx
                        ModelDataExtractor.cxx
                        ModelDataExtractor.hxx
                        HoverArea.cxx
                        HoverArea.hxx
                        FGQQWindowManager.cxx
                        FGQQWindowManager.hxx
                        PathUrlHelper.cxx
                        PathUrlHelper.hxx
                        DialogStateController.cxx
                        DialogStateController.hxx
                        ${QQUI_SOURCES}
                        )

    set_property(TARGET fgqmlui PROPERTY AUTOMOC ON)
    target_link_libraries(fgqmlui Qt5::Quick Qt5::Widgets Qt5::Network Qt5::Qml Qt5::Gui SimGearCore)
    target_include_directories(fgqmlui PRIVATE ${PROJECT_BINARY_DIR}/src/GUI)

    if (ENABLE_QQ_UI)
      # this is a headers-only dependency, so we can include <private/qopenglcontext_p.h>
      target_link_libraries(fgqmlui Qt5::GuiPrivate)
    endif()

    if (TARGET fgfs_qm_files)
      add_dependencies(fglauncher fgfs_qm_files)
      add_dependencies(fgqmlui fgfs_qm_files)
    endif()

endif()


flightgear_component(GUI "${SOURCES}" "${HEADERS}")

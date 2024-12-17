include(FlightGearComponent)

set(SOURCES
 
  dialog.cxx
  gui.cxx
  gui_funcs.cxx
  layout-props.cxx
  layout.cxx
  menubar.cxx
  new_gui.cxx
  FGFontCache.cxx
  FGColor.cxx
    FileDialog.cxx
    MouseCursor.cxx
    MessageBox.cxx
    Highlight.cxx
  )

set(HEADERS
  dialog.hxx
  gui.h
  layout.hxx
  menubar.hxx
  new_gui.hxx
  FGFontCache.hxx
  FGColor.hxx
    FileDialog.hxx
    MouseCursor.hxx
    MessageBox.hxx
    Highlight.hxx
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

if (HAVE_PUI)
  list(APPEND HEADERS 
    FGPUIMenuBar.hxx
    FGPUIDialog.hxx
    PUIFileDialog.hxx
    AirportList.hxx
    CanvasWidget.hxx
    MapWidget.hxx
    WaypointList.hxx
    property_list.hxx
  )

  list(APPEND SOURCES
    FGPUIMenuBar.cxx
    FGPUIDialog.cxx
    PUIFileDialog.cxx
    AirportList.cxx
    CanvasWidget.cxx
    MapWidget.cxx
    WaypointList.cxx
    fonts.cxx
    property_list.cxx
  )
endif()

if (HAVE_QT)
    if (QT_VERSION_MAJOR EQUAL "5")
      message(STATUS "Using Qt5 version of the macros")
      # Qt < 5.15 
      qt5_wrap_ui(uic_sources SetupRootDialog.ui
        InstallSceneryDialog.ui
        )
      qt5_add_resources(qrc_sources resources.qrc)

      if (TARGET fgfs_qm_files)
        qt5_add_resources(qrc_sources ${PROJECT_BINARY_DIR}/translations.qrc)
      endif()
    else()
      message(STATUS "Using Qt6 version of the macros")
      # Qt 6 versions
      qt_wrap_ui(uic_sources SetupRootDialog.ui
        InstallSceneryDialog.ui
        )
      qt_add_resources(qrc_sources resources.qrc)

      if (TARGET fgfs_qm_files)
        qt_add_resources(qrc_sources ${PROJECT_BINARY_DIR}/translations.qrc)
      endif()
    endif()



    add_library(fglauncher QtLauncher.cxx
                            QtLauncher.hxx
                            SettingsWrapper.hxx
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
                            AircraftItemModel.hxx
                            AircraftItemModel.cxx
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
                            AircraftProxyModel.cxx
                            AircraftProxyModel.hxx
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
    target_include_directories(fglauncher PRIVATE ${PROJECT_BINARY_DIR}/src/GUI 
      ${CMAKE_CURRENT_SOURCE_DIR})

    # we include WindowBuilder.h which needs this
    target_include_directories(fglauncher PRIVATE ${OPENSCENEGRAPH_INCLUDE_DIRS})

    target_link_libraries(fglauncher 
        Qt${QT_VERSION_MAJOR}::Core 
        Qt${QT_VERSION_MAJOR}::Widgets 
        Qt${QT_VERSION_MAJOR}::Network 
        Qt${QT_VERSION_MAJOR}::Qml 
        Qt${QT_VERSION_MAJOR}::Quick 
        Qt${QT_VERSION_MAJOR}::Svg 
        SimGearCore)

    add_library(fgqmlui 
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
                        QmlPositionedModel.cxx
                        QmlPositionedModel.hxx
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
                        PathUrlHelper.cxx
                        PathUrlHelper.hxx
                        DialogStateController.cxx
                        DialogStateController.hxx
                        GettingStartedTip.hxx
                        GettingStartedTip.cxx
                        GettingStartedTipsController.cxx
                        GettingStartedTipsController.hxx
                        TipBackgroundBox.cxx
                        TipBackgroundBox.hxx
                        GettingStartedScope.hxx
                        GettingStartedScope.cxx
                        QmlColoredImageProvider.hxx
                        QmlColoredImageProvider.cxx
                        QmlStringListModel.hxx
                        QmlStringListModel.cxx
                        )

    set_property(TARGET fgqmlui PROPERTY AUTOMOC ON)
    target_link_libraries(fgqmlui 
        Qt${QT_VERSION_MAJOR}::Widgets 
        Qt${QT_VERSION_MAJOR}::Network 
        Qt${QT_VERSION_MAJOR}::Qml 
        Qt${QT_VERSION_MAJOR}::Quick 
        Qt${QT_VERSION_MAJOR}::Svg 
        SimGearCore)

    target_include_directories(fgqmlui PRIVATE ${PROJECT_BINARY_DIR}/src/GUI ${OPENSCENEGRAPH_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR})
    
    if (TARGET fgfs_qm_files)
      add_dependencies(fglauncher fgfs_qm_files)
      add_dependencies(fgqmlui fgfs_qm_files)
    endif()

endif()


flightgear_component(GUI "${SOURCES}" "${HEADERS}")

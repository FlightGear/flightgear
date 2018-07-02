import QtQuick 2.4
import "."

Item {
    id: root
    // order of this model sets the order of buttons in the sidebar
    ListModel {
        id: pagesModel
        ListElement { title: qsTr("Summary"); pageSource: "qrc:///qml/Summary.qml"; iconPath: "qrc:///svg/toolbox-summary"; state:"loader" }
        ListElement { title: qsTr("Aircraft"); pageSource: "qrc:///qml/AircraftList.qml"; iconPath: "qrc:///svg/toolbox-aircraft"; state:"loader" }
        
        ListElement { 
            title: qsTr("Location"); pageSource: "qrc:///qml/Location.qml"; 
            iconPath: "qrc:///toolbox-location"; state:"loader"
            buttonDisabled: false
            disabledText: qsTr("Location page disabled due to conflicting user arguments (in Settings)");
        }

        // due to some design stupidity by James, we can't use the Loader mechanism for these pages; they need to exist
        // permanently so that collecting args works. So we instantiate them down below, and toggle the visiblity
        // of them and the loader using a state.

        ListElement { title: qsTr("Environment"); pageSource: ""; iconPath: "qrc:///svg/toolbox-environment"; state:"environment"  }
        ListElement { title: qsTr("Settings"); pageSource: ""; iconPath: "qrc:///svg/toolbox-settings"; state:"settings" }

        ListElement { title: qsTr("Add-ons"); pageSource: "qrc:///qml/AddOns.qml"; iconPath: "qrc:///svg/toolbox-addons"; state:"loader" }

    }

    Component.onCompleted:
    {
       _launcher.minimumWindowSize = Qt.size(Style.strutSize * 12, sidebar.minimumHeight);
    }

    Connections {
        target: _location
        onSkipFromArgsChanged: pagesModel.setProperty(2, "buttonDisabled", _location.skipFromArgs)
    }

    states: [
        State {
            name: "loader"
            PropertyChanges { target: pageLoader; visible: true }
            PropertyChanges { target: settings; visible: false }
            PropertyChanges { target: environment; visible: false }
        },

        State {
            name: "settings"
            PropertyChanges { target: pageLoader; visible: false }
            PropertyChanges { target: settings; visible: true }
            PropertyChanges { target: environment; visible: false }
        },

        State {
            name: "environment"
            PropertyChanges { target: pageLoader; visible: false }
            PropertyChanges { target: settings; visible: false }
            PropertyChanges { target: environment; visible: true }
        }
    ]

    Connections {
        target: _launcher
        onViewCommandLine: {
            sidebar.selectedPage = -1;
            pageLoader.source = "qrc:///qml/ViewCommandLine.qml"
            root.state = "loader";
        }
    }

    Sidebar {
        id: sidebar
        width: Style.strutSize * 2
        height: parent.height
        z: 1
        pagesModel: pagesModel
        selectedPage: 0 // open on the summary page

        onShowMenu: menu.show();

        onSelectPage: {
            pageLoader.source = pageSource
            root.state = pagesModel.get(selectedPage).state
        }
    }

    Settings {
        id: settings

        height: parent.height
        anchors {
            left: sidebar.right
            right: parent.right
        }
    }

    Environment {
        id: environment

        height: parent.height
        anchors {
            left: sidebar.right
            right: parent.right
        }
    }

    Loader {
        id: pageLoader
        height: parent.height
        anchors {
            left: sidebar.right
            right: parent.right
        }

        source: "qrc:///qml/Summary.qml"
    }

    function selectPage(index)
    {
        sidebar.setSelectedPage(index);
        var page = pagesModel.get(index);
        pageLoader.source = page.pageSource
        root.state = page.state
    }

    Connections {
        target: pageLoader.item
        ignoreUnknownSignals: true
        onShowSelectedAircraft: root.selectPage(1)
        onShowSelectedLocation: root.selectPage(2)
    }

    Menu {
        id: menu
        z: 100

        items: [
            MenuItem { text:qsTr("Open saved configuration..."); onTriggered: _launcher.openConfig(); },
            MenuItem { text:qsTr("Save configuration as..."); onTriggered: _launcher.saveConfigAs(); },
            MenuDivider {},
            MenuItem { text:qsTr("View command line"); onTriggered: _launcher.viewCommandLine(); },
            MenuItem { text:qsTr("Select data files location..."); onTriggered: _launcher.requestChangeDataPath(); },
            MenuItem { text:qsTr("Restore default settings..."); onTriggered: _launcher.requestRestoreDefaults(); },
            MenuDivider {},
            MenuItem { text:qsTr("Quit"); shortcut: "Ctrl+Q"; onTriggered: _launcher.quit();  } 
        ]
    }


}

import QtQuick 2.0

Item {
    id: root
    property int activeTab: -1
    property var tabs: []
    property var titles: []
    property int __panelWidth: 250

    readonly property int panelWidth: __panelWidth

    Rectangle {
        id: contentBox
        x: parent.width - width // can't use an anchors, for dragability

        height: parent.height
        width: (activeTab >= 0) ? __panelWidth : 0
        color: "#1f1f1f"
        Behavior on width {
            enabled: !splitterDrag.drag.active
            NumberAnimation {
                duration: 250
            }
        }

        clip: true

        Loader {
            id: loader
            anchors.fill: parent
            sourceComponent: (activeTab >= 0) ? tabs[activeTab] : null
        }

        Connections {
            target: loader.item
            function onRequestPanelClose() { root.activeTab = -1; }
            ignoreUnknownSignals: true
        }
    }

    Rectangle {
        id: splitter
        width: 2
        height: parent.height
        anchors.right: contentBox.left

        color: "orange"

        MouseArea {
            id: splitterDrag
            height: parent.height
            width: parent.width * 25

            drag.target: contentBox
            drag.axis: Drag.XAxis
            drag.minimumX: 0
            drag.maximumX: root.width

            onMouseXChanged: {
                __panelWidth = root.width - contentBox.x
            }

            // enabled when open?

            // click to toggle open / close

            cursorShape: Qt.SizeHorCursor
        }
    }

    Column {
        anchors.right: splitter.left
        anchors.top: parent.top
        anchors.topMargin: 20
        spacing: 1

        Repeater {
            model: tabs
            Rectangle {
                readonly property bool tabIsActive: (model.index == activeTab)
                width: tabLabel.implicitHeight + 10
                height: tabLabel.implicitWidth + 20

                color: "#3f3f3f"
                border.width: 1
                border.color: "#5f5f5f"

                Text {
                    anchors.centerIn: parent
                    id: tabLabel
                    text: titles[model.index]
                    rotation: 90
                    color: tabIsActive ? "orange" : tabMouse.containsMouse ? "#afafaf" : "#9f9f9f"
                    transformOrigin: Item.Center
                }

                MouseArea {
                    id: tabMouse
                    hoverEnabled: true
                    anchors.fill: parent
                    onClicked: {
                        if (parent.tabIsActive) {
                            activeTab = -1
                        } else {
                            activeTab = model.index
                        }
                    }
                }
            } // rectangle
        } // of repeater
    }
}

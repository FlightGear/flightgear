import QtQuick 2.0
import FlightGear 1.0 as FG

Item {
    Rectangle {
        id: hostPanel
        height: hostPanelContent.childrenRect.height + 16
        width: parent.width - 8

        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter

        border.color: "#9f9f9f"
        border.width: 1
        color: "#5f5f5f"
        opacity: 0.8

        Column {
            spacing: 8

            id: hostPanelContent
            width: parent.width - 30
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter

            InputLine {
                id: hostInput
                width: parent.width
                label: "Hostname"
                text: _application.host
                onEditingFinished: {
                    _application.host = text
                    portInput.forceActiveFocus();
                }

                KeyNavigation.tab: portInput
            }

            InputLine {
                id: portInput
                width: parent.width
                label: "Port"
                text: _application.port
                KeyNavigation.tab: queryButton

                onEditingFinished: {
                    _application.port = text
                }
            }

            Button {
                id: queryButton
                label: "Query"
                enabled: _application.host != ""
                visible: (_application.status == FG.Application.Idle)

                onClicked: {
                    _application.query();
                }
            }

            Button {
                id: cancelButton
                label: "Cancel"
                anchors.right: parent.right

                visible: (_application.status == FG.Application.Querying)

                onClicked: {
                    _application.cancelQuery();
                }
            }

            Button {
                id: clearlButton
                label: "Clear"
                anchors.right: parent.right

                visible: (_application.status == FG.Application.SuccessfulQuery) |
                         (_application.status == FG.Application.QueryFailed)

                onClicked: {
                    _application.clearQuery();
                }
            }

        }
    }

    Rectangle {
        id: canvasListPanel
        border.color: "#9f9f9f"
        border.width: 1
        color: "#5f5f5f"
        opacity: 0.8
        anchors.top: hostPanel.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        width: 300
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8
        visible: _application.canvases.length > 0

        ListView {
            id: canvasList
            model: _application.canvases
            visible: (_application.status == FG.Application.SuccessfulQuery)
            width: parent.width - 30
            height: parent.height
            anchors.horizontalCenter: parent.horizontalCenter

            delegate: Rectangle {
                width: canvasLabel.implicitWidth
                height: canvasLabel.implicitHeight + 20

                color: "#3f3f3f"

                Text {
                    id: canvasLabel
                    text: modelData['name']

                    // todo - different color if we already have a connection?
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter


                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        _application.openCanvas(modelData['path']);
                    }
                }
            }
        }
    }
}

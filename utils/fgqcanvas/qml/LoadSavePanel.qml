import QtQuick 2.0
import FlightGear 1.0 as FG

Item {

    Rectangle {
        id: savePanel
        width: parent.width - 8

        anchors.top: parent.top
        anchors.topMargin: 8
        anchors.bottom:  parent.bottom
        anchors.bottomMargin: 8

        anchors.horizontalCenter: parent.horizontalCenter

        border.color: "#9f9f9f"
        border.width: 1
        color: "#5f5f5f"
        opacity: 0.8

        Column {
            spacing: 8

            id: savePanelContent
            width: parent.width - 30
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter

            InputLine {
                id: saveTitleInput
                width: parent.width
                label: "Title"

            }

            Button {
                id: saveButton
                label: "Save"
                enabled: (saveTitleInput.text != "")

                onClicked: {
                    _application.save(saveTitleInput.text);
                }
            }
        }

        ListView {
            id: savedList
            model: _application.configs
            width: parent.width - 30
            anchors.top: savePanelContent.bottom
            anchors.topMargin: 8
            anchors.bottom:  parent.bottom
            anchors.bottomMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter

            delegate: Item {
                width: parent.width
                height:delegateFrame.height + 8

                Rectangle {
                    id: delegateFrame
                    width: parent.width
                    anchors.horizontalCenter: parent.horizontalCenter

                    height: configLabel.implicitHeight + 20

                    color: "#3f3f3f"

                    Text {
                        id: configLabel
                        text: modelData['name']
                        color: "white"
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 8
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            _application.restoreConfig(model.index)
                        }
                    }

                    Button {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        label: "X"
                        width: height

                        onClicked:  {

                        }
                    }
                } // of visible rect
            } // of delegate item

        }
    } // of frame rect
}

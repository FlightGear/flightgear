import QtQuick 2.0
import FlightGear 1.0 as FG

Item {
    id: root
    signal requestPanelClose();

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
        layer.enabled: true

        InputLine {
            id: saveTitleInput
            width: parent.width
            label: "Title"
            anchors {
                top: parent.top
                topMargin: 8
                left: parent.left
                leftMargin: 8
                right: saveButton.left
                rightMargin: 8
            }

        }

        Button {
            id: saveButton
            label: "Save"
            enabled: (saveTitleInput.text != "")
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.top: parent.top
            anchors.topMargin: 8

            onClicked: {
                _application.saveSnapshot(saveTitleInput.text);
            }
        }


        ListView {
            id: savedList
            model: _application.snapshots
            width: parent.width - 30
            anchors.top: saveTitleInput.bottom
            anchors.topMargin: 8
            anchors.bottom:  parent.bottom
            anchors.bottomMargin: 8
            anchors.horizontalCenter: parent.horizontalCenter

            delegate: Item {
                width: parent.width
                height:delegateFrame.height + 8

                Rectangle {
                    id: delegateBackFrame
                    color: "#1f1f1f"
                    width: delegateFrame.width
                    height: delegateFrame.height


                    Button {
                        id: deleteButton
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 8
                        label: "Delete"
                        onClicked:  {
                           // _application.deleteConfig(model.index)
                          //  _application.deleteConfig
                        }
                    }
                }

                Rectangle {
                    id: delegateFrame
                    width: parent.width

                    height: configLabel.implicitHeight + 20

                    opacity: 1.0
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
                            _application.restoreSnapshot(model.index);
                            root.requestPanelClose();
                        }

                        drag.target: delegateFrame
                        drag.axis: Drag.XAxis
                        drag.minimumX: -delegateFrame.width
                        drag.maximumX: 0
                    }


                } // of visible rect
            } // of delegate item

        }
    } // of frame rect
}

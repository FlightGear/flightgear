import QtQuick 2.0

Item {
    id: root
    property alias label: label.text

    property var choices: []
    property string displayRole: ""

    property int currentIndex: 0

    Text {
        id: label
        anchors.left: root.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        color: mouseArea.containsMouse ? "#68A6E1" : "black"
    }

    Rectangle {
        id: currentChoiceFrame
        radius: 4
        border.color: mouseArea.containsMouse ? "#68A6E1" : "#9f9f9f"
        border.width: 1
        height: root.height
        width: parent.width / 2
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: currentChoiceText
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 8
            text: choices[currentIndex][displayRole]
            color: mouseArea.containsMouse ? "#68A6E1" : "#7F7F7F"
        }

        Image {
            id: upDownIcon
            source: "qrc:///up-down-arrow"
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        id: mouseArea
        hoverEnabled: true
        onClicked: {
            popupFrame.visible = true
        }
    }

    Rectangle {
        id: popupFrame

        width: currentChoiceFrame.width
        anchors.left: currentChoiceFrame.left

        // todo - position so current item lies on top
        anchors.top: currentChoiceFrame.bottom

        height: choicesColumn.childrenRect.height

        visible: false

        border.color: "#9f9f9f"
        border.width: 1

        Column {
            id: choicesColumn

            Repeater {
                model: choices
                delegate: Text {
                    text: choices[model.index][root.displayRole]
                    width: popupFrame.width
                    height: 40
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.currentIndex = model.index
                            popupFrame.visible = false
                        }
                    }

                }
            }
        }
    }
}

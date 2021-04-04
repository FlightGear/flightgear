import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Item {
    id: delegateRoot

    signal performDelete();
    signal performMove(var newIndex);

    property alias deletePromptText: confirmDeletePath.promptText
    property int modelCount: 0

    height: childrenRect.height

    Item {
        id: divider
        visible: model.index > 0 // divider before each item except the first
        height: Style.margin
        width: parent.width

        Rectangle {
            color: Style.frameColor
            height: 1
            width: parent.width - Style.strutSize
            anchors.centerIn: parent
        }
    }

    // this is the main visible content
    Item {
        id: contentRect
        anchors.top: divider.bottom
     //   color: "#cfcfcf"

        height: Math.max(label.implicitHeight, pathDeleteButton.height)
        width: delegateRoot.width

        Checkbox {
            id: enableCheckbox
            checked: model.enabled
            anchors.left: parent.left
            height: parent.height
            onCheckedChanged: {
                if (model.enabled !== checked) {
                    model.enabled = checked;
                }
            }
         }

        MouseArea {
            id: pathDelegateHover
            anchors.left: enableCheckbox.right
            anchors.leftMargin: Style.margin
            anchors.right: parent.right
            height: parent.height

            hoverEnabled: true
            acceptedButtons: Qt.NoButton

            // it's important that the button and text are children of the
            // MouseArea, so nested containsMouse logic works
            ClickableText {
                id: label
                text: model.path
                onClicked: {
                    // open the location
                    _addOns.openDirectory(model.path)
                }
                anchors.left: parent.left
                anchors.right: reorderButton.left
                anchors.rightMargin: Style.margin
                height: contentRect.height

                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap

                font.strikeout: enableCheckbox.checked === false
            }

            DeleteButton {
                id: pathDeleteButton
                anchors.right: parent.right
                visible: pathDelegateHover.containsMouse
                onClicked: {
                    confirmDeletePath.visible = true
                }
            }

            DragToReorderButton {
                id: reorderButton
                anchors.right: pathDeleteButton.left
                anchors.rightMargin: Style.margin
                visible: pathDelegateHover.containsMouse && (canMoveDown || canMoveUp)

                canMoveUp: model.index > 0
                canMoveDown: model.index < (delegateRoot.modelCount - 1)

                onMoveUp: {
                    if (model.index === 0)
                        return;
                    delegateRoot.performMove(model.index - 1)
                }

                onMoveDown:  {
                    delegateRoot.performMove(model.index + 1)
                }
            }
        } // of MouseArea for hover

        YesNoPanel {
            id: confirmDeletePath
            visible: false
            anchors.fill: parent
            yesText: qsTr("Remove")
            noText: qsTr("Cancel")
            yesIsDestructive: true

            onAccepted: {
                delegateRoot.performDelete()
            }

            onRejected: {
                confirmDeletePath.visible = false
            }

            Rectangle {
                anchors.fill: parent
                z: -1
            }
        }
    }
}


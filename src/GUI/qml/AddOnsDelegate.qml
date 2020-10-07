import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {
    id: delegateRoot

    signal performDelete();
    signal performMove(var newIndex);
    signal showDetails(var detailIndex);

    property alias deletePromptText: confirmDeletePath.promptText
    property int modelCount: 0

    height: childrenRect.height

    function displayLabel(lbl) {
             return qsTr("%1 %2").arg(lbl).arg((_addOns.modules.checkVersion(model.path) ? "" : qsTr(" (disabled due to incompatible FG version)")));
    }

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

        height: Math.max(label.implicitHeight, pathDeleteButton.height)
        width: delegateRoot.width

       Checkbox {
           id: enableCheckbox
           checked: model.enable
           anchors.left: parent.left
           height: contentRect.height
           onCheckedChanged: {
               _addOns.modules.enable(model.index, checked)       
           }
        }

        MouseArea {
            id: addonsDelegateHover
            anchors.leftMargin: Style.margin
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.left: enableCheckbox.right

            hoverEnabled: true
            acceptedButtons: Qt.NoButton

            // it's important that the button and text are children of the
            // MouseArea, so nested containsMouse logic works
            ClickableText {
                id: label
                text: delegateRoot.displayLabel(model.display)
                onClicked: {
                    // open the location
                    delegateRoot.showDetails(model.index)
                }
                anchors.left: addonsDelegateHover.left
                anchors.right: reorderButton.left
                anchors.rightMargin: Style.margin
                height: contentRect.height

                font.strikeout: !model.enable

                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
            }

            DeleteButton {
                id: pathDeleteButton
                anchors.right: parent.right
                visible: addonsDelegateHover.containsMouse
                onClicked: {
                    confirmDeletePath.visible = true
                }
            }

            DragToReorderButton {
                id: reorderButton
                anchors.right: pathDeleteButton.left
                anchors.rightMargin: Style.margin
                visible: addonsDelegateHover.containsMouse && (canMoveDown || canMoveUp)

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
        }

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


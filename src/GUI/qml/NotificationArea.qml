import QtQuick 2.4
import QtQml 2.4

import FlightGear.Launcher 1.0
import "."

Item {
    id: root
    width: 500

    readonly property int ourMargin: Style.margin * 2


    Component {
        id: notificationBox

        Rectangle {
            id: boxRoot

            property alias content: contentLoader.sourceComponent

            width: notificationsColumn.width
            height: contentLoader.height + (ourMargin * 2)

            clip: true
            color: Style.themeColor
            border.width: 1
            border.color: Qt.darker(Style.themeColor)

            Rectangle {
                id: background
                anchors.fill: parent
                z: -1
                opacity: Style.panelOpacity
                color: "white"
            }

            Loader {
                // height is not anchored, can float
                anchors {
                    top: parent.top
                    left: parent.left
                    right: closeButton.left
                    margins: ourMargin
                }

                id: contentLoader
                source: model.source

                // don't set height, comes from content
            }

            Connections {
                target: contentLoader.item
                onDismiss: {
                    _notifications.dismissIndex(model.index)
                }
            }

            Image {
                id: closeButton
                source: "qrc:///white-delete-icon"

                anchors {
                    verticalCenter: parent.verticalCenter
                    right: parent.right
                    margins: ourMargin
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor

                    onClicked:  {
                        if (contentLoader.item.dismissed) {
                            contentLoader.item.dismissed();
                        }

                        _notifications.dismissIndex(model.index)
                    }
                }
            }
        } // of notification box
    }

    // ensure clicks 'near' the notifications don't go to other UI
    MouseArea {
        width: parent.width
        height: notificationsColumn.height
    }

    Column {
        id: notificationsColumn
        // height of this is determined by content. This is important
        // so the mouse area above only blocks clicks near active
        // notifciations

        anchors {
            right: parent.right
            top: parent.top
            left: parent.left
            margins: Style.strutSize
        }

        spacing: Style.strutSize

        Repeater {
            model: _notifications.active
            delegate: notificationBox
        }
    } // of boxes column

}

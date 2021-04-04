import QtQuick 2.4
import QtQml 2.4
import FlightGear 1.0

Item {
    id: root

    width: 100
    height: 100

    property GettingStartedController controller: nil

    property rect contentGeom: controller.contentGeometry
    property rect tipGeom: controller.tipGeometry

    // pass the tip height into the controller, when it's valid
    Binding {
        target: controller
        property: "activeTipHeight"
        value: contentBox.height
    }

    // the visible tip box
    TipBackgroundBox {
        id: tipBox

        fill: Style.themeColor
        borderWidth: 1
        borderColor: Qt.darker(Style.themeColor)

        x: tipGeom.x
        y: tipGeom.y
        width: tipGeom.width
        height: tipGeom.height
        arrow: controller.tip.arrow

        Item {
            id: contentBox
            x: contentGeom.x
            y: contentGeom.y
            width: contentGeom.width
            height: tipText.height + closeText.height + (Style.margin * 3)

            Text {
                id: tipText

                font.pixelSize: Style.subHeadingFontPixelSize
                color: Style.themeContrastTextColor

                anchors {
                    top: parent.top
                    left: parent.left
                    right: parent.right
                    margins: Style.margin
                }

                // size this to the text, after wrapping
                height: implicitHeight

                text: controller.tip.text
                wrapMode: Text.WordWrap
            }

            Text {
                anchors {
                    margins: Style.margin
                    left: parent.left
                    top: tipText.bottom
                }

                text: "<"
                color: prevMouseArea.containsMouse ? Style.themeContrastLinkColor : Style.themeContrastTextColor
                visible: (controller.index > 0)
                MouseArea {
                    id: prevMouseArea
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true

                    onClicked:  {
                        controller.index = controller.index - 1
                    }
                }
            }

            Text {
                id: closeText

                anchors {
                    margins: Style.margin
                    horizontalCenter: parent.horizontalCenter
                    top: tipText.bottom
                }

                text: qsTr("Close")
                color: closeMouseArea.containsMouse ? Style.themeContrastLinkColor : Style.themeContrastTextColor
                width: implicitWidth
                horizontalAlignment: Text.AlignHCenter

                MouseArea {
                    id: closeMouseArea
                    hoverEnabled: true
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: controller.close();
                }
            }

            Text {
                anchors {
                    margins: Style.margin
                    right: parent.right
                    top: tipText.bottom
                }

                text: ">"
                color: nextMouseArea.containsMouse ? Style.themeContrastLinkColor : Style.themeContrastTextColor
                visible: controller.index < (controller.count - 1)

                MouseArea {
                    id: nextMouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked:  {
                        controller.index = controller.index + 1
                    }
                    cursorShape: Qt.PointingHandCursor
                }
            }
        } // of tip content box
    } // of tip background shape
}

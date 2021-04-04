import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0


Rectangle {
    id: root

    implicitHeight: title.implicitHeight

    radius: Style.roundRadius
    border.color: Style.frameColor
    border.width: headingMouseArea.containsMouse ? 1 : 0
    color: headingMouseArea.containsMouse ? "#7fffffff" : "transparent"

    readonly property int centerX: width / 2

    readonly property bool __enabled: aircraftInfo.numVariants > 1

    property alias aircraft: aircraftInfo.uri
    property alias currentIndex: aircraftInfo.variant
    property alias fontPixelSize: title.font.pixelSize

    // allow users to control the font sizing
    // (aircraft variants can have excessively long names)
    property int popupFontPixelSize: 0

    // expose this to avoid a second AircraftInfo in the compact delegate
    // really should refactor to put the AircraftInfo up there.
    readonly property string pathOnDisk: aircraftInfo.pathOnDisk

    signal selected(var index)

    Row {
        anchors.centerIn: parent
        height: title.implicitHeight
        width: childrenRect.width

        Text {
            id: title

            anchors.verticalCenter: parent.verticalCenter

            // this ugly logic is to keep the up-down arrow at the right
            // hand side of the text, but allow the font scaling to work if
            // we're short on horizontal space
            width: Math.min(implicitWidth,
                            root.width - (Style.margin * 2) - (__enabled ? upDownIcon.implicitWidth : 0))

            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter

            font.pixelSize: Style.baseFontPixelSize * 2
            text: aircraftInfo.name
            fontSizeMode: Text.Fit

            elide: Text.ElideRight
            maximumLineCount: 1
            color: headingMouseArea.containsMouse ? Style.themeColor : Style.baseTextColor
        }

        Image {
            id: upDownIcon
            source:  headingMouseArea.containsMouse ? "image://colored-icon/up-down?theme" : "image://colored-icon/up-down?text"
           // x: root.centerX + Math.min(title.implicitWidth * 0.5, title.width * 0.5)
            anchors.verticalCenter: parent.verticalCenter
            visible: __enabled
            width: __enabled ? implicitWidth : 0
        }
    }

    MouseArea {
        id: headingMouseArea
        enabled: __enabled
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            var hCenter = root.width / 2;
            OverlayShared.globalOverlay.showOverlayAtItemOffset(popup, root,
                                                                Qt.point(hCenter, root.height))
        }
    }

    AircraftInfo
    {
        id: aircraftInfo
    }

    Component {
        id: popup
        VariantMenu {
            id: menu
            aircraftUri: aircraftInfo.uri
            popupFontPixelSize: root.popupFontPixelSize
            
            onSelect: {
                aircraftInfo.variant = index;
                root.selected(index)
            }
        }
    } // of popup component
}

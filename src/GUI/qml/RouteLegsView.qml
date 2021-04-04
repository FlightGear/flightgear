import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Rectangle
{
    id: root
    implicitHeight: childrenRect.height + Style.margin * 2

    border.width: 1
    border.color: Style.minorFrameColor

    signal clickedLeg(var index)

    TextMetrics {
        id: legDistanceWidth
        font.pixelSize: Style.baseFontPixelSize
        text: "0000Nm"
    }

    TextMetrics {
        id: legBearingWidth
        font.pixelSize: Style.baseFontPixelSize
        text: "000*True"
    }

    TextMetrics {
        id: legIdentWidth
        font.pixelSize: Style.baseFontPixelSize
        text: "XXXXX"
    }

    TextMetrics {
        id: legViaWidth
        font.pixelSize: Style.baseFontPixelSize
        text: "via XXXXX"
    }

    TextMetrics {
        id: legAltitudeWidth
        font.pixelSize: Style.baseFontPixelSize
        text: "at 40000"
    }

    readonly property int legDistanceColumnStart: root.width - (legDistanceWidth.width + (Style.margin * 2))
    readonly property int legBearingColumnStart: legDistanceColumnStart - (legBearingWidth.width + Style.strutSize)
    readonly property int legAltitudeColumnStart: legBearingColumnStart - (legAltitudeWidth.width + Style.strutSize)

    // description string fills the middle space, gets elided
    readonly property int legDescriptionColumnStart: legIdentWidth.width + legViaWidth.width + Style.margin * 3
    readonly property int legDescriptStringWidth: legAltitudeColumnStart - legDescriptionColumnStart

    Column {
        width: parent.width - Style.margin * 2
        x: Style.margin
        y: Style.margin

        Repeater {
            id: routeLegs
            width: parent.width

            model: _launcher.flightPlan.legs

            delegate: Rectangle {
                id: delegateRect
                height: rowLabel.height + Style.margin
                width: routeLegs.width
                color: (model.index % 2) ? "#dfdfdf" : "white"

                readonly property string description: {
                    var s = model.toName;
                    if (model.wpType === "navaid") {
                        var freq = model.frequency
                        if (freq.isValid())
                            s += " (" + freq.toString() + ")"
                    }

                    return s;
                }

                readonly property string altitude: {
                    var rr = model.altitudeType;
                    // corresponds to the RouteRestriction enum in route.hxx
                    if (rr === 1) {
                        return qsTr("at %1'").arg(model.altitudeFt);
                    }
                    if (rr === 2) {
                        return qsTr("above %1'").arg(model.altitudeFt);
                    }
                    if (rr === 3) {
                        return qsTr("below %1'").arg(model.altitudeFt);
                    }

                    return "";
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    id: rowLabel
                    text: model.label
                    x: Style.margin
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    id: rowAirway
                    text: {
                        var awy = model.via;
                        if (awy === undefined) return "";
                        return "via " + awy;
                    }

                    x: Style.margin * 2 + legIdentWidth.width
                }

                StyledText {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: model.wpType === "navaid"
                    text: delegateRect.description
                    x: legDescriptionColumnStart
                    width: legDescriptStringWidth
                    elide: Text.ElideRight
                }

                StyledText {
                    x: legAltitudeColumnStart
                    anchors.verticalCenter: parent.verticalCenter
                    visible: (model.altitudeType > 0)
                    text: delegateRect.altitude
                }

                StyledText {
                    x: legBearingColumnStart
                    anchors.verticalCenter: parent.verticalCenter
                    visible: (model.index > 0)
                    text: model.track.toString()
                }

                StyledText {
                    x: legDistanceColumnStart
                    anchors.verticalCenter: parent.verticalCenter
                    visible: (model.index > 0)
                    text: model.distance.toString()
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.clickedLeg(model.index)
                    }
                }

            } // of delegate rect
        }

    }

}


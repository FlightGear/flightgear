import QtQuick 2.4

Item
{
    id: root

    implicitWidth: 14

    property Flickable flickable: parent
    readonly property real heightRatio: flickable ? flickable.visibleArea.heightRatio : 0

    readonly property int barSize: Math.max(height * heightRatio, width * 3);
    readonly property bool enabled: heightRatio < 1.0
    readonly property int scrollRange: height - barSize

    function normalisedPosition()
    {
        if (!flickable) return 0;
        // becuase we might use a longer bar height than is proportional to
        // height ratio, we have to re-normalise the position
        var visArea = flickable.visibleArea;
        return visArea.yPosition / (1.0 - visArea.heightRatio);
    }

    function verticalExtent()
    {
        var f = root.flickable;
        return (f.height / f.visibleArea.heightRatio) - f.height;
    }

    MouseArea {
        id: trackArea
        hoverEnabled: true
        anchors.fill: parent
        onClicked: {
            var clickPos = (mouse.y / height)
            flickable.contentY = clickPos * verticalExtent();
        }
        visible: root.enabled
    }

    Item {
        id: track
        width: parent.width
        height: parent.height

        Rectangle {
            anchors.fill: parent
            color: "white"
            opacity: 0.5
        }

        Rectangle {
            id: thumb
            x: 2
            width: parent.width - 4
            height: root.barSize
            radius: width / 2
            color: "#4f4f4f"

            Binding on y {
                when: !thumbArea.drag.active
                value: root.normalisedPosition() * root.scrollRange
            }

            MouseArea {
                id: thumbArea
                anchors.fill: parent
                drag.axis: Drag.YAxis
                drag.minimumY: 0
                drag.maximumY: scrollRange
                drag.target: thumb
                preventStealing: true

                onMouseYChanged: {
                    var position = (thumb.y / root.scrollRange)
                    flickable.contentY = position * verticalExtent();
                }
            }
        }
    } // of track item
}

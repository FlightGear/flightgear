import QtQuick 2.0
import FlightGear 1.0 as FG

Item {
    id: root
    property bool showDecorations: true
    property alias canvas: canvasDisplay.canvas

    readonly property var centerPoint: Qt.point(width / 2, height / 2)

    clip: true;

    Component.onCompleted: {
        if (canvas) {
            width = canvas.size.width
            height = canvas.size.height
            x = canvas.center.x - (width / 2)
            y = canvas.center.y - (height / 2)
        }
    }

    function saveGeometry()
    {
        canvas.center = Qt.point(x + (width / 2), y + (height / 2))
        canvas.size = Qt.size(root.width / 2, root.height / 2);
    }

    FG.CanvasDisplay {
        id: canvasDisplay
        anchors.fill: parent

        onCanvasChanged: {
            if (canvas) {
                root.width = canvas.size.width
                root.height = canvas.size.height

                root.x = canvas.center.x - (root.width / 2)
                root.y = canvas.center.y - (root.height / 2)
            }
        }
    }

    Rectangle {
        border.width: 1
        border.color: "orange"
        color: "transparent"
        anchors.centerIn: parent
        width: parent.width
        height: parent.height

        MouseArea {
            anchors.fill: parent

            drag.target: root

            onReleased: {
                root.saveGeometry();
            }
        }

        Rectangle {
            width: 32
            height: 32
            color: "orange"
            opacity: 0.5
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            MouseArea {
                anchors.fill: parent

                // resizing

                onPositionChanged: {
                    var rootPos = mapToItem(root, mouse.x, mouse.y);
                    var rootDiff = Qt.point(rootPos.x - root.centerPoint.x,
                                            rootPos.y - root.centerPoint.y);

                    root.width = rootDiff.x * 2;
                    root.height = rootDiff.y * 2;
                    root.x = canvas.center.x - (root.width / 2)
                    root.y = canvas.center.y - (root.height / 2)
                }

                onReleased: {
                    saveGeometry();
                }
            }
        }

        Text {
            color: "orange"
            anchors.centerIn: parent
            text: "Canvas"
        }

        Text {
            anchors.fill: parent
            verticalAlignment: Text.AlignBottom

            function statusAsString(status)
            {
                switch (status) {
                case FG.CanvasConnection.NotConnected: return "Not connected";
                case FG.CanvasConnection.Connecting: return "Connecting";
                case FG.CanvasConnection.Connected: return "Connected";
                case FG.CanvasConnection.Reconnecting: return "Re-connecting";
                case FG.CanvasConnection.Error: return "Error";
                }
            }

            text: "WS: " + canvas.webSocketUrl + "\n"
                  + "Root:" + canvas.rootPath + "\n"
                  + "Status:" + statusAsString(canvas.status);
            color: "white"

        }
    }
}

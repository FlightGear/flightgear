import QtQuick 2.0
import FlightGear 1.0 as FG

Item {
    id: root
    property bool showDecorations: true
    property alias canvas: canvasDisplay.canvas

    //clip: true;

    Component.onCompleted: {
        if (canvas) {
            width = canvas.size.width
            height = canvas.size.height
            x = canvas.origin.x
            y = canvas.origin.y
        }
    }

    function saveGeometry()
    {
        canvas.origin = Qt.point(x, y )
        canvas.size = Qt.size(root.width, root.height);
    }

    FG.CanvasDisplay {
        id: canvasDisplay
        anchors.fill: parent

        onCanvasChanged: {
            if (canvas) {
                root.width = canvas.size.width
                root.height = canvas.size.height
                root.x = canvas.origin.x
                root.y = canvas.origin.y
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
                  //  var rootDiff = Qt.point(rootPos.x - root.x,
                  //                          rootPos.y - root.y);

                    root.width = rootPos.x;
                    root.height = rootPos.y;
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
                case FG.CanvasConnection.Closed: return "Closed";
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

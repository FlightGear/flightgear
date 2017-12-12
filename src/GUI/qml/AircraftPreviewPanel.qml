import QtQuick 2.0
import FlightGear.Launcher 1.0

Rectangle {
    id: root
    property var previews: []
    property int activePreview: 0

    readonly property bool __havePreviews: (previews.length > 0)
    onPreviewsChanged: {
        activePreview = 0
    }

    height: width / (16/9)
    color: "#7f7f7f"

    border.width: 1
    border.color: "#3f3f3f"

    Timer {
        id: previewCycleTimer
    }

    PreviewImage {
        width: parent.width
        height: parent.height
        imageUrl: __havePreviews ? root.previews[root.activePreview] : ""

        Rectangle {
            anchors.fill: parent
            visible: parent.isLoading
            opacity: 0.5
            color: "black"
        }

        AnimatedImage {
            id: spinner
            source: "qrc:///spinner"
            anchors.centerIn: parent
            visible: parent.isLoading
        }
    }

    Row {
        visible: (previews.length > 1)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 8

        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 8

        Repeater
        {
            model: root.previews
            Rectangle {
                height: 8
                width: 8
                radius: 4
                color: (model.index == root.activePreview) ? "white" : "#cfcfcf"
            }
        }
    }

    Image {
        id: leftArrow

        source: "qrc:///preview/left-arrow-icon"
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        opacity: leftMouseArea.containsMouse ? 1.0 : 0.3
        visible: root.activePreview > 0

        MouseArea {
            id: leftMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                root.activePreview = Math.max(root.activePreview - 1, 0)
            }
        }
    }

    Image {
        id: rightArrow

        source: "qrc:///preview/right-arrow-icon"
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        opacity: rightMouseArea.containsMouse ? 1.0 : 0.3
        visible: root.activePreview < (root.previews.length - 1)

        MouseArea {
            id: rightMouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                root.activePreview = Math.min(root.activePreview + 1, root.previews.length - 1)
            }
        }
    }
}

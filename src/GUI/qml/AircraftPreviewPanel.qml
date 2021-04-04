import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Rectangle {
    id: root
    property var previews: []
    property int activePreview: 0

    readonly property bool __havePreviews: (previews.length > 0)
    onPreviewsChanged: {
        activePreview = 0
        preview.clear()
    }

    height: width / preview.aspectRatio
    color: Style.minorFrameColor

    border.width: 1
    border.color: Style.themeColor

    Timer {
        id: previewCycleTimer
    }

    PreviewImage {
        id: preview
        width: parent.width
        height: parent.height

        function activePreviewUrl()
        {
            if (!__havePreviews) return "";
            if (root.previews.length <= root.activePreview) return "";
            return root.previews[root.activePreview];
        }

        imageUrl: activePreviewUrl();

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
                color: (model.index == root.activePreview) ? "white" : Style.themeColor
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

        GettingStartedTip {
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.left
            }
            arrow: GettingStartedTip.RightCenter
            text: qsTr("Click here to cycle through preview images")
        }
    }
}

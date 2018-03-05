import QtQuick 2.0
import FlightGear.Launcher 1.0
import "." // -> forces the qmldir to be loaded

Item {
    id: root

    property url uri
    property int installStatus: LocalAircraftCache.PackageNotInstalled
    property int packageSize: 0

    property int downloadedBytes: 0

    readonly property bool active: (installStatus == LocalAircraftCache.PackageQueued) ||
                                   (installStatus == LocalAircraftCache.PackageDownloading)

    readonly property int compactWidth: button.width + sizeText.width

    property bool compact: false

    implicitWidth: childrenRect.width
    implicitHeight: childrenRect.height


    state: "not-installed"

    onInstallStatusChanged: {
        if (installStatus == LocalAircraftCache.PackageInstalled) {
            state = "installed";
        } else if (installStatus == LocalAircraftCache.PackageNotInstalled) {
            state = "not-installed"
        } else if (installStatus == LocalAircraftCache.PackageUpdateAvailable) {
            state = "has-update"
        } else if (installStatus == LocalAircraftCache.PackageQueued) {
            state = "queued"
        } else if (installStatus == LocalAircraftCache.PackageDownloading) {
            state = "downloading"
        }
    }

    states: [
        State {
            name: "not-installed"

            PropertyChanges {
                target: button
                text: "Install"
                hoverText: ""
            }

            PropertyChanges {
                target: sizeText
                visible: true
            }

        },

        State {
            name: "installed"

            PropertyChanges {
                target: button
                text: "Uninstall"
                hoverText: ""
            }

            PropertyChanges {
                target: sizeText
                visible: true
            }
        },

        State {
            name: "has-update"

            PropertyChanges {
                target: button
                text: "Update"
                hoverText: ""
            }

            PropertyChanges {
                target: sizeText
                visible: true
            }
        },

        State {
            name: "queued"

            PropertyChanges {
                target: sizeText
                visible: true
            }

            PropertyChanges {
                target: button
                text: "Queued"
                hoverText: "Cancel"
            }
        },

        State {
            name: "downloading"
            PropertyChanges {
                target: progressFrame
                visible: true
            }

            PropertyChanges {
                target: statusText
                visible: true
            }

            PropertyChanges {
                target: sizeText
                visible: false
            }

            PropertyChanges {
                target: button
                text: "Downloading"
                hoverText: "Cancel"
            }
        }
    ]

    Button {
        id: button
        onClicked: {
            if ((root.state == "has-update") || (root.state == "not-installed")) {
                _launcher.requestInstallUpdate(root.uri);
            } else if (root.state == "installed") {
                _launcher.requestUninstall(root.uri)
            } else {
                _launcher.requestInstallCancel(root.uri)
            }
        }
    }

    Text {
        id: sizeText
        anchors.left: button.right
        anchors.leftMargin: 6
        anchors.verticalCenter: button.verticalCenter
        text: "Size: " + (root.packageSize / 0x100000).toFixed(1) + "MB"
        font.pixelSize: Style.baseFontPixelSize
    }

    Column {
        anchors.verticalCenter: button.verticalCenter
        anchors.left: button.right
        anchors.leftMargin: 6
        anchors.right: parent.right

        Rectangle {
            id: progressFrame
            radius: 6
            height: 12

            width: parent.width
            visible: false // hidden by default

            border.color: "#cfcfcf"
            border.width: 2

            Rectangle {
                id: progressBar

                radius:3
                height: 6
                anchors.verticalCenter: parent.verticalCenter

                color: "#1b7ad3"

                readonly property real fraction: root.downloadedBytes / root.packageSize
                readonly property real maxWidth: parent.width - 10

                x: 5
                width: maxWidth * fraction
            }
        }

        // show download progress textually, or error message if a problem occurs
        Text {
            id: statusText
            visible: false
            text: (compact ? "" : "Downloaded ") + (root.downloadedBytes / 0x100000).toFixed(1) +
                  "MB of " + (root.packageSize / 0x100000).toFixed(1) + "MB";
            font.pixelSize: Style.baseFontPixelSize
        }
    } // item container for progress bar and text
}

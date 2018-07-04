import QtQuick 2.4
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
                text: qsTr("Install")
                hoverText: ""
                visible: true
            }

            PropertyChanges { target: sizeText; visible: true }
            PropertyChanges { target: confirmUninstallPanel; visible: false }
        },

        State {
            name: "installed"

            PropertyChanges {
                target: button
                text: qsTr("Uninstall")
                hoverText: ""
                visible: true
            }

            PropertyChanges { target: sizeText; visible: true }
            PropertyChanges { target: confirmUninstallPanel; visible: false }
        },

        State {
            name: "has-update"

            PropertyChanges {
                target: button
                text: qsTr("Update")
                hoverText: ""
                visible: true
            }

            PropertyChanges { target: sizeText; visible: true }
            PropertyChanges { target: confirmUninstallPanel; visible: false }
        },

        State {
            name: "queued"

            PropertyChanges {
                target: sizeText
                visible: true
            }

            PropertyChanges {
                target: button
                text: qsTr("Queued")
                hoverText: qsTr("Cancel")
                visible: true
            }

            PropertyChanges { target: confirmUninstallPanel; visible: false }
        },

        State {
            name: "downloading"
            PropertyChanges { target: progressFrame; visible: true }
            PropertyChanges { target: statusText; visible: true }
            PropertyChanges { target: sizeText; visible: false }
            PropertyChanges { target: confirmUninstallPanel; visible: false }

            PropertyChanges {
                target: button
                text: qsTr("Downloading")
                hoverText: qsTr("Cancel")
                visible: true
            }
        },

        State {
            name: "confirm-uninstall"
            PropertyChanges { target: button; visible: false }
            PropertyChanges { target: progressFrame; visible: false }
            PropertyChanges { target: statusText; visible: false }
            PropertyChanges { target: sizeText; visible: false }
            PropertyChanges { target: confirmUninstallPanel; visible: true }
        }
    ]

    Button {
        id: button
        onClicked: {
            if ((root.state == "has-update") || (root.state == "not-installed")) {
                _launcher.requestInstallUpdate(root.uri);
            } else if (root.state == "installed") {
                root.state = "confirm-uninstall"
            } else {
                _launcher.requestInstallCancel(root.uri)
            }
        }
    }

    StyledText {
        id: sizeText
        anchors.left: button.right
        anchors.leftMargin: 6
        anchors.verticalCenter: button.verticalCenter
        text: "Size: " + (root.packageSize / 0x100000).toFixed(1) + "MB"
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

            border.color: Style.minorFrameColor
            border.width: 2

            Rectangle {
                id: progressBar

                radius:3
                height: 6
                anchors.verticalCenter: parent.verticalCenter

                color: Style.themeColor

                readonly property real fraction: root.downloadedBytes / root.packageSize
                readonly property real maxWidth: parent.width - 10

                x: 5
                width: maxWidth * fraction
            }
        }

        // show download progress textually, or error message if a problem occurs
        StyledText {
            id: statusText
            visible: false
            text: (compact ? "" : "Downloaded ") + (root.downloadedBytes / 0x100000).toFixed(1) +
                  "MB of " + (root.packageSize / 0x100000).toFixed(1) + "MB";
        }
    } // item container for progress bar and text

    YesNoPanel {
        id: confirmUninstallPanel
        anchors.fill: parent
        promptText: qsTr("Are you sure you want to uninstall this aircraft?")
        yesIsDestructive: true
        yesText: qsTr("Uninstall")
        noText: qsTr("Cancel")

        onRejected: root.state = "installed"
        onAccepted: _launcher.requestUninstall(root.uri)
     }
}

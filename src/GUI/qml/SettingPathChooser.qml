import QtQuick 2.4
import FlightGear 1.0 as FG
import FlightGear 1.0

SettingControl {
    id: root
    implicitHeight: defaultButton.height + Style.margin + description.height

    property alias label: label.text
    property string path
    property string defaultPath
    property alias chooseDirectory: picker.selectFolder
    property alias dialogPrompt: picker.title


    property alias value: root.path // needed for save + restore
    property string defaultValue: "" // needed for correct save/restore typing

    readonly property bool isDefault: (path.length == 0) || (path == defaultPath)

    readonly property url effectiveUrl: picker.urlFromLocalFilePath(effectivePath());

    function effectivePath()
    {
        return root.isDefault ? defaultPath : path;
    }

    function apply()
    {
        if (option != "") {
            _config.setArg(option, effectivePath())
        }
    }

    Text {
        id: label
        text: root.label
        anchors.verticalCenter: chooseButton.verticalCenter
        color: (root.enabled ? Style.baseTextColor : Style.disabledTextColor)
    }

    Text {
        id: currentPath
        text: root.isDefault ? qsTr("%1 (default)").arg(root.defaultPath) : root.path
        anchors.verticalCenter: chooseButton.verticalCenter
        color: (root.enabled ? Style.baseTextColor : Style.disabledTextColor)
        font.italic: true
        anchors.left: label.right
        anchors.leftMargin: Style.margin
        anchors.right: chooseButton.left
        anchors.rightMargin: Style.margin
        elide: Text.ElideRight
    }



    Button {
        id: chooseButton
        text: qsTr("Change")
        anchors.right: defaultButton.left
        anchors.rightMargin: Style.margin
        enabled: root.enabled

        onClicked: {
            // set current value as a URL
            picker.folder = effectiveUrl
            picker.open();
        }
    }

    Button {
        id: defaultButton
        text: qsTr("Use default")
        anchors.right: parent.right
        anchors.rightMargin: Style.margin
        enabled: root.enabled && !root.isDefault

        onClicked: {
            path = defaultPath
        }
    }

    SettingDescription {
        id: description
        enabled: root.enabled
        text: root.description
        anchors.top: chooseButton.bottom
        anchors.topMargin: Style.margin
        width: parent.width
    }

    FG.FileDialog {
        id: picker

        onAccepted: {
            path = filePath
        }

        onRejected: {

        }
    }
}

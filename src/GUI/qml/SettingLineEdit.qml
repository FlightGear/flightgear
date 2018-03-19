import QtQuick 2.4
import "."

SettingControl {
    id: root
    implicitHeight: childrenRect.height

    property string placeholder: ""
    property alias validation: edit.validator
    property alias value: edit.text

    property alias suggestedWidthString: metrics.text
    readonly property int suggestedWidth: useFullWidth ? root.width
                                                       : ((metrics.width == 0) ? Style.strutSize * 4
                                                                               : metrics.width)

    property bool useFullWidth: false

    property string defaultValue: ""

    function apply()
    {
        if (option != "") {
            _config.setArg(option, value)
        }
    }

    TextMetrics {
        id: metrics
    }

    Text {
        id: label
        text: root.label
        anchors.verticalCenter: editFrame.verticalCenter
        color: editFrame.activeFocus ? Style.themeColor :
                                     (root.enabled ? "black" : Style.inactiveThemeColor)
    }

    Rectangle {
        id: editFrame

        anchors.left: label.right
        anchors.margins: Style.margin


        height: edit.implicitHeight + Style.margin

        width: Math.min(root.width - (label.width + Style.margin * 2), Math.max(suggestedWidth, edit.implicitWidth));

        radius: Style.roundRadius
        border.color: edit.activeFocus ? Style.frameColor : Style.minorFrameColor
        border.width: 1
        clip: true

        TextInput {
            id: edit
            enabled: root.enabled
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Style.margin
            selectByMouse: true

            Text {
                id: placeholder
                visible: (edit.text.length == 0) && !edit.activeFocus
                text: root.placeholder
                color: Style.baseTextColor
            }
       }        
    }

    SettingDescription {
        id: description
        enabled: root.enabled
        text: root.description
        anchors.top: editFrame.bottom
        anchors.topMargin: Style.margin
        width: parent.width
    }
}

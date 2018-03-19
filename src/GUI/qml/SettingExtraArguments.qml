import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

SettingControl {
    id: root
    implicitHeight: childrenRect.height
    readonly property string placeholder: "--option=value --prop:/sim/name=value"
    property alias value: edit.text
    option: "xxx" // non-empty value so apply() is called
    property string defaultValue: "" // needed to type save/restore logic to string

    SettingDescription {
        id: description
        enabled: root.enabled
        anchors.top: parent.top
        anchors.topMargin: Style.margin
        width: parent.width
        text: qsTr("Enter additional command-line arguments if any are required. " +
                   "See <a href=\"http://flightgear.sourceforge.net/getstart-en/getstart-enpa2.html#x5-450004.5\">here</a> " +
                   "for documentation on possible arguments.");
    }

    Rectangle {
        id: editFrame

        anchors.left: parent.left
        anchors.margins: Style.margin
        anchors.right: parent.right
        anchors.top: description.bottom

        height: edit.height + Style.margin

        border.color: edit.activeFocus ? Style.frameColor : Style.minorFrameColor
        border.width: 1

        TextEdit {
            id: edit
            enabled: root.enabled
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Style.margin
            height: Math.max(Style.strutSize * 4, implicitHeight)
            textFormat: TextEdit.PlainText
            font.family: "Courier"
            selectByMouse: true
            wrapMode: TextEdit.WordWrap

            Text {
                id: placeholder
                visible: (edit.text.length == 0) && !edit.activeFocus
                text: root.placeholder
                color: Style.baseTextColor
            }
       }
    }

    function apply()
    {
        var tokens = tokenizer.tokens;
        for (var i = 0; i < tokens.length; i++) {
            var tk = tokens[i];
            if (tk.arg.substring(0, 5) == "prop:") {
                _config.setProperty(tk.arg.substring(5), tk.value);
            } else {
                _config.setArg(tk.arg, tk.value);
            }
        }
    }

    ArgumentTokenizer {
        id: tokenizer
        argString: edit.text
    }

}

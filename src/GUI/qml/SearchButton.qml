import QtQuick 2.2
import "."

FocusScope
{
    id: root

    width: Style.strutSize * 3
    height: frame.height

    property string placeholder: qsTr("Search")
    property bool active: false

    signal search(string term)

    property bool autoSubmit: true
    property alias autoSubmitTimeout: searchTimer.interval

    onActiveChanged: {
        if (!active) {
            clear();
        }
    }

    function clear()
    {
        buttonText.text = ""
        root.focus = false
        searchTimer.stop();
        root.search("");
    }

    Rectangle
    {
        id: frame
        radius: Style.roundRadius

        width: root.width
        height: Math.max(searchIcon.height, buttonText.height) + (Style.roundRadius)
        border.width: 1
        border.color: (mouse.containsMouse | active) ? Style.themeColor: Style.minorFrameColor
        clip: true



        TextInput {
            id: buttonText
            anchors.left: parent.left
            anchors.right: searchIcon.left
            anchors.margins: Style.margin
            anchors.verticalCenter: parent.verticalCenter

            color: Style.baseTextColor
            focus: true

            onTextChanged: {
                if (root.autoSubmit) {
                    searchTimer.restart();
                }
            }

            onEditingFinished: {
                if (text == "") {
                    clear();
                } else {
                    root.search(text);
                    root.focus = false;
                }
            }

            text: ""

            // placeholder text, hides itself whenever parent has non-empty text
            Text {
                anchors.fill: parent
                visible: parent.text == ""
                text: root.placeholder
            }
        }

        Image {
            id: searchIcon
            source: clearButtonMouse.containsMouse ? "qrc:///up-down-arrow" :"qrc:///search-icon-small"
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            anchors.verticalCenter: parent.verticalCenter
        }

        MouseArea {
            id: mouse
            anchors.left: parent.left
            height: parent.height
            anchors.right: clearButtonMouse.left
            hoverEnabled: true

            onClicked: {
                if (!buttonText.activeFocus) {
                    root.forceActiveFocus();
                    buttonText.selectAll();
                }
            }
        }


        MouseArea {
            id: clearButtonMouse
            anchors.right: parent.right
            height: parent.height
            width: searchIcon.width
            hoverEnabled: true

            onClicked: {
                clear();
            }
        }

        Timer {
            id: searchTimer
            interval: 800
            onTriggered: {
                if (buttonText.text.length > 2) {
                    root.search(buttonText.text)
                } else if (buttonText.text.length == 0) {
                    root.search(""); // ensure we update with no search
                }
            }
        }
    }
} // of FocusScope

import QtQuick 2.2

Rectangle {
    id: root

   // property string text
    property bool active: false

    signal search(string term)

    radius: 6

    border.width: 1
    border.color: (mouse.containsMouse | active) ? "#1b7ad3" : "#cfcfcf"
    clip: true

    TextInput {
        id: buttonText
        anchors.left: parent.left
        anchors.right: searchIcon.left
        anchors.margins: 4
        anchors.verticalCenter: parent.verticalCenter

        color: "#3f3f3f"

        onTextChanged: {
            searchTimer.restart();
        }

        onEditingFinished: {
            root.search(text);
            focus = false;
        }

        text: "Search"
    }

    Image {
        id: searchIcon
        source: "qrc:///search-icon-small"
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            buttonText.forceActiveFocus();
            buttonText.text = "";
        }

    }

    onActiveChanged: {
        if (!active) {
            // rest search text when we deactive
            buttonText.text = "Search"
            searchTimer.stop();
        }
    }

    Timer {
        id: searchTimer
        interval: 800
        onTriggered: {
            if (buttonText.text.length > 2) {
                root.search(buttonText.text)
            }
        }
    }
}

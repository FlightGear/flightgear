import QtQuick 2.4
import FlightGear 1.0


Item {
    id: root

    readonly property int __smallMargin: 2

    width: Style.strutSize * 2
    height: iconImage.height + label.height + Style.margin + (__smallMargin * 2)

    property string icon: ""
    property alias label: label.text

    signal clicked()

    property bool selected: false
    property bool enabled: true

    property string disabledText: ""

    onEnabledChanged: {
        // if we become enabled, ensure we hide the
        // dsiabled prompt (can happen if hovering over 'fly!')
        if (enabled) disabledTextBox.hide();
    }

    Rectangle {
        id: baseRect
        anchors.fill: parent
        visible: root.enabled & (root.selected | mouse.containsMouse)
        color: Style.activeColor
    }

    Image {
        id: iconImage
        source: root.icon + "?themeContrast"
        anchors.horizontalCenter: parent.horizontalCenter
        opacity: root.enabled ? 1.0 : 0.5
       y: __smallMargin
    }

    Text {
        id: label
        color: Style.themeContrastTextColor
        // enabled appearance is done via opacity to match the icon
        opacity: root.enabled ? 1.0 : 0.5
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        anchors.top: iconImage.bottom
        anchors.topMargin: __smallMargin
        font.pixelSize: Style.baseFontPixelSize
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: if (root.enabled) root.clicked();

        onEntered: {
            // disabled tooltip
            if (!root.enabled) tooltipTimer.restart()
        }

        onExited: {
            tooltipTimer.stop()
            disabledTextBox.hide();
        }
    }

    Timer {
        id: tooltipTimer
        onTriggered: {
            disabledTextBox.show()
        }
    }

    Rectangle {
        id: disabledTextBox
        height:disabledTextContent.implicitHeight + Style.margin * 2
        width: disabledTextContent.implicitWidth + Style.margin * 2
        color: Style.themeColor
        visible: false  

        anchors.left: baseRect.right
        anchors.verticalCenter: baseRect.verticalCenter

        function show() {
            hideDisabledAnimation.stop();
            showDisabledAnimation.start();
        }

        function hide() {
            if (!visible)
                return;
            showDisabledAnimation.stop();
            hideDisabledAnimation.start();
        }

        Text {
            id: disabledTextContent
            x: Style.margin
            y: Style.margin
            color: "white"
            font.pixelSize: Style.subHeadingFontPixelSize
            verticalAlignment: Text.AlignVCenter
            text: root.disabledText
        }

        SequentialAnimation {
            id: showDisabledAnimation
            PropertyAction { target:disabledTextBox; property:"visible"; value: true }
            NumberAnimation { 
                target:disabledTextBox; property:"opacity"
                from: 0.0; to: 1.0
                duration: 500
            }
        }

        SequentialAnimation {
            id: hideDisabledAnimation
            NumberAnimation { 
                target:disabledTextBox; property:"opacity"
                from: 1.0; to: 0.0
                duration: 500
            }
            PropertyAction { target:disabledTextBox; property:"visible"; value: false }
        }
    } // of disabled text box
}

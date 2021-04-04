import QtQuick 2.4
import FlightGear 1.0

FocusScope {
    id: root
    property string label
    property bool enabled: true
    property double value: 0.0
    property alias min: validator.bottom
    property int max: validator.top
    property alias decimals: validator.decimals

    property bool wrap: false
    property alias suffix: suffix.text
    property alias prefix: prefix.text
    property alias maxDigits: edit.maximumLength
    property int step: 1
    property bool live: false

    implicitHeight: editFrame.height
    // we have a margin between the frame and the label, and on each
    implicitWidth: label.width + editFrame.width + Style.margin

    signal commit(var newValue);

    function incrementValue()
    {
        if (edit.activeFocus) {
            var newValue = Math.min(parseFloat(edit.text) + root.step, root.max)
            edit.text = newValue
            if (live) {
                commit(newValue);
            }
        } else {
            commit(Math.min(value + root.step, root.max))
        }
    }

    function decrementValue()
    {
        if (edit.activeFocus) {
            var newValue = Math.max(parseFloat(edit.text) - root.step, root.min)
            edit.text = newValue
            if (live) {
                commit(newValue);
            }
        } else {
            commit(Math.max(value - root.step, root.min))
        }
    }

    TextMetrics {
        id: metrics
        text: root.max // use maximum value
    }

    StyledText {
        id: label
        text: root.label
        anchors.verticalCenter: editFrame.verticalCenter
        hover: editFrame.activeFocus
        enabled: root.enabled
    }

    MouseArea {
        height: root.height
        width: root.width
        enabled: root.enabled

        // use wheel events to adjust up/dowm
        onClicked: {
            edit.forceActiveFocus();
        }

        onWheel: {
            var delta = wheel.angleDelta.y
            if (delta > 0) {
                root.incrementValue()
            } else if (delta < 0) {
               root.decrementValue()
            }
        }
    }

    // timer to commit the value when in live mode
    Timer {
        id: liveEditTimer
        interval: 800
        onTriggered: {
            if (edit.activeFocus) {
                commit(parseFloat(edit.text));
            }
        }
    }

    Binding {
        when: !edit.activeFocus
        target: edit
        property: "text"
        value: root.value
    }

    Rectangle {
        id: editFrame
        clip: true
        anchors.left: label.right
        anchors.margins: Style.margin

        height: edit.implicitHeight + Style.margin
        width: edit.width + prefix.width + suffix.width + upDownArea.width + Style.margin * 2
        radius: Style.roundRadius
        border.color: root.enable ? (edit.activeFocus ? Style.frameColor : Style.minorFrameColor) : Style.disabledMinorFrameColor
        border.width: 1

        StyledText {
            id: prefix
            visible: root.prefix !== ""
            enabled: root.enabled
            anchors.baseline: edit.baseline
            anchors.left: parent.left
            anchors.margins: Style.margin
        }

        TextInput {
            id: edit
            enabled: root.enabled
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: prefix.right
            selectByMouse: true
            width: metrics.width
            horizontalAlignment: Text.AlignRight
            font.pixelSize: Style.baseFontPixelSize

            focus: true
            color: enabled ? (activeFocus ? Style.themeColor : Style.baseTextColor) : Style.disabledTextColor

            validator: DoubleValidator {
                id: validator
            }

            Keys.onUpPressed: {
                root.incrementValue();
            }

            Keys.onDownPressed: {
                root.decrementValue();
            }

            onActiveFocusChanged: {
                if (activeFocus) {
                    selectAll();
                } else {
                    commit(parseFloat(text))
                    liveEditTimer.stop();
                }
            }

            onTextChanged: {
                if (activeFocus && root.live) {
                    liveEditTimer.restart();
                }
            }
        }

        StyledText {
            id: suffix
            visible: root.suffix !== ""
            enabled: root.enabled
            anchors.baseline: edit.baseline
            anchors.right: upDownArea.left
        }

        Item {
            id: upDownArea
          //  color: "white"
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            anchors.verticalCenter: editFrame.verticalCenter
            height: upDownIcon.implicitHeight
            visible: edit.activeFocus
            width: upDownIcon.implicitWidth

            Image {
                id: upDownIcon
                // show up/down arrows
                source: "image://colored-icon/up-down?text"
            }

            MouseArea {
                width: parent.width
                height: parent.height / 2
                onPressed: {
                    root.incrementValue();
                }

                Rectangle {
                    anchors.fill: parent
                    opacity: 0.5
                    color: Style.themeColor
                    visible: parent.pressed
                }

                Timer {
                    id: upRepeat
                    interval: 250
                    running: parent.pressed
                    repeat: true
                    onTriggered: root.incrementValue()
                }
            }

            MouseArea {
                width: parent.width
                height: parent.height / 2
                anchors.bottom: parent.bottom
                onPressed: {
                    root.decrementValue();
                }

                Rectangle {
                    anchors.fill: parent
                    opacity: 0.5
                    color: Style.themeColor
                    visible: parent.pressed
                }

                Timer {
                    id: downRepeat
                    interval: 250
                    running: parent.pressed
                    repeat: true
                    onTriggered: root.decrementValue()
                }
            }
        }
    } // of frame rectangle


}

import QtQuick 2.4
import FlightGear 1.0

FocusScope {
    id: root

    property alias label: label.text
    property bool enabled: true
    property var value: new Date()
    property bool isDuration: true

    implicitHeight: editFrame.height
    implicitWidth: label.implicitWidth + Style.margin + editFrame.width

    function updateCurrentTime()
    {
        root.value = new Date(0, 0, 0, hours.value, minutes.value);
    }

    function setDurationMinutes(minutes)
    {
        var d = new Date(0, 0, 0, 0, minutes);
        setTime(d);
    }

    function setTime(date)
    {
        hours.value = date.getHours();
        minutes.value = date.getMinutes();
        updateCurrentTime();
    }

    StyledText {
        id: label
        anchors.left: root.left
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignRight
        enabled: root.enabled
    }

    Keys.onReturnPressed: {
        if (activeFocus) {
            focus = false;
        }
    }

    Rectangle {
        id: editFrame
        clip: true


        height: hours.height + Style.margin

        anchors.left: label.right
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        width: editRow.childrenRect.width + Style.roundRadius * 2 + 64

        Row {
            id: editRow
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
                right: parent.right
                margins: Style.margin
            }

            DateTimeValueEdit {
                id: hours
                minValue: 0
                maxValue: root.isDuration ? 9999 : 23
                widthString: "00"
                nextToFocus: minutes
                anchors.verticalCenter: parent.verticalCenter
                onCommit: updateCurrentTime();
            }

            StyledText {
                text: " : "
                anchors.verticalCenter: parent.verticalCenter
                enabled: root.enabled
            }

            DateTimeValueEdit {
                id: minutes
                minValue: 0
                maxValue: 59
                widthString: "00"
                anchors.verticalCenter: parent.verticalCenter
                previousToFocus: hours
                onCommit: updateCurrentTime();
            }
        } // of time elements row

        // frame rectange - we need this so we can clip our children
        // but ensure our frame also apepars on top
        Rectangle {
            z: 100
            anchors.fill: parent
            border.color: root.focus ? Style.themeColor : Style.minorFrameColor
            border.width: 1
            radius: Style.roundRadius
            color: "transparent"
        }
    }

}

import QtQuick 2.4
import FlightGear 1.0

FocusScope {
    id: root

    property alias label: label.text
    property bool enabled: true
    property var value: new Date()

    implicitHeight: label.implicitHeight
    implicitWidth: label.implicitWidth + Style.margin + editFrame.width

    function daysInMonth()
    {
        var tempDate = new Date(year.value, month.value, 0 /* last day of preceeding month */);
        return tempDate.getDate();
    }

    function updateCurrentDate()
    {
        root.value = new Date(year.value, month.value - 1, dayOfMonth.value,
                              hours.value, minutes.value);
    }

    function setDate(date)
    {
        year.value = date.getFullYear();
        month.value = date.getMonth() + 1;
        dayOfMonth.value = date.getDate();
        hours.value = date.getHours();
        minutes.value = date.getMinutes();
        updateCurrentDate();
    }

    StyledText {
        id: label
        anchors.left: root.left
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignRight
        enabled: root.enabled
    }

    Rectangle {
        id: editFrame
        radius: Style.roundRadius
        border.color: root.focus ? Style.themeColor : Style.minorFrameColor

        border.width: 1
        height: 30
    //    height: currentChoiceText.implicitHeight + Style.margin

        anchors.left: label.right
        anchors.leftMargin: Style.margin

        width: editRow.childrenRect.width + Style.roundRadius * 2 + 64

        // width of current item, or available space after the label
 //       width: Math.min(root.width - (label.width + Style.margin),
   //                     currentChoiceText.implicitWidth + (Style.margin * 2) + upDownIcon.width);

        anchors.verticalCenter: parent.verticalCenter

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
                id: year
                maxValue: 9999
                widthString: "0000"
                nextToFocus: month
                fieldWidth: 4
                anchors.verticalCenter: parent.verticalCenter
                onCommit: updateCurrentDate();
            }

            StyledText {
                text: " / "
                anchors.verticalCenter: parent.verticalCenter
                enabled: root.enabled
            }

            DateTimeValueEdit {
                id: month
                minValue: 1
                maxValue: 12
                widthString: "00"
                nextToFocus: dayOfMonth
                previousToFocus: year
                anchors.verticalCenter: parent.verticalCenter
                onCommit: updateCurrentDate();
            }

            StyledText {
                text: " / "
                anchors.verticalCenter: parent.verticalCenter
                enabled: root.enabled
            }

            DateTimeValueEdit {
                id: dayOfMonth
                minValue: 1
                maxValue: root.daysInMonth()
                widthString: "00"
                nextToFocus: hours
                previousToFocus: month
                anchors.verticalCenter: parent.verticalCenter
                onCommit: updateCurrentDate();
            }

            // spacer here
            StyledText {
                text: "  "
                anchors.verticalCenter: parent.verticalCenter
                enabled: root.enabled
            }

            DateTimeValueEdit {
                id: hours
                minValue: 0
                maxValue: 23
                widthString: "00"
                nextToFocus: minutes
                previousToFocus: dayOfMonth
                anchors.verticalCenter: parent.verticalCenter
                onCommit: updateCurrentDate();
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
                onCommit: updateCurrentDate();
            }
        } // of date elements row
    }

}

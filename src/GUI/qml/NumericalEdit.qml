import QtQuick 2.4
import FlightGear 1.0
import QtQuick.Window 2.0
import FlightGear.Launcher 1.0
import "."

FocusScope {
    id: root
    property string label
    property bool enabled: true
    property var quantity
    property bool live: false
    property alias unitsMode: units.mode

    UnitsModel {
        id: units
    }

    implicitHeight: editFrame.height
    // we have a margin between the frame and the label, and on each
    implicitWidth: label.width + editFrame.width + Style.margin

    signal commit(var newValue);

    function doCommit(newValue)
    {
        var q = quantity;
        q.value = newValue;
        commit(q);
    }

    function parseTextAsValue()
    {
        if (units.numDecimals == 0) {
            return parseInt(edit.text);
        }

        return parseFloat(edit.text);
    }

    function clampValue(newValue)
    {
        if (units.wraps) {
            // integer wrapping for now
            var range = (units.maxValue - units.minValue + 1);
            if (newValue < units.minValue) newValue += range;
            if (newValue > units.maxValue) newValue -= range;
        }

        return Math.min(Math.max(units.minValue, newValue), units.maxValue);
    }

    function incrementValue()
    {
        if (edit.activeFocus) {
            var newValue = clampValue(parseTextAsValue() + units.stepSize);
            edit.text = newValue
            if (live) {
                doCommit(newValue);
            }
        } else {
            doCommit(clampValue(quantity.value + units.stepSize));
        }
    }

    function decrementValue()
    {
        if (edit.activeFocus) {
            var newValue = clampValue(parseTextAsValue() - units.stepSize);
            edit.text = newValue
            if (live) {
                doCommit(newValue);
            }
        } else {
            doCommit(clampValue(quantity.value - units.stepSize));
        }
    }

    function changeToUnits(unitIndex)
    {
        if (edit.activeFocus) {
            // build up a quantity with the current text value, in the
            // previous units. We need to parse as text before we
            // change the selected unit
            var q = root.quantity;
            q.value = clampValue(parseTextAsValue());

            // convert to the new quantity
            units.selectedIndex = unitIndex;
            var newQuantity = q.convertToUnit(units.selectedUnit)

            console.info("Changing text to:" + newQuantity.value.toFixed(units.numDecimals));
            edit.text = newQuantity.value.toFixed(units.numDecimals)

            console.info("Change units commit:" + newQuantity.value)
            commit(newQuantity);
        } else {
            // not focused, easy
            units.selectedIndex = unitIndex;
            root.commit(root.quantity.convertToUnit(units.selectedUnit));
        }
    }

    function showUnitsMenu()
    {

        var screenPos = _launcher.mapToGlobal(editFrame, Qt.point(0, editFrame.height))
        var pop = popup.createObject(root, {x:screenPos.x, y:screenPos.y })
        tracker.window = pop;
        pop.show();
    }

    Component.onCompleted: {
        // ensure any initial value is accepted by our mode.
        // this stops people passing in completely wrong quantities
        if (!units.isUnitInMode(quantity.unit)) {
            console.warn("NumericalEdit: was inited with incorrect unit");
            var q = quantity;
            q.unit = units.selectedUnit;
            commit(q);
        }
    }

    onQuantityChanged: {
        if (units.isUnitInMode(quantity.unit)) {
            // ensure our units model is in sync
            units.selectedUnit = quantity.unit
        } else {
            console.warn("Passed illegal quantity");
            // reset back to something permitted
            var q = quantity;
            q.unit = units.selectedUnit;
            commit(q);
        }
    }

    TextMetrics {
        id: metrics
        text: units.maxTextForMetrics
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
                doCommit(parseTextAsValue());
            }
        }
    }

    Binding {
        when: !edit.activeFocus
        target: edit
        property: "text"
        value: root.quantity.value.toFixed(units.numDecimals)
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

        ClickableText {
            id: prefix
            visible: units.isPrefix
            enabled: root.enabled
            anchors.baseline: edit.baseline
            anchors.left: parent.left
            anchors.margins: Style.margin
            text: visible ? units.shortText : ""
            onClicked: root.showUnitsMenu()
            clickable: (units.numChoices > 1)
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

            validator: units.validator

            Keys.onUpPressed: {
                root.incrementValue();
            }

            Keys.onDownPressed: {
                root.decrementValue();
            }

            Keys.onReturnPressed: {
                root.focus = false;
                // will trigger onActiveFocusChanged
            }

            onActiveFocusChanged: {
                if (activeFocus) {
                    selectAll();
                } else {
                    doCommit(parseTextAsValue())
                    liveEditTimer.stop();
                }
            }

            onTextChanged: {
                if (activeFocus && root.live) {
                    liveEditTimer.restart();
                }
            }
        }

        ClickableText {
            id: suffix
            visible: !units.isPrefix
            enabled: root.enabled
            anchors.baseline: edit.baseline
            anchors.right: upDownArea.left
            text: visible ? units.shortText : ""
            onClicked: root.showUnitsMenu();
            clickable: (units.numChoices > 1)
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
                source: "qrc:///up-down-arrow"
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

    PopupWindowTracker {
        id: tracker
    }

    Component {
        id: popup
        Window {
            id: unitSelectionPopup
            flags: Qt.Popup
            color: "white"
            height: choicesColumn.childrenRect.height + Style.margin * 2
            width: choicesColumn.width + Style.margin * 2

            Rectangle {
                border.width: 1
                border.color: Style.minorFrameColor
                anchors.fill: parent
            }

            // choice layout column
            Column {
                id: choicesColumn
                spacing: Style.margin
                x: Style.margin
                y: Style.margin
                width: menuWidth


                function calculateMenuWidth()
                {
                    var minWidth = 0;
                    for (var i = 0; i < choicesRepeater.count; i++) {
                        minWidth = Math.max(minWidth, choicesRepeater.itemAt(i).implicitWidth);
                    }
                    return minWidth;
                }

                readonly property int menuWidth: calculateMenuWidth()

                // main item repeater
                Repeater {
                    id: choicesRepeater
                    model: units
                    delegate:
                        Text {
                            id: choiceText
                            readonly property bool selected: units.selectedIndex === model.index

                            text: model.longName
                            height: implicitHeight + Style.margin
                            font.pixelSize: Style.baseFontPixelSize
                            color: choiceArea.containsMouse ? Style.themeColor : Style.baseTextColor

                            MouseArea {
                                id: choiceArea
                                width: unitSelectionPopup.width // full width of the popup
                                height: parent.height
                                hoverEnabled: true

                                onClicked: {
                                    root.changeToUnits(model.index);
                                    unitSelectionPopup.close();
                                }
                            }
                        } // of Text delegate
                } // text repeater
            } // text column
        }
    } // of popup component
}

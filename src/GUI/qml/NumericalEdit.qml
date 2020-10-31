import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0
import "."

FocusScope {
    id: root
    property string label
    property bool enabled: true
    property var quantity
    property bool live: false
    property alias unitsMode: units.mode

    readonly property int microPadding: 2

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

            edit.text = newQuantity.value.toFixed(units.numDecimals)
            commit(newQuantity);
        } else {
            // not focused, easy
            units.selectedIndex = unitIndex;
            root.commit(root.quantity.convertToUnit(units.selectedUnit));
        }
    }

    function showUnitsMenu()
    {
        OverlayShared.globalOverlay.showOverlayAtItemOffset(menu, root,
                                                                Qt.point(editFrame.x, editFrame.height))
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
        width: edit.width + prefix.width + suffix.width + upDownArea.width + Style.margin * 2 + (microPadding * 2)
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
            anchors.leftMargin: microPadding

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
            anchors.rightMargin: microPadding

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

    Component {
        id: menu

        OverlayMenu {
            model: units
            displayRole: "longName"
            currentIndex: units.selectedIndex
            onSelect: root.changeToUnits(index);
        }
    }
}

import QtQuick 2.4
import FlightGear 1.0

FocusScope {
    id: root
    property Item nextToFocus
    property Item previousToFocus

    property int value: 0
    property int minValue: 0 // might be 1 for day-of-month
    property int maxValue: 99 // max numerical value

    property int fieldWidth: 2
    property bool valueWraps: false

    property string widthString: "00"

    width: metrics.width + (input.activeFocus ?  upDownArea.width : 0)
    height: metrics.height

    // promote Z value when focused so our icon decoration
    // is on top of our sibling edits.
    z: input.activeFocus ? 10 : 0

    signal commit(var newValue);

    onMaxValueChanged: {
        // cap current value if the max is now lower
        value = Math.min(value, maxValue);
    }

    function doCommit(newValue)
    {
        value = newValue;
        commit(newValue)
    }

    function zeroPaddedNumber(v)
    {
        var s = v.toString();
        while (s.length < fieldWidth) {
            s = "0" + s;
        }
        return s;
    }

    readonly property string __valueString: zeroPaddedNumber(value)

    function incrementValue()
    {
        if (input.activeFocus) {
            value = Math.min(parseInt(input.text) + 1, maxValue)
            input.text = __valueString
        } else {
            doCommit(Math.min(value + 1, maxValue));
        }
    }

    function decrementValue()
    {
        if (input.activeFocus) {
            value = Math.max(parseInt(input.text) - 1, minValue)
            input.text = __valueString
        } else {
            doCommit(Math.max(value - 1, minValue));
        }
    }

    function focusNext()
    {
        if (nextToFocus != undefined) {
            nextToFocus.forceActiveFocus();
        }
    }

    TextInput {
        id: input
        focus: true

        // validate on integers
        height: parent.height
        width: metrics.width
        maximumLength: fieldWidth
        font.pixelSize: Style.baseFontPixelSize

        Keys.onUpPressed: {
            incrementValue();
        }

        Keys.onDownPressed: {
            decrementValue();
        }

        Keys.onTabPressed:  { root.focusNext(); }

        Keys.onBacktabPressed:  {
            if (previousToFocus != undefined) {
                previousToFocus.focus = true;
            }
        }

        Keys.onPressed:  {
            if ((event.key === Qt.Key_Colon) || (event.key === Qt.Key_Slash)) {
                nextToFocus.focus = true;
                event.accepted = true;
            }
        }

//        onTextChanged: {
//            if (activeFocus && (text.length == root.widthString.length)) {
//                root.focusNext();
//            }
//        }

        onActiveFocusChanged: {
            if (activeFocus) {
                selectAll();
            } else {
                doCommit(parseInt(text))
            }
        }

        validator: IntValidator {
            id: validator
            top: root.maxValue
            bottom: root.minValue
        }
    }

    Binding {
        when: !input.activeFocus
        target: input
        property: "text"
        value: root.__valueString
    }

    MouseArea {
        height: root.height
        width: root.width
        // use wheel events to adjust up/dowm
        onClicked: {
            input.forceActiveFocus();
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

    Rectangle {
        id: upDownArea
        color: Style.backgroundColor
        anchors.left: input.right
        anchors.verticalCenter: input.verticalCenter
        height: upDownIcon.implicitHeight
        visible: input.activeFocus
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
        }

        MouseArea {
            width: parent.width
            height: parent.height / 2
            anchors.bottom: parent.bottom
            onPressed: {
                root.decrementValue();
            }
        }
    }

    TextMetrics {
        id: metrics
        text: root.widthString
    }
}

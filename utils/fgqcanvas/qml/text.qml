import QtQuick 2.0
import FlightGear 1.0

CanvasItem {
    id: root
//    implicitHeight: text.implicitHeight
 //   implicitWidth: text.implicitWidth

    property alias text: text.text
    property alias fontPixelSize: text.font.pixelSize
    property alias fontFamily: text.font.family
    property alias color: text.color

    property string canvasAlignment: "center"

    property string __canvasVAlign: "center"
    property string __canvasHAlign: "center"

    onCanvasAlignmentChanged: {
        if (canvasAlignment == "center") {
            __canvasVAlign = __canvasVAlign = "center";
        } else {
            var pieces = canvasAlignment.split("-");
            __canvasHAlign = pieces[0]
            __canvasVAlign = pieces[1]
        }
    }

    Text {
        id: text
    }

    Binding {
        when: __canvasHAlign == "left"
        target: text
        property: "anchors.left"
        value: root.left
    }

    Binding {
        when: __canvasHAlign == "right"
        target: text
        property: "anchors.right"
        value: root.left
    }

    Binding {
        when: __canvasHAlign == "center"
        target: text
        property: "anchors.horizontalCenter"
        value: root.left
    }

    Binding {
        when: __canvasVAlign == "top"
        target: text
        property: "anchors.top"
        value: root.top
    }

    Binding {
        when: __canvasVAlign == "bottom"
        target: text
        property: "anchors.bottom"
        value: root.top
    }

    Binding {
        when: __canvasVAlign == "center"
        target: text
        property: "anchors.verticalCenter"
        value: root.top
    }

    Binding {
        when: __canvasVAlign == "baseline"
        target: text
        property: "anchors.baseline"
        value: root.top
    }
}

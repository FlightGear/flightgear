import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Item {
    id: root
    property alias label: label.text

    property var model: nil
    property string displayRole: "display"
    property bool enabled: true
    property int currentIndex: 0
    property string headerText: ""

    implicitHeight: Math.max(label.implicitHeight, currentChoiceFrame.height)
    implicitWidth: label.implicitWidth + (Style.margin * 2) + currentChoiceFrame.__naturalWidth

    function select(index)
    {
        root.currentIndex = index;
    }

    ModelDataExtractor {
        id: currentItemText
        model: root.model
        role: root.displayRole
        index: root.currentIndex
    }

    function haveHeader()
    {
        return headerText !== "";
    }

    function currentText()
    {
        if ((currentIndex == -1) && haveHeader())
            return headerText;

        if (!currentItemText.data)
            return "";

        return currentItemText.data
    }

    StyledText {
        id: label
        anchors.left: root.left
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        horizontalAlignment: Text.AlignRight
        enabled: root.enabled
        hover: mouseArea.containsMouse
    }

    Rectangle {
        id: currentChoiceFrame
        radius: Style.roundRadius
        border.color: root.enabled ? (mouseArea.containsMouse ? Style.themeColor : Style.minorFrameColor)
                                   : Style.disabledMinorFrameColor
        border.width: 1
        height: currentChoiceText.implicitHeight + Style.margin
        clip: true
        color: Style.backgroundColor

        anchors.left: label.right
        anchors.leftMargin: Style.margin

        // width of current item, or available space after the label
        width: Math.min(root.width - (label.width + Style.margin), __naturalWidth);

        readonly property int __naturalWidth: currentChoiceText.implicitWidth + (Style.margin * 3) + upDownIcon.width
        anchors.verticalCenter: parent.verticalCenter

        StyledText {
            enabled: root.enabled
            id: currentChoiceText
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: Style.margin
            text: currentText()
            elide: Text.ElideRight
            maximumLineCount: 1
        }

        Image {
            id: upDownIcon
            visible: root.enabled
            source:  mouseArea.containsMouse ? "image://colored-icon/up-down?theme" : "image://colored-icon/up-down?text"
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.fill: parent
        id: mouseArea
        hoverEnabled: root.enabled
        enabled: root.enabled
        onClicked: {
            OverlayShared.globalOverlay.showOverlayAtItemOffset(menu, root,
                                                                Qt.point(currentChoiceFrame.x, root.height))
        }
    }

    Component {
        id: menu
        OverlayMenu {
            currentIndex: root.currentIndex
            model: root.model
            headerText: root.headerText
            onSelect: function doSelect(index) {
                root.select(index);
            }
            
            displayRole: root.displayRole
        }
    }


}

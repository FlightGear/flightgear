import QtQuick 2.4

Item {
    id: root

    readonly property bool dismissOnClickOutside: activeOverlayLoader.item && activeOverlayLoader.item.hasOwnProperty("dismissOnClickOutside") ?
                                                 activeOverlayLoader.item.dismissOnClickOutside : false

    property var __showPos: Qt.point(0,0)

    function showOverlay(comp)
    {
        __showPos = Qt.point(0,0)
        activeOverlayLoader.sourceComponent = comp;
        activeOverlayLoader.visible = true;
        activeOverlayLoader.x = __showPos.x;
        activeOverlayLoader.y = __showPos.y;
    }

    function showOverlayAtItemOffset(comp, item, offset)
    {
        __showPos = mapFromItem(item, offset.x, offset.y)
        
        activeOverlayLoader.sourceComponent = comp;
        activeOverlayLoader.visible = true;
        activeOverlayLoader.x = __showPos.x;
        activeOverlayLoader.y = __showPos.y;
    }

    function dismissOverlay()
    {
        activeOverlayLoader.sourceComponent = null
        activeOverlayLoader.visible = false;
    }

    function verticalOverflowFor(itemHeight)
    {
        // assumes we fill the whole window, so anything
        // extending past our bottom is overflowing
        var ov = (itemHeight + activeOverlayLoader.y) - root.height;
        return Math.max(ov, 0);
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        visible: activeOverlayLoader.visible && root.dismissOnClickOutside
        onClicked: root.dismissOverlay()
    }

    Loader
    {
        id: activeOverlayLoader
        // no size, size to the component
    }
}

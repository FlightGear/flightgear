import QtQuick 2.4

Item {
    id: root

    readonly property bool dismissOnClickOutside: activeOverlayLoader.item && activeOverlayLoader.item.hasOwnProperty("dismissOnClickOutside") ?
                                                 activeOverlayLoader.item.dismissOnClickOutside : false

    function showOverlay(comp)
    {
        activeOverlayLoader.sourceComponent = comp;
        activeOverlayLoader.visible = true;
    }

    function showOverlayAtItemOffset(comp, item, offset)
    {
        var pt = mapFromItem(item, offset.x, offset.y)
        activeOverlayLoader.sourceComponent = comp;
        activeOverlayLoader.visible = true;
        activeOverlayLoader.x = pt.x;
        activeOverlayLoader.y = pt.y;
    }

    function dismissOverlay()
    {
        activeOverlayLoader.sourceComponent = null
        activeOverlayLoader.visible = false;
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

        onStatusChanged: {
            if (status == Loader.Ready) {
                if (item.hasOwnProperty("adjustYPosition")) {
                    // setting position here doesn't work, so we use a 0-interval
                    // timer to do it shortly afterwards
                    adjustPosition.start();
                }
            }
        }

        function adjustY()
        {
            var overflowHeight = (y + item.height) - root.height;
            if (overflowHeight > 0) {
                activeOverlayLoader.y = Math.max(0, y - overflowHeight)
            }
        }

        Timer {
            id: adjustPosition
            interval: 0
            onTriggered: activeOverlayLoader.adjustY();
        }
    }
}

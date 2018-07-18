import QtQuick 2.4

Item {

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

    Loader
    {
        id: activeOverlayLoader
        // no size, size to the component
    }
}

import QtQuick 2.4
import FlightGear 1.0

Item {
    id: root
    visible: false

    function show()
    {
        computeMenuWidth();
        visible = true;
    }

    function close()
    {
        visible = false;
    }
    
    function computeMenuWidth()
    {
        var minWidth = 0;
        for (var i = 0; i < items.length; i++) {
            minWidth = Math.max(minWidth, items[i].minWidth());
        }

        menuFrame.width = minWidth;
    }

    property alias items: contentBox.children

// underlying mouse area to close menu when clicking outside
// this also eats hover events
    MouseArea {
        x: -1000
        y: -1000
        width: 2000
        height: 2000
        hoverEnabled: true
        onClicked: root.close()
    }

    width: 1
    height: 1

    Rectangle {
        id: menuFrame
        border.width: 1
        border.color: Style.minorFrameColor
        height: contentBox.childrenRect.height + 2
        color: Style.panelBackground

        // width is computed during show(), but if we don't see a valid
        // value here, the Column doesn't position the items until
        // slightly late
        width: 100
        

        Column {
            id: contentBox
            x: 1
            y: 1
            width: parent.width - 2

// helper function called by BaseMenuItem on its parent, i.e,
// us, to request closing the menu
            function requestClose()
            {
                root.close();
            }

            function getMenu() {
                return root;
            }

            // menu items get inserted here
        }
    }
}

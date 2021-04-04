import QtQuick 2.4
import FlightGear 1.0

Item
{
    property bool enabled: true
    
    implicitHeight: Style.menuItemHeight
    width: parent.width // take width from our parent menu

    function minWidth() { return 0; }   

    function closeMenu()
    {
        parent.requestClose();
    }

    function menu()
    {
        return parent.getMenu();
    }
}

import QtQuick 2.4

// dummy item for Qt >= 5.7, where we use the real Qt Quick Controls 2 scrollbar
// we need to be a real Item so anchors, etc work, but we set our width to zero
Item
{
    property var flickable: nil
    width: 0
}

import QtQuick 2.4

import "."

Rectangle {
    id: root
    color: Style.themeColor

    property alias pagesModel: buttonRepeater.model
    property int selectedPage: 0

    signal selectPage(var pageSource);

    function setSelectedPage(index)
    {
        selectedPage = index
    }

    Column {
        width: parent.width
        anchors.top: parent.top
        anchors.topMargin: Style.margin
        anchors.bottom: flyButton.top

        Repeater {
            id: buttonRepeater

            delegate: SidebarButton {
                icon: model.iconPath
                label: model.title
                onClicked: {
                    root.selectedPage = model.index
                    root.selectPage(model.pageSource);
                }

                selected: (model.index === root.selectedPage)
            }
        }
    }

    SidebarButton {
        id: flyButton
        label: qsTr("Fly!")
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Style.margin
        enabled: _launcher.canFly
        icon: "qrc:///toolbox-fly"
        onClicked: _launcher.fly();
    }
}

import QtQuick 2.4

import "."

Rectangle {
    id: root
    color: Style.themeColor

    property alias pagesModel: buttonRepeater.model
    property int selectedPage: 0
    property alias showMenuIcon: menuIcon.visible

    readonly property int minimumHeight: mainColumn.childrenRect.height + flyButton.height

    signal selectPage(var pageSource);

    signal showMenu();

    function setSelectedPage(index)
    {
        selectedPage = index
    }

    Column {
        id: mainColumn
        width: parent.width
        anchors.top: parent.top
        anchors.bottom: flyButton.top
        //spacing: Style.margin

        Rectangle {
            id: menuIcon
            width: parent.width
            height:menuIconImage.sourceSize.height
            color: menuMouseArea.containsMouse ? Style.activeColor : "transparent"

            Image {
                id: menuIconImage
                anchors.centerIn: parent
                source: "qrc:///ellipsis-icon"
            }

            MouseArea {
                id: menuMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.showMenu()
            }
        }

        Repeater {
            id: buttonRepeater

            delegate: SidebarButton {
                icon: model.iconPath
                label: model.title
                onClicked: {
                    root.selectedPage = model.index
                    root.selectPage(model.pageSource);
                }

                enabled: !model.buttonDisabled
                disabledText: (model.disabledText !== undefined) ? model.disabledText : ""
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
        disabledText: qsTr("The selected aircraft is not installed")
        icon: "qrc:///toolbox-fly"
        onClicked: _launcher.fly();
    }
}

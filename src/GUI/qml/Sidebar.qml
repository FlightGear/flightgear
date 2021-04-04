import QtQuick 2.4
import QtQml 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Rectangle {
    id: root
    color: Style.themeColor

    property alias pagesModel: buttonRepeater.model
    property int selectedPage: 0
    property alias showMenuIcon: menuIcon.visible

    readonly property int minimumHeight: mainColumn.childrenRect.height + flyButton.height

    implicitWidth: flyButton.width

    signal selectPage(var pageSource);

    signal showMenu();

    function setSelectedPage(index)
    {
        selectedPage = index
    }

    GettingStartedScope.controller: tips.controller

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
                source: "image://colored-icon/ellipsis?themeContrast"
            }

            MouseArea {
                id: menuMouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.showMenu()
            }


            GettingStartedTip {
                id: menuTip
                tipId: "sidebarMenuTip"
                arrow: GettingStartedTip.TopLeft

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    horizontalCenterOffset: Style.margin
                    top: parent.bottom
                }
                text: qsTr("Access additional options here")
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
        label: _launcher.flyButtonLabel
        anchors.bottom: parent.bottom
        enabled: _launcher.canFly
        disabledText: qsTr("The selected aircraft is not installed or has updates pending")
        icon: _launcher.flyIconUrl
        onClicked: _launcher.fly();
    }

    GettingStartedTipLayer {
        id: tips
        anchors.fill: parent
        scopeId: "sidebar"
    }
}

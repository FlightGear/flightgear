import QtQuick 2.4

import FlightGear.Launcher 1.0
import FlightGear 1.0

Item {
    id: root
    property alias title: headerTitle.text
    property alias contents: contentBox.children
    property alias showAdvanced: advancedToggle.open
    property string settingGroup: ""
    property string summary: ""
    readonly property bool haveAdvancedSettings: anyAdvancedSettings(contents)

    implicitWidth: parent.width
    implicitHeight: headerRect.height + contentBox.height + (Style.margin * 2)

    signal apply();

    function saveState()
    {
        for (var i = 0; i < contents.length; i++) {
            contents[i].saveState();
        }
    }

    function anyAdvancedSettings(items)
    {
        for (var i = 0; i < items.length; i++) {
            if (items[i].advanced === true) return true;
        }

        return false;
    }

    // we determine the initial open/close state of the advanced section
    // based on whether any of the advanced settings are non-default
    function anyNonDefaultAdvancedSettings(items)
    {
        for (var i = 0; i < items.length; i++) {
            var control = items[i];
            if (control.advanced === true) {
                if (!control.__isDefault && !control.hidden) {
                    //console.info("Non-default advanced setting:" + control.label + ","
                    //             + control.defaultValue + " != " + control.value) ;
                    return true;
                }
             }
        }

        return false;
    }

    Connections {
        target: _config
        function onCollect() { root.apply(); }
    }

    Component.onCompleted: {
        // use this as a trigger to decide the initial open/close state
        if (anyNonDefaultAdvancedSettings(contents)) {
            showAdvanced = true;
        }
    }

    Rectangle {
        id: headerRect
        width: parent.width
        height: headerTitle.height + (Style.margin * 2)

        color: Style.themeColor
        border.width: 1
        border.color: Style.frameColor

        Text {
            id: headerTitle
            color: Style.themeContrastTextColor
            anchors.verticalCenter: parent.verticalCenter
            font.bold: true
            font.pixelSize: Style.subHeadingFontPixelSize
            anchors.left: parent.left
            anchors.leftMargin: Style.inset
        }

        AdvancedSettingsToggle
        {
            id: advancedToggle
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            height: parent.height
            visible: root.haveAdvancedSettings

            GettingStartedTip {
                tipId: "expandSectionTip"
                enabled: root.haveAdvancedSettings

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.bottom
                }
                arrow: GettingStartedTip.TopRight
                text: qsTr("Click here to show advanced settings in this section")
            }
        }
    }


    MouseArea {
        anchors.fill: contentBox
        onClicked: {
            // take focus back from any active control
            root.focus = true
        }
    }

    Column {
        id: contentBox
        anchors.top: headerRect.bottom
        anchors.topMargin: Style.margin
        width: parent.width
        spacing: Style.margin * 2

        // this is here so SettingControl 's parent (which is us)
        // can be used to find the advanced toggle state
        property alias showAdvanced: advancedToggle.open
    }

    // bottom spacing item
    Item {
        height: Style.margin
        width: parent.width
        anchors.top: contentBox.bottom
    }
}

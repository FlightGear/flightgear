import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0
import "."

Item {
    id: root

    Rectangle {
        id: content
        color: Style.themeColor
        anchors.fill: parent

    }

    Flickable {
        id: flick

        contentHeight: textColumn.childrenRect.height + imageClipFrame.height + (Style.strutSize * 2)
        flickableDirection: Flickable.VerticalFlick

        anchors.fill: parent


        Item  {
            id: imageClipFrame
            clip: true

            height: root.height * 0.5
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
            }

            PreviewImage {
                id: preview

                anchors.centerIn: parent

                // over-zoom the preview to fill the entire space available
                readonly property double scale: Math.max(root.width / sourceSize.width,
                                                         imageClipFrame.height / sourceSize.height)

                width: height * aspectRatio
                height: scale * sourceSize.height

                property var urlsList: _launcher.defaultSplashUrls()
                imageUrl: urlsList[0]
            }

            Text {
                id: logoText
                width: parent.width - Style.strutSize

                anchors {
                    top: parent.top
                    topMargin: Style.strutSize
                }

                font.pointSize: Style.strutSize * 3
                font.italic: true
                font.bold: true

                horizontalAlignment: Text.AlignHCenter
                fontSizeMode: Text.Fit
                text: "FlightGear " + _launcher.versionString
                color: "white"
                style: Text.Outline
                styleColor: "black"
            }
        }



        Column {
            id: textColumn
            anchors {
                left: parent.left
                right: parent.right
                top: imageClipFrame.bottom

                margins: Style.strutSize
            }

            spacing: Style.strutSize

            Text {
                width: parent.width
                font.pixelSize: Style.baseFontPixelSize * 1.5
                color: "white"
                wrapMode: Text.WordWrap

                readonly property string forumLink: "href=\"https://forum.flightgear.org\"";

                text: qsTr("Welcome to FlightGear, the open source flight simulator. " +
                           "This software is the work of volunteers. We hope you enjoy it. " +
                           "If you find problems or would like to contribute, " +
                           "please <a %1>visit our forum</a>.").arg(forumLink)

                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }

            Text {
                width: parent.width
                font.pixelSize: Style.baseFontPixelSize * 1.5
                color: "white"
                wrapMode: Text.WordWrap

                readonly property string gplLink: "href=\"https://www.gnu.org/licenses/gpl-3.0.html\"";

                text: qsTr("FlightGear is Free software, licensed under the <a %1>GNU General Public License</a>. " +
                           "You are free to use, customize and fix the software; and share your changes with the community.").arg(gplLink)

                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }

            Text {
                width: parent.width
                font.pixelSize: Style.baseFontPixelSize * 1.5
                color: "white"

                wrapMode: Text.WordWrap

                text: qsTr("FlightGear can automatically report crashes and errors to the development team, " +
                           "which helps to improve the software for everyone. This reporting is anonymous but " +
                           "contains information such as the aircraft in use, your operating system and graphics driver. " +
                           "You can enable or disable this reporting in the 'Settings' page."
                           )
            }

            Item {
                width: parent.width
                height: childrenRect.height

                Button {
                    invertColor: true
                    text: qsTr("Okay");
                    anchors.right: parent.right

                    onClicked: {
                        OverlayShared.globalOverlay.dismissOverlay();
                    }
                }
            }
        } // of column

    } // of Flickable


    Scrollbar {
        id: scrollbar
        anchors.right: parent.right
        height: flick.height
        flickable: flick
        visible: flick.contentHeight > flick.height
    }

} // of Item

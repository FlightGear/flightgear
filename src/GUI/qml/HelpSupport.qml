import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {
    id: root

    readonly property string forumLink: "href=\"http://forum.flightgear.org\"";
    readonly property string wikiLink: "href=\"http://wiki.flightgear.org\"";

    Flickable
    {
        id: flick

        contentHeight: contentColumn.childrenRect.height
        flickableDirection: Flickable.VerticalFlick
        height: parent.height
        width: parent.width - (Style.strutSize * 4 + scrollbar.width)
        x: Style.strutSize * 2
        y: Style.strutSize

        Column {
            id: contentColumn
            spacing: Style.strutSize
            width: parent.width

            Text {
                width: parent.width
                font.pixelSize: Style.baseFontPixelSize * 1.5
                color: Style.baseTextColor
                wrapMode: Text.WordWrap

                text: qsTr("<p>FlightGear is open-source software, developed entirely by volunteers. This means " +
                           "we can't offer the same kind of support as a commercial product, but we have " +
                           "an excellent user community, many of whom are willing to help out " +
                           "other users. The easiest place to ask questions and get support is at <a %1>our forums</a>.</p>\n" +
                           "<p>To get started with the simulator, please use our tutorial system: " +
                           "this is available from the 'Help' menu in the simulator, and teaches the basics of " +
                           "getting the Cessna 172 airborne and flying a standard circuit.</p>\n" +
                           "<p>Another good source of information is <a %2>the official wiki</a>, which contains " +
                           "FAQs, tutorials and information on individual aircraft, scenery areas and more."
                           ).arg(root.forumLink).arg(root.wikiLink)

                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }

            Text {
                width: parent.width
                font.pixelSize: Style.baseFontPixelSize * 1.5
                color: Style.baseTextColor
                wrapMode: Text.WordWrap

                readonly property string bugTrackerLink: "href=\"https://sourceforge.net/p/flightgear/codetickets/new/\"";
                readonly property string sceneryDBLink: "href=\"https://scenery.flightgear.org\"";

                text: qsTr("<p>If you've found a bug, please consider if it's in a particular aircraft, " +
                           "the scenery, or in the main program. Aircraft are developed by many different people, " +
                           "so <a %1>our forums</a> are the best way to identify the author(s) and contact them. For bugs in the " +
                           "program, please check our <a %2>bug tracker</a>; first by searching for existing bugs, and " +
                           "then creating a new ticket if necessary.</p>\n" +
                           "<p>Due to the automated way we generate our world scenery, it's usually not possible " +
                           "to fix individual data issues, such as missing roads or misplaced coastlines. For 3D models " +
                           "placed in the scenery, you can submit improvements to <a %3>our scenery database.</a>" +
                           "</p>"
                            ).arg(root.forumLink).arg(bugTrackerLink).arg(sceneryDBLink)

                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }
        }

    } // of flickable

    Scrollbar {
        id: scrollbar
        anchors.right: parent.right
        height: flick.height
        flickable: flick
        visible: flick.contentHeight > flick.height
    }
}

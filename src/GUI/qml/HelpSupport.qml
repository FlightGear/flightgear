import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {
    id: root

    readonly property string forumLink: "href=\"https://forum.flightgear.org\"";
    readonly property string forumHelpLink: "href=\"https://forum.flightgear.org/viewforum.php?f=17\"";

    readonly property string wikiLink: "href=\"http://wiki.flightgear.org/Portal:User\"";

    ScrolledFlickable
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

                readonly property var pdfManualLink: "href=\"" + _launcher.urlToDataPath("Docs/getstart.pdf") + "\""
                readonly property var shortRefLink: "href=\"" +  _launcher.urlToDataPath("Docs/FGShortRef.html") +  "\""

                text: qsTr("<p>FlightGear is open source software, developed entirely by volunteers. " +
                           "Support is provided by our excellent user community. " +
                           "The easiest place to ask questions and get support is on <a %1>our forums</a>.</p>\n" +
                           "<p>To get started with the simulator, please use our tutorial system: " +
                           "this is available from the 'Help' menu in the simulator. We recommend " +
                           "starting with the Cessna 172 to learn how to get airborne.</p>\n" +
                           "<p>Other good resources:<ul>" +
                           "<li><a %2>the official manual (PDF)</a></li>\n" +
                           "<li><a %3>key commands reference</a></li>\n" +
                           "<li><a %4>our wiki</a> (which includes FAQs)</li>\n" +
                           "</ul></p>"
                           ).arg(root.forumLink).arg(pdfManualLink).arg(shortRefLink).arg(root.wikiLink)

                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }

            Text {
                width: parent.width
                font.pixelSize: Style.baseFontPixelSize * 1.5
                color: Style.baseTextColor
                wrapMode: Text.WordWrap

                text: qsTr("<p>If you find an issue, please use <a %1>our help forum</a>.</p>\n").arg(root.forumHelpLink);

                onLinkActivated: {
                    Qt.openUrlExternally(link)
                }
            }

            Text {
                width: parent.width
                font.pixelSize: Style.baseFontPixelSize * 1.5
                color: Style.baseTextColor
                wrapMode: Text.WordWrap

                text: qsTr("<p>FlightGear is the result of twenty-five years of work " +
                           "by hundreds of contributors around the world. We'd be " +
                           "delighted if you would join us.</p>\n")

//                onLinkActivated: {
//                    Qt.openUrlExternally(link)
//                }
            }
        }

    } // of flickable

    FGCompatScrollbar {
        id: scrollbar
        anchors.right: parent.right
        height: flick.height
        flickable: flick
        visible: flick.contentHeight > flick.height
    }
}

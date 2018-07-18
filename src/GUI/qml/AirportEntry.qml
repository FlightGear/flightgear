import QtQuick 2.4
import QtQuick.Window 2.0
import FlightGear 1.0
import FlightGear.Launcher 1.0
import "."

LineEdit
{
    id: root
    placeholder: "KSFO"
    suggestedWidthString: "XXXX"

    Positioned {
        id: airport
    }

    function selectAirport(guid)
    {
        airport.guid = guid
        text = airport.ident
        // we don't want this to trigger a search ....
        searchTimer.stop();
    }

    onActiveFocusChanged: {
        if (activeFocus) {
            OverlayShared.globalOverlay.showOverlayAtItemOffset(overlay, root, Qt.point(xOffsetForEditFrame, root.height + Style.margin))
        } else {
            OverlayShared.globalOverlay.dismissOverlay()
            searchCompleter.clear();
            if (!airport.valid) {
                text = ""; // ensure we always contain a valid ICAO or nothing
                // we don't want this to trigger a search ....
                searchTimer.stop();
            }
        }
    }

    NavaidSearch {
        id: searchCompleter
        airportsOnly: true
        maxResults: 20

        onSearchComplete: {
            if (exactMatch !== 0) {
                selectAirport(exactMatch)
                return;
            }
        }
    }

    onTextChanged: {
        searchTimer.restart();
    }

    Timer {
        id: searchTimer
        interval: 400
        onTriggered: {
            if (root.text.length >= 2) {
                searchCompleter.setSearch(root.text, NavaidSearch.Airplane)
            } else {
                // ensure we update with no search (cancel, effectively)
                searchCompleter.clear();
            }
        }
    }

    StyledText {
        anchors.left: parent.right
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        text: airport.name
        visible: airport.valid
        width: Style.strutSize * 3
        elide: Text.ElideRight
    }

    Component {
        id: overlay

        Rectangle {
            id: selectionPopup
            color: "white"
            height: choicesColumn.childrenRect.height + Style.margin * 2
            width: choicesColumn.width + Style.margin * 2

            visible: searchCompleter.haveExistingSearch

            Rectangle {
                border.width: 1
                border.color: Style.minorFrameColor
                anchors.fill: parent
            }

            // choice layout column
            Column {
                id: choicesColumn
                spacing: Style.margin
                x: Style.margin
                y: Style.margin
                width: menuWidth


                function calculateMenuWidth()
                {
                    var minWidth = 0;
                    for (var i = 0; i < choicesRepeater.count; i++) {
                        minWidth = Math.max(minWidth, choicesRepeater.itemAt(i).implicitWidth);
                    }
                    return minWidth;
                }

                readonly property int menuWidth: calculateMenuWidth()

                // main item repeater
                Repeater {
                    id: choicesRepeater
                    model: searchCompleter
                    delegate:
                        Text {
                            id: choiceText

                            text: model.ident + " - " + model.name
                            height: implicitHeight + Style.margin
                            font.pixelSize: Style.baseFontPixelSize
                            color: choiceArea.containsMouse ? Style.themeColor : Style.baseTextColor

                            MouseArea {
                                id: choiceArea
                                width: selectionPopup.width // full width of the popup
                                height: parent.height
                                hoverEnabled: true

                                onClicked: {
                                    root.selectAirport(model.guid)
                                    root.focus = false
                                }
                            }
                        } // of Text delegate
                } // text repeater
            } // text column
        }
    } // of overlay component
}

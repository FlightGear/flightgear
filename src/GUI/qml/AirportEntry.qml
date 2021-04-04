import QtQuick 2.4
import FlightGear 1.0
import FlightGear.Launcher 1.0 

FocusScope
{
    id: root
    readonly property Positioned airport: airport
    implicitWidth: edit.implicitWidth + nameDisplay.width
    implicitHeight: edit.implicitHeight

    property alias label: edit.label

    signal clickedName();

    Positioned {
        id: airport
    }

    function selectAirport(guid)
    {
        airport.guid = guid
        edit.text = airport.ident
        // we don't want this to trigger a search ....
        searchTimer.stop();
    }

    signal pickAirport(var guid);

    NavaidSearch {
        id: searchCompleter
        airportsOnly: true
        maxResults: 20

        onSearchComplete: {
            if (exactMatch !== 0) {
                pickAirport(exactMatch)
                return;
            }
        }
    }

    Timer {
        id: searchTimer
        interval: 400
        onTriggered: {
            if (edit.text.length >= 2) {
                searchCompleter.setSearch(edit.text, _launcher.aircraftType)
            } else {
                // ensure we update with no search (cancel, effectively)
                searchCompleter.clear();
            }
        }
    }


    LineEdit
    {
        id: edit
        placeholder: "KSFO"
        suggestedWidthString: "XXXX"
        commitOnReturn: false
        focus: true

        onActiveFocusChanged: {
            if (activeFocus) {
                OverlayShared.globalOverlay.showOverlayAtItemOffset(overlay, root,
                                                                    Qt.point(xOffsetForEditFrame, root.height + Style.margin),
                                                                    false /* don't offset */);
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

        onTextChanged: {
            searchTimer.restart();
        }

        Keys.onReturnPressed: {
            // start a search right now, if we don't have one active
            if (!searchCompleter.haveExistingSearch && (edit.text.length >= 2)) {
                searchCompleter.setSearch(edit.text, _launcher.aircraftType)
            }

            // if we have a search and at least one valid result, select it
            // combined with the previous behaviour, this means entering an ICAO
            // code (which are returend synchronously inside setSearch) and
            // hitting enter/return works as expected
            if (searchCompleter.haveExistingSearch && (searchCompleter.numResults > 0)) {
                root.pickAirport(searchCompleter.guidAtIndex(0));
                root.focus = false
            }
        }
    }

    ClickableText {
        id: nameDisplay
        anchors.left: edit.right
        anchors.leftMargin: Style.margin
        anchors.verticalCenter: parent.verticalCenter
        text: airport.name
        visible: airport.valid
        width: Style.strutSize * 3
        elide: Text.ElideRight
        onClicked: root.clickedName()
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

                            readonly property bool isCurrent: (selectionPopup.currentIndex === model.index)
                            color: choiceArea.containsMouse ? Style.themeColor : Style.baseTextColor

                            MouseArea {
                                id: choiceArea
                                width: selectionPopup.width // full width of the popup
                                height: parent.height
                                hoverEnabled: true

                                onClicked: {
                                    root.pickAirport(model.guid)
                                    root.focus = false
                                }
                            }
                        } // of Text delegate
                } // text repeater
            } // text column
        }
    } // of overlay component
}

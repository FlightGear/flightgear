import QtQuick 2.4
import QtQml 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

Item {
    id: root

    signal showSelectedAircraft();
    signal showSelectedLocation();
    signal showFlightPlan();

    GettingStartedScope.controller: tips.controller

    readonly property string __aircraftDescription: _launcher.selectedAircraftInfo.description

    PreviewImage {
        id: preview
        anchors.centerIn: parent

        // over-zoom the preview to fill the entire space available
        readonly property double scale: Math.max(root.width / sourceSize.width,
                                                         root.height / sourceSize.height)

        width: height * aspectRatio
        height: scale * sourceSize.height

        property var urlsList: _launcher.defaultSplashUrls()
        property int __currentUrl: 0

        Binding on urlsList {
            when: _launcher.selectedAircraftInfo.previews.length > 0
            value: _launcher.selectedAircraftInfo.previews
        }

        onUrlsListChanged: {
            var len = preview.urlsList.length;
            __currentUrl = Math.floor(Math.random() * len)
        }

        Timer {
            running: true
            interval: 8000
            repeat: true
            onTriggered: {
                var len = preview.urlsList.length
                preview.__currentUrl = (preview.__currentUrl + 1) % len
            }
        }

        function currentUrl()
        {
            if (urlsList.length <= __currentUrl) return "";
            return urlsList[__currentUrl];
        }

        visible: imageUrl != ""
        imageUrl: currentUrl()
    }

    Text {
        id: logoText
        font.pointSize: Style.strutSize * 2
        font.italic: true
        font.bold: true
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: Style.strutSize
        }
        fontSizeMode: Text.Fit
        text: "FlightGear " + _launcher.versionString
        color: "white"
        style: Text.Outline
        styleColor: "black"
    }

    ClickableText {
        anchors {
            left: logoText.left
            right: logoText.right
        }

        // anchoring to logoText bottom doesn't work as expected because of
        // dynamic text sizing, so bind it manually
        y: logoText.y + Style.margin + logoText.contentHeight
        wrapMode: Text.WordWrap
        text: qsTr("Licenced under the GNU Public License (GPL) - click for more info")
        baseTextColor: "white"
        style: Text.Outline
        styleColor: "black"
        font.bold: true
        font.pixelSize: Style.subHeadingFontPixelSize

        onClicked: {
            _launcher.launchUrl("http://home.flightgear.org/about/");
        }
    }


    Rectangle {
        id: summaryPanel

        color: "transparent"
        border.width: 1
        border.color: Style.frameColor
        clip: true

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: Style.strutSize
        }

        height: summaryGrid.height + Style.margin * 2

        // set opacity here only, so we don't make the whole summary pannel translucent
        Rectangle {
            id: background
            anchors.fill: parent
            z: -1
            opacity: Style.panelOpacity
            color: Style.panelBackground
        }

        Grid {
            id: summaryGrid
            columns: 3
            columnSpacing: Style.margin
            rowSpacing: Style.margin

            readonly property int middleColumnWidth: summaryGrid.width - (locationLabel.width + Style.margin * 2 + aircraftHistoryPopup.width)

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: Style.margin
            }

            // aircraft name row
            StyledText {
                text: qsTr("Aircraft:")
                horizontalAlignment: Text.AlignRight
                font.pixelSize: Style.headingFontPixelSize
            }

            ClickableText {
                text: {
                    if (_launcher.skipAircraftFromArgs) return qsTr("<i>set from user arguments (in Settings)</i>");
                    if (_launcher.selectedAircraftInfo.name === "") return  qsTr("No aircraft selected"); 
                    return _launcher.selectedAircraftInfo.name;
                    }
                enabled: _launcher.selectedAircraftInfo.name !== "" && !_launcher.skipAircraftFromArgs
                font.pixelSize: Style.headingFontPixelSize

                onClicked: root.showSelectedAircraft();
            }

            HistoryPopup {
                id: aircraftHistoryPopup
                model: _launcher.aircraftHistory
                enabled: !_launcher.aircraftHistory.isEmpty && !_launcher.skipAircraftFromArgs
                onSelected: {
                    _launcher.selectedAircraft = _launcher.aircraftHistory.uriAt(index)
                }

                GettingStartedTip {
                    tipId: "aircraftHistoryTip"
                    nextTip: "locationHistoryTip"
                    arrow: GettingStartedTip.BottomRight

                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        bottom: parent.top
                    }
                    text: qsTr("Access recently used aircraft.")
                }
            }

            // empty space in next row (thumbnail, long aircraft description)
            Item {
                width: 1; height: 1
                visible: aircraftDetailsRow.visible
            }

            Item {
                id: aircraftDetailsRow
                width: summaryGrid.middleColumnWidth
                height: Math.max(thumbnail.height, aircraftDescriptionText.height)
                visible: !_launcher.skipAircraftFromArgs

                ThumbnailImage {
                    id: thumbnail
                    aircraftUri: _launcher.selectedAircraft
                    maximumSize.width: 172
                    maximumSize.height: 128
                }

                StyledText {
                    id: aircraftDescriptionText
                    anchors {
                        left: thumbnail.right
                        leftMargin: Style.margin
                        right: parent.right
                    }

                    text: root.__aircraftDescription
                    visible: root.__aircraftDescription != ""
                    wrapMode: Text.WordWrap
                    maximumLineCount: 5
                    elide: Text.ElideRight
                }
            }

            Item {
                width: 1; height: 1
                visible: aircraftDetailsRow.visible
            }

            // aircraft state row, if enabled
            Item {
               width: 1; height: 1
               visible: stateSelectionGroup.visible
           }

            Column {
                id: stateSelectionGroup
                visible: _launcher.selectedAircraftInfo.hasStates && !_launcher.skipAircraftFromArgs
                width: summaryGrid.middleColumnWidth
                spacing: Style.margin

                Component.onCompleted: updateComboFromController();

                function updateComboFromController()
                {
                    stateSelectionCombo.currentIndex = _launcher.selectedAircraftInfo.statesModel.indexForTag(_launcher.selectedAircraftState)
                }

                PopupChoice {
                    id: stateSelectionCombo
                    model: _launcher.selectedAircraftInfo.statesModel
                    displayRole: "name"
                    label: qsTr("State:")
                    width: parent.width   
                    headerText: qsTr("Default state")

                    function select(index)
                    {
                        if (index === -1) {
                            _launcher.selectedAircraftState = "";
                        } else {
                            _launcher.selectedAircraftState = model.tagForState(index);
                        }
                    }

                    GettingStartedTip {
                        id: aircraftStateTip
                        tipId: "aircraftStateTip"
                        arrow: GettingStartedTip.LeftCenter
                        enabled: stateSelectionGroup.visible
                        standalone: true

                        // show as a one-shot tip, first time we're enabled
                        onEnabledChanged: {
                            if (enabled) {
                                tips.showOneShot(this)
                            }
                        }

                        x: parent.implicitWidth
                        anchors {
                            verticalCenter: parent.verticalCenter
                        }
                        text: qsTr("Choose the starting condition of the aircraft")
                    }
                }

                StyledText {
                    id: stateDescriptionText
                    wrapMode: Text.WordWrap
                    maximumLineCount: 5
                    elide: Text.ElideRight
                    width: parent.width
                    text: _launcher.selectedAircraftInfo.statesModel.descriptionForState(stateSelectionCombo.currentIndex)
                }

                Connections {
                    target: _launcher.selectedAircraftInfo
                    onInfoChanged: stateSelectionGroup.updateComboFromController()
                }

                Connections {
                    target: _launcher
                    onSelectedAircraftStateChanged: stateSelectionGroup.updateComboFromController()
                } // of connections
            }

            Item {
                width: 1; height: 1
                visible: stateSelectionGroup.visible
            }

            // location summary row
            StyledText {
                id: locationLabel
                text: qsTr("Location:")
                horizontalAlignment: Text.AlignRight
                font.pixelSize: Style.headingFontPixelSize
            }

            ClickableText {
                enabled: !_location.skipFromArgs
                text: _location.skipFromArgs ? qsTr("<i>set from user arguments (in Settings)</i>")
                                             : _launcher.location.description
                font.pixelSize: Style.headingFontPixelSize
                width: summaryGrid.middleColumnWidth
                onClicked: root.showSelectedLocation()

                GettingStartedTip {
                    tipId: "currentLocationTextTip"
                    anchors {
                        top: parent.bottom
                        left: parent.left
                        leftMargin: Style.strutSize
                    }
                    arrow: GettingStartedTip.TopLeft
                    text: qsTr("Click to view and change the current location.")
                }
            }

            HistoryPopup {
                id: locationHistoryPopup
                model: _launcher.locationHistory
                enabled: !_launcher.aircraftHistory.isEmpty && !_location.skipFromArgs
                onSelected: {
                    _launcher.restoreLocation(_launcher.locationHistory.locationAt(index))
                }

                GettingStartedTip {
                    tipId: "locationHistoryTip"
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                        bottom: parent.top
                    }
                    arrow: GettingStartedTip.BottomRight
                    text: qsTr("Access recently used locations")
                }
            }

            // flight plan summary row
            StyledText {
                id: flightPlanLabel
                text: qsTr("Flight Plan:")
                horizontalAlignment: Text.AlignRight
                font.pixelSize: Style.headingFontPixelSize
                visible: _launcher.flightPlan.enabled
            }

            ClickableText {
                text: _launcher.flightPlan.description
                font.pixelSize: Style.headingFontPixelSize
                width: summaryGrid.middleColumnWidth
                onClicked: root.showFlightPlan()
                visible: _launcher.flightPlan.enabled
            }

            Item { // padding item for flight plan row
                width: 1; height: 1
                visible: _launcher.flightPlan.enabled
            }

            // settings summary row
            StyledText {
                text: qsTr("Settings:")
                horizontalAlignment: Text.AlignRight
                font.pixelSize: Style.headingFontPixelSize
                visible: !_launcher.inAppMode
            }

            StyledText {
                text: _launcher.combinedSummary.join(", ")
                font.pixelSize: Style.headingFontPixelSize
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                width: summaryGrid.middleColumnWidth
                visible: !_launcher.inAppMode
            }

            Item {
                width: 1; height: 1
            }
        }
    } // of summary box

    GettingStartedTipLayer {
        id: tips
        anchors.fill: parent
        scopeId: "summary"
    }
}

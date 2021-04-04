import QtQuick 2.2
import FlightGear.Launcher 1.0 as FG
import FlightGear 1.0

ListHeaderBox
{
    id: root
    signal clearSelection();

    GettingStartedScope.controller: tips.controller

    contents: [

        ToggleSwitch {
            id: doFilterCheck
            checked: _launcher.browseAircraftModel.ratingsFilterEnabled

            onCheckedChanged: {
                _launcher.browseAircraftModel.ratingsFilterEnabled = checked
                 _launcher.saveUISetting("enable-ratings-filter", checked);
            }

            label: qsTr("Filter using ratings")
            anchors.verticalCenter: parent.verticalCenter
        },

        StyledText {
            anchors {
                verticalCenter: parent.verticalCenter
                leftMargin: Style.margin
                left: doFilterCheck.right
                right: adjustRatingsText.left
            }
            text: _launcher.browseAircraftModel.summaryText
        },

        ClickableText {
            id: adjustRatingsText
            anchors.right: parent.right
            anchors.rightMargin: Style.margin
            text: qsTr("Adjust minimum ratings")
            anchors.verticalCenter: parent.verticalCenter
            onClicked:  {
                // clear selection so we don't jump to the selected item
                // each time the proxy model changes.
                root.clearSelection();
                editRatingsPanel.visible = true

            }

            Component.onCompleted: {
                editTip.showOneShot()
            }

            GettingStartedTip {
                id: editTip
                tipId: "editRatingsTip"

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    top: parent.bottom
                }
                standalone: true
                arrow: GettingStartedTip.TopRight
                text: qsTr("Click here to change which aircraft are shown or hidden based on their ratings")
            }
        },

        // mouse are behind panel to consume clicks
        MouseArea {
            width: 10000 // deliberately huge values here
            height: 10000
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            visible: editRatingsPanel.visible

            onClicked: {
                editRatingsPanel.visible = false
            }
        },

        Rectangle {
            id: editRatingsPanel
            visible: false
            width: parent.width
            y: parent.height - 1
            height: childrenRect.height + (Style.margin * 2)

            border.width: 1
            border.color: "#9f9f9f"

            Column {
                y: Style.margin
                spacing: (Style.margin * 2)

                StyledText {
                    text: qsTr("Aircraft are rated by the community based on four critiera, on a scale from " +
                               "one to five. The ratings are designed to help make an informed guess how "+
                               "complete and functional an aircraft is.")
                    width: editRatingsPanel.width - Style.strutSize * 2
                    wrapMode: Text.WordWrap
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                RatingSlider {
                    label: qsTr("Minimum flight-model (FDM) rating:")
                    ratings: _launcher.browseAircraftModel.ratings
                    ratingIndex: 0
                }

                RatingSlider {
                    label: qsTr("Minimum systems rating")
                    ratings: _launcher.browseAircraftModel.ratings
                    ratingIndex: 1
                }

                RatingSlider {
                    label: qsTr("Minimum cockpit visual rating")
                    ratings: _launcher.browseAircraftModel.ratings
                    ratingIndex: 2
                }

                RatingSlider {
                    label: qsTr("Minimum exterial visual model rating")
                    ratings: _launcher.browseAircraftModel.ratings
                    ratingIndex: 3
                }
            }
        }
    ]

}

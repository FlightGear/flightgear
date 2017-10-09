import QtQuick 2.0
import FGLauncher 1.0

Item {

    property alias aurcradftURI: aircraft.uri


    AircraftInfo
    {
        id: aircraft
    }

    Column {
        Text {
            id: aircraftName
        }

        Image {
            id: preview


            // selector overlay

            // left / right arrows
        }

        Timer {
            id: previewCycleTimer
        }


        Text {
            id: aircraftDescription
        }

        Text {
            id: aircraftAuthors
        }

        // info button

        // version warning!

        // ratings box

        // package size

        // install / download / update button
    } // main layout column
}

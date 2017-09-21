import FlightGear.Launcher 1.0

Section {
    id: timeSettings
    title: "Time & Date"

    Combo {
        id: timeOfDay
        label: qsTr("Time of day")
        description: qsTr("Select the time of day used when the simulator starts, or enter a "
            + "custom date and time.")
        choices: [qsTr("Current time"), qsTr("Dawn"), qsTr("Morning"), qsTr("Noon"),
            qsTr("Afternoon"), qsTr("Dusk"), qsTr("Evening"),
            qsTr("Midnight"), qsTr("Custom time & date")]
        defaultIndex: 0

        readonly property var args: ["", "dawn", "morning", "noon", "afternoon",
            "dusk", "evening", "midnight"]

        readonly property bool isCustom: (selectedIndex == 8)
        readonly property bool isDefault: (selectedIndex == 0)

        function summary()
        {
            if (!timeOfDay.isCustom && !timeOfDay.isDefault) {
                return choices[selectedIndex].toLowerCase() + ";";
            }

            return "";
        }
    }

    DateTime {
        id: customTime
        label: qsTr("Enter custom time & date")
        visible: timeOfDay.isCustom
//        description: "Enter a date and time."
    }

    Checkbox {
        id: customTimeIsGMT
        label: qsTr("Custom time is GMT / UTC")
        visible: timeOfDay.isCustom
    }

    Combo {
        id: season
        label: qsTr("Season")
        description: qsTr("Select if normal (summer) or winter textures are used for the scenery. "
            + "This does not affect other aspects of the simulation at present.")
        keywords: ["season", "scenery", "texture", "winter"]
        choices: [qsTr("Summer (default)"),
            qsTr("Winter")]
        defaultIndex: 0
        readonly property var args: ["summer", "winter"]
    }

    onApply: {
        if (timeOfDay.isCustom) {
            var timeString = Qt.formatDateTime(customTime.value, "yyyy:MM:dd:hh:mm:ss");
            if (customTimeIsGMT.checked) {
                _config.setArg("start-date-gmt", timeString)
            } else {
                _config.setArg("start-date-sys", timeString)
            }
        } else if (timeOfDay.selectedIndex > 0) {
            _config.setArg("timeofday", timeOfDay.args[timeOfDay.selectedIndex])
        }

        _config.setArg("season", season.args[season.selectedIndex])
    }

    summary: timeOfDay.summary()
}

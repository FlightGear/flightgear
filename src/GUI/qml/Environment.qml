import QtQuick 2.4
import FlightGear.Launcher 1.0
import "."

Item {
    Binding {
        target: _launcher
        property: "environmentSummary"
        value: timeOfDay.summary().concat(weatherSettings.summary());
    }

    Flickable {
        contentHeight: sectionColumn.childrenRect.height
        flickableDirection: Flickable.VerticalFlick
        anchors.fill: parent
        anchors.leftMargin: Style.margin
        anchors.rightMargin: Style.margin
        
        Column
        {
            id: sectionColumn
            width: parent.width

            Item {
                // below header margin
                width: parent.width
                height: Style.margin
            }

            Section {
                id: timeOfDaySettings
                title: qsTr("Time & Date")
                settingGroup: "timeDate"

                contents: [

                    SettingsComboBox {
                        id: timeOfDay
                        label: qsTr("Time of day")
                        description: qsTr("Select the time of day used when the simulator starts, or enter a "
                            + "custom date and time.")
                        choices: [qsTr("Current time"), qsTr("Dawn"), qsTr("Morning"), qsTr("Noon"),
                            qsTr("Afternoon"), qsTr("Dusk"), qsTr("Evening"),
                            qsTr("Midnight"), qsTr("Custom time & date")]
                        defaultIndex: 0
                        setting: "time-of-day"
                        readonly property var args: ["", "dawn", "morning", "noon", "afternoon",
                            "dusk", "evening", "midnight"]

                        readonly property bool isCustom: (selectedIndex == 8)
                        readonly property bool isDefault: (selectedIndex == 0)

                        function summary()
                        {
                            if (!timeOfDay.isCustom && !timeOfDay.isDefault) {
                                return [choices[selectedIndex].toLowerCase()];
                            }

                            return [];
                        }
                    },

                    SettingsDateTimePicker
                    {
                        id: customTime
                        label: qsTr("Custom time & date")
                        hidden: !timeOfDay.isCustom
                         description: qsTr("Enter a date and time to begin the flight at. By default this is "
                                           + "in local time for the chosen starting location - use the option "
                                           + "below to request a time in GMT / UTC.")
                         setting: "custom-time"
                    },

                    SettingCheckbox {
                        id: customTimeIsGMT
                        label: qsTr("Custom time is GMT / UTC")
                        visible: timeOfDay.isCustom
                        setting: "custom-time-is-gmt"
                    },

                    SettingsComboBox {
                        id: season
                        label: qsTr("Season")
                        description: qsTr("Select if normal (summer) or winter textures are used for the scenery. "
                            + "This does not affect other aspects of the simulation at present, "
                            + "such as icing or weather simulation");
                        keywords: ["season", "scenery", "texture", "winter"]
                        choices: [qsTr("Summer (default)"), qsTr("Winter")]
                        defaultIndex: 0
                        setting: "winter-textures"
                        readonly property var args: ["summer", "winter"]
                    }
                ]

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

                    if (season.selectedIndex > 0) {
                        _config.setArg("season", season.args[season.selectedIndex])
                    }
                }
            }

            Section {
                id: weatherSettings
                title: qsTr("Weather")
                settingGroup: "weather"

                contents: [
                    SettingCheckbox {
                        id: advancedWeather
                        label: qsTr("Advanced weather modelling")
                        description: qsTr("Detailed weather simulation based on local terrain and "
                                     + "atmospheric simulation. Note that using advanced weather with "
                                     + "real-world weather data (METAR) information may not show exactly "
                                     + "the conditions recorded, and is not recommended for multi-player "
                                     + "flight since the weather simulation is not shared over the network.")
                        setting: "aws-enabled"
                    },

                    SettingCheckbox {
                        id: fetchMetar
                        label: qsTr("Real-world weather")
                        description: qsTr("Download real-world weather from the NOAA servers based on location.")
                        option: "real-weather-fetch"
                        setting: "fetch-metar"
                    },

                    SettingsComboBox {
                        id: weatherScenario
                        enabled: !fetchMetar.checked
                        label: qsTr("Weather scenario")
                        displayRole: "name"
                        choices: _weatherScenarios
                        readonly property bool isCustomMETAR: (selectedIndex == 0);
                        description: _weatherScenarios.descriptionForItem(selectedIndex)
                        defaultIndex: 1
                        setting: "weather-scenario"
                    },

                    SettingLineEdit {
                        id: customMETAR

                        property bool __cachedValid: true

                        function revalidate() {
                            __cachedValid = _launcher.validateMetarString(value);
                        }

                        hidden: !weatherScenario.isCustomMETAR
                        enabled: !fetchMetar.checked
                        label: qsTr("METAR")
                        placeholder: "XXXX 012345Z 28035G50KT 250V300 9999 TSRA SCT022CB BKN030 13/09 Q1005"
                        useFullWidth: true
                        setting: "custom-metar"
                        description: __cachedValid ? qsTr("Enter a custom METAR string, e.g: '%1'").arg(placeholder)
                                                   : qsTr("The entered METAR string doesn't seem to be valid.")

                        onValueChanged: {
                            validateTimeout.restart()
                        }
                    }
                ]

                Timer {
                    id: validateTimeout
                    interval: 200
                    onTriggered: customMETAR.revalidate();
                }

                onApply: {
                    // important that we always set a value here, to override
                    // the auto-saved value in FlightGear. Otherwise we get
                    // confusing behaviour when the user toggles the setting
                    // inside the sim
                    _config.setProperty("/nasal/local_weather/enabled", advancedWeather.checked);

                    // Thorsten R advises that these are also needed for AW to startup
                    // correctly.
                    if (advancedWeather.checked) {
                        _config.setProperty("/sim/gui/dialogs/metar/mode/global-weather", !advancedWeather.checked);
                        _config.setProperty("/sim/gui/dialogs/metar/mode/local-weather", advancedWeather.checked);
                        _config.setProperty("/sim/gui/dialogs/metar/mode/manual-weather", advancedWeather.checked);
                    }

                    var index = weatherScenario.selectedIndex;

                    // set description from the weather scenarios, so Local-weather
                    // can run the appropriate simulation
                    if (fetchMetar.checked) {
                        if (advancedWeather.checked) {
                            _config.setProperty("/local-weather/tmp/tile-management", "METAR");
                            _config.setProperty("/local-weather/tmp/tile-type", "live");
                        }
                    } else {
                        if (weatherScenario.isCustomMETAR) {
                            _config.setArg("metar", customMETAR.value)
                        } else {
                            _config.setArg("metar", _weatherScenarios.metarForItem(index))
                        }

                        // either way, set the scenario name since Local-Weather keys off
                        // this to know what to do with the scenario + metar data
                        _config.setProperty("/environment/weather-scenario",
                                            _weatherScenarios.nameForItem(index))

                        if (advancedWeather.checked) {
                            // set AW tile options
                            var s = _weatherScenarios.localWeatherData(index);
                            if (s.length === 0) {
                               // local weather will use METAR
                            } else {
                                _config.setProperty("/local-weather/tmp/tile-management", s[0]);
                                _config.setProperty("/local-weather/tmp/tile-type", s[1]);
                            }
                        } // of using Advanced (local) weather
                    } // of not using real-wxr
                }

                function summary()
                {
                    var result = [];
                    if (advancedWeather.checked) result.push(qsTr("advanced weather"));
                    if (fetchMetar.checked) result.push(qsTr("real-world weather"));
                    return result;
                }
            }
        } // of Column
    } // of Flickable
}

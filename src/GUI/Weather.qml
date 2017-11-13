import FlightGear.Launcher 1.0
import QtQml 2.2

Section {
    id: weatherSettings
    title: "Weather"

    Checkbox {
        id: advancedWeather
        label: "Advanced weather modelling"
        description: "Detailed weather simulation based on local terrain and "
                     + "atmospheric simulation. Note that using advanced weather with "
                     + "real-world weather data (METAR) information may not show exactly "
                     + "the conditions recorded, and is not recommended for multi-player "
                     + "flight since the weather simulation is not shared over the network."
    }

    Checkbox {
        id: fetchMetar
        label: "Real-world weather"
        description: "Download real-world weather from the NOAA servers based on location."
        option: "real-weather-fetch"
    }

    Combo {
        id: weatherScenario
        enabled: !fetchMetar.checked
        label: "Weather scenario"
        model: _weatherScenarios
        readonly property bool isCustomMETAR: (selectedIndex == 0);
        description: _weatherScenarios.descriptionForItem(selectedIndex)
        defaultIndex: 1
    }

    LineEdit {
        id: customMETAR

        property bool __cachedValid: true

        function revalidate() {
            __cachedValid = _launcher.validateMetarString(value);
        }

        visible: weatherScenario.isCustomMETAR
        enabled: !fetchMetar.checked
        label: "METAR"
        placeholder: "XXXX 012345Z 28035G50KT 250V300 9999 TSRA SCT022CB BKN030 13/09 Q1005"
        description: __cachedValid ? "Enter a custom METAR string"
                                   : "The entered METAR string doesn't seem to be valid."

        onValueChanged: {
            validateTimeout.restart()
        }
    }

    Timer {
        id: validateTimeout
        interval: 200
        onTriggered: customMETAR.revalidate();
    }

    onApply: {
        if (advancedWeather.checked) {
            // set description from the weather scenarios, so Local-weather
            // can run the appropriate simulation
            _config.setProperty("/nasal/local_weather/enabled", 1);
        }

        var index = weatherScenario.selectedIndex;

        if (!fetchMetar.checked) {
            if (weatherScenario.isCustomMETAR) {
                _config.setArg("metar", customMETAR.value)
            } else {
                _config.setArg("metar", _weatherScenarios.metarForItem(index))
            }

            // either way, set the scenario name since Local-Weather keys off
            // this to know what to do with the scenario + metar data
            _config.setProperty("/environment/weather-scenario",
                                _weatherScenarios.nameForItem(index))

        }
    }

    summary: (advancedWeather.checked ? "advanced weather;" : "")
             + (fetchMetar.checked ? "real-world weather;" : "")
}

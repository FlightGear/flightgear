import FlightGear.Launcher 1.0
import QtQml 2.2

Section {
    id: weatherSettings
    title: "Weather"

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
        if (!fetchMetar.checked) {
            if (weatherScenario.isCustomMETAR) {
                _config.setArg("metar", customMETAR.value)
            } else {
                _config.setArg("metar", _weatherScenarios.metarForItem(weatherScenario.selectedIndex))
            }
        }
    }

    summary: fetchMetar.checked ? "real-world weather;" : ""
}

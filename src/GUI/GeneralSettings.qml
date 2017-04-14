import FlightGear.Launcher 1.0

Section {
    id: generalSettings
    title: "General"

    Checkbox {
        id: startPaused
        label: "Start paused"
        description: "Automatically pause the simulator when launching. This is useful "
            + "when starting in the air."
        keywords: ["pause", "freeze"]
    }

    Checkbox {
        id: autoCoordination
        label: "Enable auto-coordination"
        description: "When flying with the mouse, or a joystick lacking a rudder axis, "
            + "it's difficult to manually coordinate aileron and rudder movements during "
            + "turn. This option automatically commands the rudder to maintain zero "
            + "slip angle when banking";
        advanced: true
        keywords: ["input", "mouse", "control", "rudder"]
        option: "auto-coordination"
    }

    Checkbox {
        id: showConsoleWin
        label: "Show debugging console"
        description: "Open a console window showing debug output from the application."
        advanced: true
        visible: _osName == "win"
        keywords: ["console"]
    }

    onApply: {
        if (startPaused.checked) {
            _config.setArg("enable-freeze")
        }

        if (showConsoleWin.checked) {
            _config.setArg("console")
        }
    }
}

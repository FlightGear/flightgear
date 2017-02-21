import FlightGear.Launcher 1.0

Section {
    id: viewSettings
    title: "View & Window"

    Checkbox {
        id: fullscreen
        label: "Start full-screen"
        description: "Start the simulator in full-screen mode"
        keywords: ["window", "full", "screen"]
        option: "fullscreen"
    }

    Combo {
        id: windowSize
        enabled: !fullscreen.checked
        label: "Window size"
        description: "Select the initial size of the window. (This has no effct in full-screen mode)"
        advanced: true
        choices: ["640x480", "800x600", "1024x768", "1920x1080", "2560x1600" ]
        defaultIndex: 2
        readonly property bool isDefault: selectedIndex == defaultIndex
    }

    onApply: {
        if (!windowSize.isDefault) {
            _config.setArg("geometry", windowSize.choices[windowSize.selectedIndex]);
        }
    }

    summary: fullscreen.checked ? "full-screen;" : ""
}

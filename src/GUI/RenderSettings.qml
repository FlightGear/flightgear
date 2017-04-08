import FlightGear.Launcher 1.0

Section {
    id: renderingSetttings
    title: "Rendering"

    readonly property bool rembrandt: (renderer.selectedIndex == 2)
    readonly property bool alsEnabled: (renderer.selectedIndex == 1)
    readonly property bool msaaEnabled: !rembrandt && (msaa.selectedIndex > 0)

    Combo {
        id: renderer
        label: "Renderer"
        choices: ["Default", "Atmospheric Light-Scattering", "Rembrandt"]
        description: descriptions[selectedIndex]
        defaultIndex: 0

        readonly property var descriptions: [
            "The default renderer provides standard visuals with maximum compatability",
            "The ALS renderer uses a sophisticated physical atmospheric model and several " +
            "other effects to give realistic rendering of large distances.",
            "Rembrandt is a configurable multi-pass renderer which supports shadow-maps, cinematic " +
            "effects and more. However, not all aircraft appear correctly and performance will " +
            "depend greatly on your system hardware."
        ]

        keywords: ["als", "rembrandt", "render", "shadow"]
    }

    Combo {
        id: msaa
        label: "Anti-aliasing"
        description: "Anti-aliasing improves the appearance of high-contrast edges and lines." +
        "This is especially noticeable on sloping or diagonal egdes. " +
        "Higher settings can reduce performance."
        keywords: ["msaa", "anti", "aliasing", "multi", "sample"]
        choices: ["Off", "2x", "4x"]
        enabled: !rembrandt
        property var data: [0, 2, 4];
        defaultIndex: 0
    }

    onApply: {
        _config.setProperty("/sim/rendering/multi-sample-buffers", msaaEnabled)
        _config.setProperty("/sim/rendering/multi-samples", msaa.data[msaa.selectedIndex])

        _config.setEnableDisableOption("rembrandt",  rembrandt);

        if (alsEnabled) {
            _config.setProperty("/sim/rendering/shaders/skydome", true);
        }
    }

    summary: (rembrandt ? "Rembrandt;" : (alsEnabled ? "ALS;" : ""))
             + (msaaEnabled ? "anti-aliasing;" : "")
}

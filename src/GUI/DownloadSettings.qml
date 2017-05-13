import FlightGear.Launcher 1.0

Section {
    id: downloadSettings
    title: "Downloads"

    Checkbox {
        id: terrasync
        label: "Download scenery automatically"
        description: "FlightGear can automatically download scenery as needed, and check for updates to "
            + "the scenery. If you disable this option, you will need to download & install scenery "
            + "using an alternative method."
        keywords: ["terrasync", "download", "scenery"]
        option: "terrasync"
    }

    // file path chooser for downloads directory

    PathChooser {
        id: downloadDir
        label: "Download location"
        description: "FlightGear stores downloaded files (scenery and aircraft) in this location. "
         + "Depending on your settings, it may grow to a considerable size (many gigabytes). "
         + "If you change the download location, files will need to be downloaded again."
        advanced: true
        chooseDirectory: true
        defaultPath: _config.defaultDownloadDir
        option: "download-dir"
        dialogPrompt: "Choose a location to store download files."

        keywords: ["download", "storage", "disk"]
    }

    onApply: {
    }

    summary: terrasync.checked ? "scenery downloads;" : ""
}

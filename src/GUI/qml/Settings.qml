import QtQuick 2.4
import QtQuick.Controls 2.2

import FlightGear.Launcher 1.0
import FlightGear 1.0

Item {
    id: settings

    Rectangle {
        visible: _launcher.isSearchActive
        anchors.fill: parent
        color: Style.backgroundColor
    }

    Rectangle {
        // search 'dimming' rectangle
        visible: _launcher.isSearchActive
        anchors.fill: parent
        color: "#7f7f7f"
    }

    Binding {
        target: _launcher
        property: "settingsSummary"
        value: generalSettings.summary().concat(mpSettings.summary(),
                                                downloadSettings.summary(),
                                                windowSettings.summary(),
                                                renderSection.summary());
    }

    Flickable
    {
        id: settingsFlick
        contentHeight: sectionColumn.childrenRect.height
        flickableDirection: Flickable.VerticalFlick
        height: parent.height
        width: parent.width - Style.margin * 2
        x: Style.margin
        
        ScrollBar.vertical: ScrollBar {}

        Column
        {
            id: sectionColumn
            width: parent.width

            GettingStartedScope.controller: tips.controller

            Item {
                // top margin
                width: parent.width
                height: Style.margin
            }

            Item {
                id: header
                width: parent.width
                height: headerText.height

                StyledText {
                    id: headerText
                    text: qsTr("Settings")
                    font.pixelSize: Style.headingFontPixelSize
                    anchors.left: parent.left
                    anchors.leftMargin: Style.inset
                }

                SearchButton {
                    id: search
                    width: Style.strutSize * 4
                    anchors.right: parent.right
                    anchors.rightMargin: Style.margin
                    anchors.verticalCenter: parent.verticalCenter
                    autoSubmitTimeout: 250
                    onSearch: {
                        _launcher.settingsSearchTerm = term
                    }

                    GettingStartedTip {
                        tipId: "searchSettingsTip"
                        nextTip: "expandSectionTip"

                        anchors {
                            horizontalCenter: parent.horizontalCenter
                            top: parent.bottom
                        }
                        arrow: GettingStartedTip.TopRight
                        text: qsTr("Enter text here to search for a setting")
                    }
                }
            }

            Item {
                // below header margin
                width: parent.width
                height: Style.margin
            }

            Section {
                id: generalSettings
                title: qsTr("General")
                settingGroup: "general"

                function summary()
                {
                    var result = [];
                    if (startPaused.checked) result.push(qsTr("paused"));
                    if (!showConsoleWin.hidden && showConsoleWin.checked) result.push(qsTr("console"));
                    return result;
                }

                contents: [
                    SettingCheckbox {
                        id: startPaused
                        label: qsTr("Start paused")
                        description: qsTr("Automatically pause the simulator when launching. This is useful "
                            + "when starting in the air.")
                        keywords: ["pause", "freeze"]
                        option: "freeze"
                        setting: "start-paused"
                    },

                    SettingCheckbox {
                        id: autoCoordination
                        label: qsTr("Enable auto-coordination")
                        description: qsTr("When flying with the mouse, or a joystick lacking a rudder axis, "
                            + "it's difficult to manually coordinate aileron and rudder movements during "
                            + "turn. This option automatically commands the rudder to maintain zero "
                            + "slip angle when banking");
                        advanced: true
                        keywords: ["input", "mouse", "control", "rudder"]
                        option: "auto-coordination"
                        setting: "auto-coordination"
                    },

                    SettingCheckbox {
                        id: showConsoleWin
                        label: qsTr("Show debugging console")
                        description: qsTr("Open a console window showing debug output from the application.")
                        advanced: true
                        hidden: _osName !== "win"
                        keywords: ["console", "terminal", "log", "debug"]
                        setting: "console"
                    },

                    SettingCheckbox {
                        id: enableCrashReporting
                        label: qsTr("Enable crash & error reporting")
                        description: qsTr("Send crash and error reports to the development team for analysis.")
                        defaultValue: true
                        keywords: ["crash", "report", "telemetry", "sentry", "segfault"]
                        setting: "enable-sentry"
                    },

                    SettingCheckbox {
                        id: developerMode
                        advanced: true
                        label: qsTr("Enable developer mode")
                        description: qsTr("Enable simulator & aircraft development features, such as increased error messages in log files.")
                        defaultValue: false
                        keywords: ["develop", "developer"]
                        setting: "develop"
                    },

                    SettingCheckbox {
                        id: restartLauncher
                        label: qsTr("Re-open FlightGear on exit")
                        description: qsTr("Re-open this window when exiting FlightGear, to start another flight immediately.")
                        keywords: ["restart", "reopen"]
                        setting: "restart-launcher"
                    }
                ]

                onApply: {
                    if (!showConsoleWin.hidden && showConsoleWin.checked) _config.setArg("console");
                    _config.setEnableDisableOption("sentry",  enableCrashReporting.checked);
                    if (developerMode.checked) _config.setArg("developer");
                    if (restartLauncher.checked) _config.setArg("restart-launcher");
                }
            }

            Section {
                id: mpSettings
                title: qsTr("Multi-player")
                settingGroup: "mp"

                readonly property int defaultMPPort: 5000

                function summary()
                {
                    var result = [];
                    if (enableMP.checked) result.push(qsTr("multi-player"));
                    return result;
                }

                contents: [
                    SettingCheckbox {
                        id: enableMP
                        label: qsTr("Connect to the multi-player network")
                        description: qsTr("FlightGear supporters maintain a network of servers to enable global multi-user "
                                          + "flight. This requires a moderately fast Internet connection to be usable. Your aircraft "
                                          + "will be visible to other users online, and you will see their aircraft.")
                        keywords: ["network", "mp", "multiplay","online"]
                        setting: "enabled"

                        onValueChanged: {
                            if (value) {
                                _launcher.queryMPServers();
                            }
                        }
                    },

                    SettingLineEdit {
                        id: callSign
                        enabled: enableMP.checked
                        label: qsTr("Callsign")
                        description: qsTr("Enter a callsign you will use online. This is visible to all users and is " +
                                          "how ATC services and other pilots will refer to you. " +
                                          "(Maximum of seven characters permitted)")
                        placeholder: "D-FGFS"
                        suggestedWidthString: "MMMMMMMMMMM"
                        keywords: ["callsign", "handle", "name"]

                        // between one and seven alphanumerics, underscores and/or hypens
                        // spaces not permitted
                        validation: RegExpValidator { regExp: /[\w-]{1,7}/ }
                        setting: "callsign"
                    },

                    SettingsComboBox {
                        id: mpServer
                        label: qsTr("Server")
                        enabled: enableMP.checked
                        description: qsTr("Select a server close to you for better responsiveness and reduced lag when flying online.")
                        model: _launcher.mpServersModel

                        readonly property bool currentIsCustom: (_launcher.mpServersModel.currentServer === "__custom__")
                        readonly property bool currentIsNoServers: (_launcher.mpServersModel.currentServer === "__noservers__")
                        property string __savedServer;

                        keywords: ["server", "hostname"]
                        setting: "server"

                        // following three elements are to deal with the irregular way we save the
                        // MP server state, because the index is unstable, and the time to query online servers
                        // is determined by the MPServersModel. So we let that code do the state
                        // restoration, and emit a signal we handler here, when it wants to update the UI.
                        function saveState()
                        {
                            // these values match the code in MPServersModel.cpp, sorry for that
                            // nastyness
                            _config.setValueForKey("mpSettings", "mp-server", _launcher.mpServersModel.currentServer);
                        }

                        function restoreState()
                        {
                            // no-op, this is triggered by MPServersModel::restoreMPServerSelection
                        }

                        // can't use a Binding here, since we need a bidrectional link
                        onSelectedIndexChanged: _launcher.mpServersModel.currentIndex = selectedIndex

                        Connections
                        {
                            target: _launcher.mpServersModel
                            onCurrentIndexChanged: mpServer.selectedIndex = _launcher.mpServersModel.currentIndex
                        }
                    },

                    SettingLineEdit {
                        id: mpCustomServer
                        enabled: enableMP.checked
                        label: qsTr("Custom server")
                        hidden: !mpServer.currentIsCustom
                        description: qsTr("Enter a server hostname or IP address, and optionally a port number. (Default port is 5000) For example 'localhost:5001'")
                        placeholder: "localhost:5001"
                        suggestedWidthString: "MMMMMMMMMMMMMMMMMMMMMMMMMMMMM"
                        setting: "custom-server"
                    }
                ]

                onApply: {
                    if (enableMP.checked) {
                        if (mpServer.currentIsNoServers) {
                            console.warn("MP enabled but no valid server selected, skipping");
                        } else if (mpServer.currentIsCustom) {
                            var pieces = mpCustomServer.value.split(':')
                            var customPort = defaultMPPort;
                            if (pieces.length > 1) {
                                customPort = pieces[1];
                            }

                            if (pieces[0].length > 0) {
                                _config.setProperty("/sim/multiplay/txhost", pieces[0]);
                                _config.setProperty("/sim/multiplay/txport", customPort);
                            } else {
                                console.warn("Custom MP server selected but no hostname defined");
                            }
                        } else {
                            var host =  _launcher.mpServersModel.currentServer;
                            if (host.length > 0) {
                                _config.setProperty("/sim/multiplay/txhost", host);
                            }

                            var port = _launcher.mpServersModel.currentPort;
                            if (port === 0) {
                                console.info("Using default MP port");
                                port = defaultMPPort;
                            }

                            _config.setProperty("/sim/multiplay/txport", port);
                        }

                        if (callSign.value.length > 0) {
                            _config.setArg("callsign", callSign.value)
                        }
                    }
                }
            }

            Section {
                id: downloadSettings
                title: qsTr("Downloads")
                width: parent.width
                settingGroup: "downloads"

                function summary()
                {
                    var result = [];
                    if (terrasync.checked) result.push(qsTr("scenery downloads"));
                    return result;
                }

                contents: [
                    SettingCheckbox {
                        id: terrasync
                        label: qsTr("Download scenery automatically")
                        description: qsTr("FlightGear can automatically download scenery as needed, and check for updates to "
                            + "the scenery. If you disable this option, you will need to download & install scenery "
                            + "using an alternative method.")
                        keywords: ["terrasync", "download", "scenery"]
                        option: "terrasync"
                        setting: "terrasync"

                        // ensure we pass --disable-terrasync when unchecked, because
                        // terrasync state is autosaved and hence can stick inside the sim
                        setIfDefault: true
                    },

                    SettingsComboBox {
                        id: terraSyncDnsMode
                        label: qsTr("TerraSync DNS Server")
                        description: qsTr("Select the DNS server used for TerraSync lookup.")
                        advanced: true
                        choices: [qsTr("Default"), qsTr("Google"), qsTr("Custom DNS Server") ]
                        defaultIndex: 0
                        readonly property bool isGoogle: selectedIndex == 1
                        readonly property bool isCustom: selectedIndex == 2
                        keywords: ["terrasync", "dns", "network", "download", "scenery"]
                        setting: "terrasync-dns-mode"
                    },

                    SettingLineEdit {
                        id: terraSyncCustomDns
                        hidden: !terraSyncDnsMode.isCustom
                        label: qsTr("Custom DNS Server")
                        advanced: true
                        description: qsTr("Enter a custom DNS server IP address, for example '8.8.8.8'")
                       // validation: RegExpValidator { regExp: /[\d]{1,7}[\s]*x[\s]*[\d]{1,7}$/ }
                        suggestedWidthString: "255.255.255.255"
                        setting: "terrasync-dns-custom-server"
                    },

                    SettingPathChooser {
                        id: downloadDir
                        label: qsTr("Download location")
                        description: qsTr("FlightGear stores downloaded files (scenery and aircraft) in this location. "
                                     + "Depending on your settings, it may grow to a considerable size (many gigabytes). "
                                     + "If you change the download location, files will need to be downloaded again. "
                                     + "When changing this setting, FlightGear will restart to use the new location correctly.")
                        advanced: true
                        chooseDirectory: true
                        defaultPath: _config.defaultDownloadDir
                        dialogPrompt: qsTr("Choose a location to store download files.")
                        setting: "download-dir"

                        // we disable this UI if the user passes --download-dir when starting the
                        // launcher, otherwise we coudld end up in a complete mess
                        enabled: _config.enableDownloadDirUI

                        keywords: ["download", "storage", "disk"]

                        onPathChanged: {
                            // lots of special work needs to be done immediately that this
                            // value is changed.
                            _launcher.downloadDirChanged(path);
                        }

                        function saveState()
                        {
                            // ensure this is a no-op, we write the value explicity
                            // in LauncherController::downloadDirChanged
                        }
                    }
                ]

                onApply: {
                    // note we do /not/ apply the downloadDir setting here, since it's
                    // only permitted to occurr once, and we set it via other means;
                    // on startup via runLauncherDialog, and if it changes, via
                    // LauncherMainWindow::downloadDirChanged

                    if (terraSyncDnsMode.isCustom) {
                        _config.setProperty("/sim/terrasync/dns-server", terraSyncCustomDns.value);
                    } else if (terraSyncDnsMode.isGoogle) {
                        // DNSClient.cxx handles this as a special value
                        _config.setProperty("/sim/terrasync/dns-server", "google");
                    }
                }

                summary: terrasync.checked ? "scenery downloads;" : ""
            }

            Section {
                id: windowSettings
                title: qsTr("View & Window")
                width: parent.width
                settingGroup: "window"

                function summary()
                {
                    var result = [];
                    if (fullscreen.checked) result.push(qsTr("full-screen"));
                    return result;
                }

                contents: [
                    SettingCheckbox {
                        id: fullscreen
                        label: qsTr("Start full-screen")
                        description: qsTr("Start the simulator in full-screen mode.");
                        setting: "fullscreen"
                        option: "fullscreen"
                    },

                    SettingsComboBox {
                        id: windowSize
                        enabled: !fullscreen.checked
                        label: qsTr("Window size")
                        description: qsTr("Select the initial size of the window (this has no effect in full-screen mode).")
                        advanced: true
                        choices: ["640x480", "800x600", "1024x768", "1920x1080", "2560x1600", qsTr("Custom Size") ]
                        defaultIndex: 2
                        readonly property bool isDefault: selectedIndex == defaultIndex
                        readonly property bool isCustom: selectedIndex == 5
                        keywords: ["window", "geometry", "size", "resolution"]
                        setting: "window-size"
                    },

                    SettingLineEdit {
                        id: customWindowSize
                        hidden: !windowSize.isCustom
                        label: qsTr("Custom size")
                        advanced: true
                        description: qsTr("Enter a custom window size in the form 'WWWWW x HHHHH', for example '1280 x 900'")
                        validation: RegExpValidator { regExp: /[\d]{1,7}[\s]*x[\s]*[\d]{1,7}$/ }
                        setting: "custom-size"
                        suggestedWidthString: "0000000 x 0000000"

                    }

                ]

                onApply: {
                    if (windowSize.isCustom) {
                        _config.setArg("geometry", customWindowSize.value);
                    } else if (!windowSize.isDefault) {
                        _config.setArg("geometry", windowSize.choices[windowSize.selectedIndex]);
                    }
                }
            }

            Section {
                id: renderSection
                title: qsTr("Rendering")
                width: parent.width
                settingGroup: "render"

                readonly property bool msaaEnabled: (msaa.selectedIndex > 0)
                readonly property bool ws30Enabled: (sceneryversion.selectedIndex > 0)

                function summary()
                {
                    var result = [];
                    if (msaaEnabled) result.push(qsTr("anti-aliasing"));
                    if (ws30Enabled) result.push(qsTr("scenery v3.0"));
                    return result;
                }

                contents: [         
                    SettingsComboBox {
                        id: msaa
                        label: qsTr("Anti-aliasing")
                        setting: "aa"
                        description: qsTr("Anti-aliasing improves the appearance of high-contrast edges and lines. " +
                            "This is especially noticeable on sloping or diagonal edges. " +
                            "Higher settings can reduce performance.")
                        keywords: ["msaa", "anti", "aliasing", "multi", "sample"]
                        choices: [qsTr("Off"), "2x", "4x", "8x"]
                        property var data: [0, 2, 4, 8];
                        defaultIndex: 0
                    },

                    SettingCheckbox {
                        id: compressTextures
                        setting: "texture-compression"
                        keywords: ["texture", "compression", "memory", "dxt", "cache"]
                        // no option, since we need to set a property
                        advanced: true

                        label: qsTr("Cache graphics for faster loading")
                        description: qsTr("By converting images used in rendering to an optimised format " +
                                          "loading times and memory use can be improved. This will consume " +
                                          "some disk space and take initial time while images are converted, " +
                                          "but subsequent loads will be faster, and use less memory.")
                    },

                    SettingsComboBox {
                        id: sceneryversion
                        setting: "scenery-version"
                        advanced: true

                        label: qsTr("Scenery version")
                        description: qsTr("Scenery version 2.0 is available for the whole world and works well for " +
                            "lower powered systems. Scenery version 3.0 is currently under development, but is only " +
                            "available for limited areas. It is more detailed and performs better on high powered systems.")
                        keywords: ["scenery", "ws2.0", "ws20", "ws3.0", "ws30"]
                        choices: ["V2.0", "V3.0"]
                        property var data: [0, 1];
                        defaultIndex: 0
                    }
                ]

                onApply: {
                    if (msaaEnabled) {
                        _config.setProperty("/sim/rendering/multi-sample-buffers", 1)
                        _config.setProperty("/sim/rendering/multi-samples", msaa.data[msaa.selectedIndex])
                    }

                    if (ws30Enabled) {
                        _config.setProperty("/scenery/use-vpb", true)
                        _config.setProperty("/sim/rendering/scenery-path-suffix[98]/enabled", true)
                    } else {
                        _config.setProperty("/scenery/use-vpb", false)
                        _config.setProperty("/sim/rendering/scenery-path-suffix[98]/enabled", false)
                    }

                    _config.setProperty("/sim/rendering/texture-cache/cache-enabled", compressTextures.value);
                }
            }

            Section {
                id: extraArgsSection
                title: qsTr("Additional Settings")
                settingGroup: "extraArgs"
                width: parent.width
                contents: [
                    SettingExtraArguments {
                        id: extraArgs
                        keywords: ["property", "extra", "command", "argument"]
                        setting: "extra-args"
                    }
                ]
            }
        } // of Column

        GettingStartedTipLayer {
            id: tips
            anchors.fill: parent
            scopeId: "settings"
        }
    } // of Flickable
}

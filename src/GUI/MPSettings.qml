import FlightGear.Launcher 1.0
import QtQml 2.0

Section {
    // note this id is used, hard-coded, in MPServersModel
    id: mpSettings
    title: "Multi-player"

    Checkbox {
        id: enableMP
        label: "Connect to the multi-player network"
        description: "FlightGear supporters maintain a network of servers to enable global multi-user "
            + "flight. This requires a moderately fast Internet connection to be usable. Your aircraft "
            + "will be visible to other users online, and you will see their aircraft."
        keywords: ["network", "mp", "multiplay","online"]
    }

    LineEdit {
        id: callSign
        enabled: enableMP.checked
        label: "Callsign"
        description: "Enter a callsign you will use online. This is visible to all users and is " +
                     "how ATC services and other pilots will refer to you. " +
                     "(Maximum of seven characters permitted)"
        placeholder: "D-FGFS"
        keywords: ["callsign", "handle", "name"]

        // between one and seven alphanumerics, underscores and or hypens
        // spaces not permitted
        validation: "[\\w-]{1,7}"
    }

    Combo {
        id: mpServer
        label: "Server"
        enabled: enableMP.checked
        description: "Select a server close to you for better responsiveness and reduced lag when flying online."
        model: _mpServers

        readonly property bool currentIsCustom: (model.serverForIndex(selectedIndex) == "__custom__")

        keywords: ["server", "hostname"]
    }

    Connections
    {
        target: _mpServers
        onRestoreIndex: {
            mpServer.selectedIndex = index
        }
    }

    LineEdit {
        id: mpCustomServer
        enabled: enableMP.checked
        label: "Custom server"
        visible: mpServer.currentIsCustom
        description: "Enter a server hostname or IP address, and a port number. For example 'localhost:5001'"
        placeholder: "localhost:5001"
    }

    readonly property int defaultMPPort: 5000

    onApply: {
        if (enableMP.checked) {
            if (mpServer.currentIsCustom) {
                var pieces = mpCustomServer.value.split(':')
                var port = defaultMPPort;
                if (pieces.length > 1) {
                    port = pieces[1];
                }

                _config.setProperty("/sim/multiplay/txhost", pieces[0]);
                _config.setProperty("/sim/multiplay/txport", port);
            } else {
                var sel = mpServer.selectedIndex
                _config.setProperty("/sim/multiplay/txhost", _mpServers.serverForIndex(sel));
                var port = _mpServers.portForIndex(sel);
                if (port == 0) {
                    port = defaultMPPort;
                }

                _config.setProperty("/sim/multiplay/txport", port);
            }

            if (callSign.value.length > 0) {
                _config.setArg("callsign", callSign.value)
            }
        }
    }

    onRestore:  {
        // nothing to do, restoration is done by the C++ code
        // in MPServersModel::restoreMPServerSelection
    }

    onSave: {
        saveSetting("mp-server", _mpServers.serverForIndex(mpServer.selectedIndex));
    }

    summary: enableMP.checked ? "multi-player;" : ""
}

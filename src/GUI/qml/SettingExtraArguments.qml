import QtQuick 2.4
import FlightGear.Launcher 1.0
import FlightGear 1.0

SettingControl {
    id: root
    implicitHeight: col.height + Style.margin * 2
    readonly property string placeholder: "--option=value --prop:/sim/name=value"
    property alias value: edit.text
    option: "xxx" // non-empty value so apply() is called
    property string defaultValue: "" // needed to type save/restore logic to string

    Column
    {
        id: col
        y: Style.margin
        width: parent.width
        spacing: Style.margin

        SettingDescription {
            id: description
            enabled: root.enabled
            text: qsTr("Enter additional command-line arguments if any are required. " +
                       "See <a href=\"http://flightgear.sourceforge.net/getstart-en/getstart-enpa2.html#x5-450004.5\">documentation</a> " +
                       "for possible arguments. " +
                       "<br>" +
                       "<b>Warning:</b> values entered here always override other settings; see the " +
                       "<a href=\"#view-command-line\">final set of arguments</a> that will be used."
                       );

            onLinkActivated: {
                if (link == "#view-command-line") {
                    _launcher.viewCommandLine();
                } else {
                    Qt.openUrlExternally(link)
                }
            }

            width: parent.width
        }

        SettingDescription {
            id: warningText
            enabled: root.enabled
            visible: tokenizer.haveUnsupportedArgs
            width: parent.width
            text: qsTr("<b>Warning:</b> specifying <tt>fg-root</tt>, <tt>fg-aircraft</tt>, <tt>fg-scenery</tt> or <tt>fg-home</tt> " +
                       "using this section is not recommended, and may cause problem or prevent the simulator from running. " +
                       "Please use the add-ons page to setup scenery and aircrft directories, and the 'Select data files location' " +
                       "menu item to change the root data directory.");
        }

        SettingDescription {
            id: positionalArgsText
            enabled: root.enabled
            visible: tokenizer.havePositionalArgs
            width: parent.width
            text: qsTr("<b>Note:</b> you have entered arguments relating to the startup location below. " +
                       "To prevent problems caused by conflicting settings, the values entered on the location " +
                       "page (for example, airport or altitude) will be ignored.");
        }

        SettingDescription {
            id: aircraftArgsText
            enabled: root.enabled
            visible: tokenizer.haveAircraftArgs
            width: parent.width
            text: qsTr("<b>Note:</b> you have entered arguments relating to the selected aircraft. " +
                       "To prevent problems caused by conflicting settings, the aircraft page will be ignored.");
        }

        PlainTextEditBox
        {
            id: edit
            width: parent.width
            enabled: root.enabled
            placeholder: root.placeholder
        }
    }

    function apply()
    {
        var tokens = tokenizer.tokens;
        for (var i = 0; i < tokens.length; i++) {
            var tk = tokens[i];
            if (tk.arg.substring(0, 5) === "prop:") {
                _config.setProperty(tk.arg.substring(5), tk.value, LaunchConfig.ExtraArgs);
            } else {
                _config.setArg(tk.arg, tk.value, LaunchConfig.ExtraArgs);
            }
        }
    }

    Binding {
        target: _location
        property: "skipFromArgs"
        value: tokenizer.havePositionalArgs
    }

    Binding {
        target: _launcher
        property: "skipAircraftFromArgs"
        value: tokenizer.haveAircraftArgs
    }

    ArgumentTokenizer {
        id: tokenizer
        argString: edit.text
    }
}

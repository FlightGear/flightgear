import QtQuick 2.4
import QtQml 2.4

import FlightGear 1.0

Item {
    id: root

    property GettingStartedController controller: ctl

    property alias scopeId: ctl.scopeId
    property alias active: ctl.active

    function showOneShot(tip) {
        ctl.showOneShotTip(tip);
    }

    GettingStartedController {
        id: ctl
        visualArea: root
    }

    // ensure active-ness is updated, if the 'reset all tips' function is used
    Connections {
        target: _launcher
        onDidResetGettingStartedTips: ctl.tipsWereReset();
    }

    // use a Loader to handle tip display, so we're not creating visual items
    // for the tip, in the common case that no tip is displayed.
    Loader {
        id: load
        source: (ctl.active && ctl.tipPositionValid) ? "qrc:///qml/GettingStartedTipDisplay.qml" : ""
        x: ctl.tipPositionInVisualArea.x
        y: ctl.tipPositionInVisualArea.y
    }

    // pass the controller into our loaded item
    Binding {
        target: load.item
        property: "controller"
        value: ctl
    }
}

import QtQuick 2.7
import QtQuick.Controls 2.0 as QQC2

ListView
{
    Keys.onUpPressed: scrollBar.decrease()
    Keys.onDownPressed: scrollBar.increase()

    QQC2.ScrollBar.vertical: QQC2.ScrollBar {
        id: scrollBar
    }
}

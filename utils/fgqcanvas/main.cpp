#include "temporarywidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("FGCanvas");
    a.setOrganizationDomain("flightgear.org");
    a.setOrganizationName("FlightGear");

    TemporaryWidget w;
    w.show();

    return a.exec();
}

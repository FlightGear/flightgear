#include "temporarywidget.h"

#include <QApplication>
#include <QNetworkAccessManager>

#include "fgqcanvasfontcache.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("FGCanvas");
    a.setOrganizationDomain("flightgear.org");
    a.setOrganizationName("FlightGear");

    QNetworkAccessManager* downloader = new QNetworkAccessManager;

    FGQCanvasFontCache::initialise(downloader);

    TemporaryWidget w;
    w.show();

    return a.exec();
}

#include "temporarywidget.h"

#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QStandardPaths>

#include "fgqcanvasfontcache.h"
#include "fgqcanvasimageloader.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("FGCanvas");
    a.setOrganizationDomain("flightgear.org");
    a.setOrganizationName("FlightGear");

    QNetworkAccessManager* downloader = new QNetworkAccessManager;

    QNetworkDiskCache* cache = new QNetworkDiskCache;
    cache->setCacheDirectory(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    downloader->setCache(cache); // takes ownership

    FGQCanvasFontCache::initialise(downloader);
    FGQCanvasImageLoader::initialise(downloader);

    TemporaryWidget w;
    w.show();

    int result = a.exec();
    delete downloader;

    return result;
}

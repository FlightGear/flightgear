//
// Copyright (C) 2017 James Turner  zakalawe@mac.com
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "temporarywidget.h"

#include <QApplication>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QStandardPaths>
#include <QQmlEngine>

#include "fgqcanvasfontcache.h"
#include "fgqcanvasimageloader.h"
#include "canvasitem.h"

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

    qmlRegisterType<CanvasItem>("FlightGear", 1, 0, "CanvasItem");


    TemporaryWidget w;
    w.setNetworkAccess(downloader);
    w.show();

    int result = a.exec();
    delete downloader;

    return result;
}

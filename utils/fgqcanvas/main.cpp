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

#include <QApplication>
#include <QQmlEngine>
#include <QQuickView>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QScreen>

#include "canvasitem.h"
#include "applicationcontroller.h"
#include "canvasdisplay.h"
#include "canvasconnection.h"
#include "canvaspainteddisplay.h"
#include "WindowData.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("FGCanvas");
    a.setOrganizationDomain("flightgear.org");
    a.setOrganizationName("FlightGear");

    QCommandLineParser parser;
    parser.addPositionalArgument("config", QCoreApplication::translate("main", "JSON configuration to load"));
    parser.process(a);

    ApplicationController appController;

    qmlRegisterType<CanvasItem>("FlightGear", 1, 0, "CanvasItem");
    qmlRegisterType<CanvasDisplay>("FlightGear", 1, 0, "CanvasDisplay");
    qmlRegisterType<CanvasPaintedDisplay>("FlightGear", 1, 0, "PaintedCanvasDisplay");

    qmlRegisterUncreatableType<WindowData>("FlightGear", 1, 0, "WindowData", "Don't create me");
    qmlRegisterUncreatableType<CanvasConnection>("FlightGear", 1, 0, "CanvasConnection", "Don't create me");
    qmlRegisterUncreatableType<ApplicationController>("FlightGear", 1, 0, "Application", "Can't create");

    const QStringList args = parser.positionalArguments();

    if (!args.empty()) {
        appController.setDaemonMode();
        appController.loadFromFile(args.front());
    }

    appController.createWindows();

    int result = a.exec();
    return result;
}

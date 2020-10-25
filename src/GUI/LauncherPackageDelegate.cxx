
// Copyright (C) 2020 James Turner <james@flightgear.org>
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

#include "config.h"

#include "LauncherPackageDelegate.hxx"

#include <QDebug>
#include <QJSEngine>

#include <simgear/package/Root.hxx>

#include <Main/globals.hxx>
#include <Network/HTTPClient.hxx>

#include "LauncherNotificationsController.hxx"

LauncherPackageDelegate::LauncherPackageDelegate(QObject* parent) : QObject(parent)
{
    globals->packageRoot()->addDelegate(this);
    const auto http = globals->get_subsystem<FGHTTPClient>();
    _defaultCatalogId = http->getDefaultCatalogId();
}

LauncherPackageDelegate::~LauncherPackageDelegate()
{
    globals->packageRoot()->removeDelegate(this);
}

void LauncherPackageDelegate::catalogRefreshed(simgear::pkg::CatalogRef aCatalog, simgear::pkg::Delegate::StatusCode aReason)
{
    if ((aReason != Delegate::STATUS_REFRESHED) || !aCatalog) {
        return;
    }

    auto nc = LauncherNotificationsController::instance();

    if (aCatalog->migratedFrom() != simgear::pkg::CatalogRef{}) {
        QJSValue args = nc->jsEngine()->newObject();

        args.setProperty("newCatalogName", QString::fromStdString(aCatalog->name()));

        if (aCatalog->id() == _defaultCatalogId) {
            nc->postNotification("did-migrate-official-catalog-to-" + QString::fromStdString(_defaultCatalogId),
                                 QUrl{"qrc:///qml/DidMigrateOfficialCatalogNotification.qml"},
                                 args);
        } else {
            nc->postNotification("did-migrate-catalog-to-" + QString::fromStdString(aCatalog->id()),
                                 QUrl{"qrc:///qml/DidMigrateOtherCatalogNotification.qml"},
                                 args);
        }
    }
}

void LauncherPackageDelegate::finishInstall(simgear::pkg::InstallRef ref, simgear::pkg::Delegate::StatusCode status)
{
    Q_UNUSED(ref)
    Q_UNUSED(status)
}

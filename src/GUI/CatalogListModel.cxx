// CatalogListModel.cxx - part of GUI launcher using Qt5
//
// Written by James Turner, started March 2015.
//
// Copyright (C) 2015 James Turner <zakalawe@mac.com>
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

#include "CatalogListModel.hxx"

#include <QDebug>
#include <QUrl>

// Simgear
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <simgear/package/Package.hxx>

// FlightGear
#include <Main/globals.hxx>

using namespace simgear::pkg;

CatalogListModel::CatalogListModel(QObject* pr, simgear::pkg::RootRef& rootRef) :
    QAbstractListModel(pr),
    m_packageRoot(rootRef)
{
    refresh();
}

CatalogListModel::~CatalogListModel()
{
}

void CatalogListModel::refresh()
{
    beginResetModel();
    m_catalogs = m_packageRoot->allCatalogs();
    endResetModel();
}

int CatalogListModel::rowCount(const QModelIndex& parent) const
{
    return m_catalogs.size();
}

QVariant CatalogListModel::data(const QModelIndex& index, int role) const
{
    simgear::pkg::CatalogRef cat = m_catalogs.at(index.row());

    if (role == Qt::DisplayRole) {
        QString name = QString::fromStdString(cat->name());
        QString desc;
        if (cat->isEnabled()) {
            desc = QString::fromStdString(cat->description()).simplified();
        } else {
            switch (cat->status()) {
            case Delegate::FAIL_NOT_FOUND:
                desc = tr("The catalog data was not found on the server at the expected location (URL)");
                break;
            case Delegate::FAIL_VERSION:
                desc =  tr("The catalog is not comaptible with the version of FlightGear");
                break;
            case Delegate::FAIL_HTTP_FORBIDDEN:
                desc = tr("The catalog server is blocking access from some reason (forbidden)");
                break;
            default:
                desc = tr("disabled due to an internal error");
            }
        }
        return tr("%1 - %2").arg(name).arg(desc);
    } else if (role == Qt::ToolTipRole) {
        return QString::fromStdString(cat->url());
    } else if (role == CatalogUrlRole) {
        return QUrl(QString::fromStdString(cat->url()));
    } else if (role == CatalogIdRole) {
        return QString::fromStdString(cat->id());
    } else if (role == CatalogPackageCountRole) {
        return static_cast<quint32>(cat->packages().size());
    } else if (role == CatalogInstallCountRole) {
        return static_cast<quint32>(cat->installedPackages().size());
    }

    return QVariant();
}

bool CatalogListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return false;
}

Qt::ItemFlags CatalogListModel::flags(const QModelIndex &index) const
{
    auto r = QAbstractListModel::flags(index);
    const auto cat = m_catalogs.at(index.row());
    r.setFlag(Qt::ItemIsEnabled, cat->isEnabled());
    return r;
}

// CatalogListModel.hxx - part of GUI launcher using Qt5
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

#ifndef FG_GUI_CATALOG_LIST_MODEL
#define FG_GUI_CATALOG_LIST_MODEL

#include <QAbstractListModel>
#include <QDateTime>
#include <QDir>
#include <QPixmap>
#include <QStringList>

#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>

const int CatalogUrlRole = Qt::UserRole + 1;
const int CatalogIdRole = Qt::UserRole + 2;

class CatalogListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    CatalogListModel(QObject* pr, simgear::pkg::RootRef& root);

    ~CatalogListModel();

    virtual int rowCount(const QModelIndex& parent) const;

    virtual QVariant data(const QModelIndex& index, int role) const;
    
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

private slots:


private:
    simgear::pkg::RootRef m_packageRoot;
};

#endif // of FG_GUI_CATALOG_LIST_MODEL

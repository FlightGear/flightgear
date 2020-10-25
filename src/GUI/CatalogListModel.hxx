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
const int CatalogPackageCountRole = Qt::UserRole + 3;
const int CatalogInstallCountRole = Qt::UserRole + 4;
const int CatalogStatusRole = Qt::UserRole + 5;
const int CatalogDescriptionRole = Qt::UserRole + 6;
const int CatalogNameRole = Qt::UserRole + 7;
const int CatalogIsNewlyAdded = Qt::UserRole + 8;
const int CatalogEnabled = Qt::UserRole + 9;

class CatalogDelegate;

class CatalogListModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool isAddingCatalog READ isAddingCatalog NOTIFY isAddingCatalogChanged)
    Q_PROPERTY(CatalogStatus statusOfAddingCatalog READ statusOfAddingCatalog NOTIFY statusOfAddingCatalogChanged)
public:
    CatalogListModel(QObject* pr, const simgear::pkg::RootRef& root);

    ~CatalogListModel();

    int rowCount(const QModelIndex& parent) const override;

    QVariant data(const QModelIndex& index, int role) const override;
    
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void removeCatalog(int index);

    Q_INVOKABLE void refreshCatalog(int index);

    Q_INVOKABLE void installDefaultCatalog(bool showAddFeedback);

    // returns the index of the new catalog
    Q_INVOKABLE void addCatalogByUrl(QUrl url);

    Q_INVOKABLE void finalizeAddCatalog();

    Q_INVOKABLE void abandonAddCatalog();

    bool isAddingCatalog() const;

    enum CatalogStatus
    {
        Ok = 0,
        Refreshing,
        NetworkError,
        NotFoundOnServer,
        IncompatibleVersion,
        HTTPForbidden,
        InvalidData,
        UnknownError,
        NoAddInProgress
    };

    Q_ENUMS(CatalogStatus)

    void onCatalogStatusChanged(simgear::pkg::Catalog *cat);

    CatalogStatus statusOfAddingCatalog() const;

signals:
    void isAddingCatalogChanged();
    void statusOfAddingCatalogChanged();

    void catalogsChanged();

public slots:
    void resetData();

private slots:

private:
    simgear::pkg::RootRef m_packageRoot;
    simgear::pkg::CatalogList m_catalogs;

    simgear::pkg::CatalogRef m_newlyAddedCatalog;

    int indexOf(QUrl url);

    CatalogDelegate* m_delegate;

    CatalogStatus translateStatusForCatalog(simgear::pkg::CatalogRef cat) const;
};

#endif // of FG_GUI_CATALOG_LIST_MODEL

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
#include <QTimer>

// Simgear
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>

#include <simgear/package/Package.hxx>
#include <simgear/package/Install.hxx>

// FlightGear
#include <Main/globals.hxx>
#include <Network/HTTPClient.hxx>

using namespace simgear::pkg;

class CatalogDelegate : public simgear::pkg::Delegate
{
public:
    CatalogDelegate(CatalogListModel* outer) : p(outer) {}

    void catalogRefreshed(CatalogRef catalog, StatusCode) override
    {
        p->onCatalogStatusChanged(catalog);
    }

    void startInstall(InstallRef) override {}
    void installProgress(InstallRef, unsigned int, unsigned int) override {}
    void finishInstall(InstallRef, StatusCode ) override {}
private:
    CatalogListModel* p = nullptr;
};


CatalogListModel::CatalogListModel(QObject* pr, const
                                   simgear::pkg::RootRef& rootRef) :
    QAbstractListModel(pr),
    m_packageRoot(rootRef)
{
    m_delegate = new CatalogDelegate(this);
    m_packageRoot->addDelegate(m_delegate);

    resetData();
}

CatalogListModel::~CatalogListModel()
{
    m_packageRoot->removeDelegate(m_delegate);
}

void CatalogListModel::resetData()
{
    CatalogList updatedCatalogs = m_packageRoot->allCatalogs();
    std::sort(updatedCatalogs.begin(), updatedCatalogs.end(),
              [](const CatalogRef& catA, const CatalogRef& catB)
    {   // lexicographic ordering
        return catA->name() < catB->name();
    });

    if (updatedCatalogs == m_catalogs)
        return;

    beginResetModel();
    m_catalogs = updatedCatalogs;
    endResetModel();

    emit catalogsChanged();
}

int CatalogListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(m_catalogs.size());
}

QVariant CatalogListModel::data(const QModelIndex& index, int role) const
{
    const auto cat = m_catalogs.at(static_cast<size_t>(index.row()));
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
                desc =  tr("The catalog is not compatible with the version of FlightGear");
                break;
            case Delegate::FAIL_HTTP_FORBIDDEN:
                desc = tr("The catalog server is blocking access from some reason (forbidden)");
                break;
            default:
                desc = tr("disabled due to an internal error");
            }
        }
        return tr("%1 - %2").arg(name).arg(desc);
    } else if (role == CatalogDescriptionRole) {
        return QString::fromStdString(cat->description());
    } else if (role == CatalogNameRole) {
        return QString::fromStdString(cat->name());
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
    } else if (role == CatalogStatusRole) {
        return translateStatusForCatalog(cat);
    } else if (role == CatalogIsNewlyAdded) {
        return (cat == m_newlyAddedCatalog);
    } else if (role == CatalogEnabled) {
        return cat->isUserEnabled();
    }

    return QVariant();
}

bool CatalogListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    auto cat = m_catalogs.at(static_cast<size_t>(index.row()));
    if (role == CatalogEnabled) {
        cat->setUserEnabled(value.toBool());
        return true;
    }
    return false;
}

Qt::ItemFlags CatalogListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags r = Qt::ItemIsSelectable;
    const auto cat = m_catalogs.at(static_cast<size_t>(index.row()));
    if (cat->isEnabled()) {
        r |= Qt::ItemIsEnabled;
    }
    return r;
}

QHash<int, QByteArray> CatalogListModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();
    result[CatalogUrlRole] = "url";
    result[CatalogIdRole] = "id";
    result[CatalogDescriptionRole] = "description";
    result[CatalogNameRole] = "name";
    result[CatalogStatusRole] = "status";
    result[CatalogIsNewlyAdded] = "isNewlyAdded";
    result[CatalogEnabled] = "enabled";
    return result;
}

void CatalogListModel::removeCatalog(int index)
{
    if ((index < 0) || (index >= static_cast<int>(m_catalogs.size()))) {
        return;
    }

    const std::string removeId = m_catalogs.at(static_cast<size_t>(index))->id();
    m_packageRoot->removeCatalogById(removeId);
    resetData();
}

void CatalogListModel::refreshCatalog(int index)
{
    if ((index < 0) || (index >= static_cast<int>(m_catalogs.size()))) {
        return;
    }
    m_catalogs.at(static_cast<size_t>(index))->refresh();
}

void CatalogListModel::installDefaultCatalog(bool showAddFeedback)
{
    FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
    auto cat = Catalog::createFromUrl(m_packageRoot, http->getDefaultCatalogUrl());
    if (showAddFeedback) {
      cat = m_newlyAddedCatalog;
      emit isAddingCatalogChanged();
      emit statusOfAddingCatalogChanged();
    }

    resetData();
}

void CatalogListModel::addCatalogByUrl(QUrl url)
{
    if (m_newlyAddedCatalog) {
        qWarning() << Q_FUNC_INFO << "already adding a catalog";
        return;
    }

    m_newlyAddedCatalog = Catalog::createFromUrl(m_packageRoot, url.toString().toStdString());
    resetData();
    emit isAddingCatalogChanged();
}

int CatalogListModel::indexOf(QUrl url)
{
    std::string urlString = url.toString().toStdString();
    auto it = std::find_if(m_catalogs.begin(), m_catalogs.end(),
                           [urlString](simgear::pkg::CatalogRef cat) { return cat->url() == urlString;});
    if (it == m_catalogs.end())
        return -1;

    return static_cast<int>(std::distance(m_catalogs.begin(), it));
}

void CatalogListModel::finalizeAddCatalog()
{
    if (!m_newlyAddedCatalog) {
        qWarning() << Q_FUNC_INFO << "no catalog add in progress";
        return;
    }

    auto it = std::find(m_catalogs.begin(), m_catalogs.end(), m_newlyAddedCatalog);
    if (it == m_catalogs.end()) {
        qWarning() << Q_FUNC_INFO << "couldn't find new catalog in m_catalogs" << QString::fromStdString(m_newlyAddedCatalog->url());
        return;
    }

    const int row = static_cast<int>(std::distance(m_catalogs.begin(), it));
    m_newlyAddedCatalog.clear();
    emit isAddingCatalogChanged();
    emit statusOfAddingCatalogChanged();
    emit dataChanged(index(row), index(row));

}

void CatalogListModel::abandonAddCatalog()
{
    if (!m_newlyAddedCatalog)
        return;

    m_packageRoot->removeCatalog(m_newlyAddedCatalog);

    m_newlyAddedCatalog.clear();
    emit isAddingCatalogChanged();
    emit statusOfAddingCatalogChanged();

    resetData();
}

bool CatalogListModel::isAddingCatalog() const
{
    return m_newlyAddedCatalog.get() != nullptr;
}

void CatalogListModel::onCatalogStatusChanged(Catalog* cat)
{
    if (cat == nullptr) {
        resetData();
        return;
    }

    //qInfo() << Q_FUNC_INFO << "for" << QString::fromStdString(cat->url()) << translateStatusForCatalog(cat);

    // download the official catalog often fails with a 404 due to how we
    // compute the version-specific URL. This is the logic which bounces the UI
    // to the fallback URL.
    if (cat->status() == Delegate::FAIL_NOT_FOUND) {
        FGHTTPClient* http = globals->get_subsystem<FGHTTPClient>();
        if (cat->url() == http->getDefaultCatalogUrl()) {
            cat->setUrl(http->getDefaultCatalogFallbackUrl());
            cat->refresh(); // and trigger another refresh
            return;
        }
    }

    if (cat == m_newlyAddedCatalog) {
        // defer this signal slightly so that QML calling finalizeAdd or
        // abandonAdd in response, doesn't re-enter the package code
        QTimer::singleShot(0, this, &CatalogListModel::statusOfAddingCatalogChanged);
        return;
    }

    auto it = std::find(m_catalogs.begin(), m_catalogs.end(), cat);
    if (it == m_catalogs.end())
        return;

    int row = std::distance(m_catalogs.begin(), it);
    emit dataChanged(index(row), index(row));
}

CatalogListModel::CatalogStatus CatalogListModel::translateStatusForCatalog(CatalogRef cat) const
{
    switch (cat->status()) {
    case Delegate::STATUS_SUCCESS:
    case Delegate::STATUS_REFRESHED:
        return Ok;

    case Delegate::FAIL_DOWNLOAD:       return NetworkError;
    case Delegate::STATUS_IN_PROGRESS:  return Refreshing;
    case Delegate::FAIL_NOT_FOUND:      return NotFoundOnServer;
    case Delegate::FAIL_VERSION:        return IncompatibleVersion;
    case Delegate::FAIL_HTTP_FORBIDDEN: return HTTPForbidden;
    case Delegate::FAIL_VALIDATION:     return InvalidData;
    default:
        return UnknownError;
    }
}

CatalogListModel::CatalogStatus CatalogListModel::statusOfAddingCatalog() const
{
    if (!m_newlyAddedCatalog.get()) {
        return NoAddInProgress;
    }

    return translateStatusForCatalog(m_newlyAddedCatalog);
}

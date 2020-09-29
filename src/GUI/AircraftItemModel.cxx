// AircraftModel.cxx - part of GUI launcher using Qt5
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

#include "config.h"

#include "AircraftItemModel.hxx"

#include <QSettings>
#include <QDebug>
#include <QSharedPointer>
#include <QSettings>

// Simgear
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>

// FlightGear
#include <Main/globals.hxx>

#include "QmlAircraftInfo.hxx"
#include "FavouriteAircraftData.hxx"

using namespace simgear::pkg;

bool isPackageFailure(Delegate::StatusCode status)
{
    switch (status) {
    case Delegate::STATUS_SUCCESS:
    case Delegate::STATUS_REFRESHED:
    case Delegate::STATUS_IN_PROGRESS:
        return false;

    default:
        return true;
    }
}

class PackageDelegate : public simgear::pkg::Delegate
{
public:
    PackageDelegate(AircraftItemModel* model) :
        m_model(model)
    {
        m_model->m_packageRoot->addDelegate(this);
    }

    ~PackageDelegate() override
    {
        m_model->m_packageRoot->removeDelegate(this);
    }

protected:
    void catalogRefreshed(CatalogRef aCatalog, StatusCode aReason) override;
    void startInstall(InstallRef aInstall) override
    {
        QModelIndex mi(indexForPackage(aInstall->package()));
        m_model->dataChanged(mi, mi);
    }

    void installProgress(InstallRef aInstall, unsigned int bytes, unsigned int total) override
    {
        Q_UNUSED(bytes)
        Q_UNUSED(total)
        QModelIndex mi(indexForPackage(aInstall->package()));
        m_model->dataChanged(mi, mi);
    }

    void finishInstall(InstallRef aInstall, StatusCode aReason) override
    {
        QModelIndex mi(indexForPackage(aInstall->package()));
        m_model->dataChanged(mi, mi);

        if ((aReason != USER_CANCELLED) && (aReason != STATUS_SUCCESS)) {
            m_model->installFailed(mi, aReason);
        }

        if (aReason == STATUS_SUCCESS) {
            m_model->installSucceeded(mi);
        }

        m_model->installedAircraftCountChanged();
    }

    void availablePackagesChanged() override
    {
        m_model->refreshPackages();
    }

    void installStatusChanged(InstallRef aInstall, StatusCode aReason) override
    {
        Q_UNUSED(aReason)
        QModelIndex mi(indexForPackage(aInstall->package()));
        m_model->dataChanged(mi, mi);
    }

    void finishUninstall(const PackageRef& pkg) override
    {
        QModelIndex mi(indexForPackage(pkg));
        m_model->dataChanged(mi, mi);
        m_model->installedAircraftCountChanged();
    }

private:
    QModelIndex indexForPackage(const PackageRef& ref) const
    {
        auto it = std::find(m_model->m_packages.begin(),
                            m_model->m_packages.end(), ref);
        if (it == m_model->m_packages.end()) {
            return QModelIndex();
        }

        const int offset = static_cast<int>(std::distance(m_model->m_packages.begin(), it));
        return m_model->index(offset + m_model->m_cachedLocalAircraftCount);
    }

    AircraftItemModel* m_model;
};

void PackageDelegate::catalogRefreshed(CatalogRef aCatalog, StatusCode aReason)
{
    if (aReason == STATUS_IN_PROGRESS) {
        // nothing to do
    } else if ((aReason == STATUS_REFRESHED) || (aReason == STATUS_SUCCESS)) {
        m_model->refreshPackages();
    } else if (aReason ==  FAIL_VERSION) {
        // silent about this
    } else {
        qWarning() << "failed refresh of"
            << QString::fromStdString(aCatalog->url()) << ":" << aReason << endl;
    }
}

//////////////////////////////////////////////////////////////////////////
/// \brief AircraftItemModel::AircraftItemModel
/// \param pr
///
AircraftItemModel::AircraftItemModel(QObject* pr) :
    QAbstractListModel(pr)
{
    auto cache = LocalAircraftCache::instance();
    connect(cache, &LocalAircraftCache::scanStarted,
            this, &AircraftItemModel::onScanStarted);
    connect(cache, &LocalAircraftCache::addedItems,
            this, &AircraftItemModel::onScanAddedItems);
    connect(cache, &LocalAircraftCache::cleared,
            this, &AircraftItemModel::onLocalCacheCleared);
}

AircraftItemModel::~AircraftItemModel()
{
    delete m_delegate;
}

void AircraftItemModel::setPackageRoot(const simgear::pkg::RootRef& root)
{
    if (m_packageRoot) {
        delete m_delegate;
        m_delegate = nullptr;
    }

    m_packageRoot = root;

    if (m_packageRoot) {
        m_delegate = new PackageDelegate(this);
        // packages may already be refreshed, so pull now
        refreshPackages();
    }
}

void AircraftItemModel::onScanStarted()
{
    const int numToRemove = m_cachedLocalAircraftCount;
    if (numToRemove > 0) {
        int lastRow = numToRemove - 1;
        beginRemoveRows(QModelIndex(), 0, lastRow);
        m_delegateStates.remove(0, numToRemove);
        m_cachedLocalAircraftCount = 0;
        endRemoveRows();
    }
}

void AircraftItemModel::refreshPackages()
{
    simgear::pkg::PackageList newPkgs = m_packageRoot->allPackages();
    const int firstRow = m_cachedLocalAircraftCount;
    const int newSize = static_cast<int>(newPkgs.size());
    const int newTotalSize = firstRow + newSize;

    if (m_packages.size() != newPkgs.size()) {
        const int oldSize = static_cast<int>(m_packages.size());
        if (newSize > oldSize) {
            // growing
            int firstNewRow = firstRow + oldSize;
            int lastNewRow = firstRow + newSize - 1;
            beginInsertRows(QModelIndex(), firstNewRow, lastNewRow);
            m_packages = newPkgs;
            m_delegateStates.resize(newTotalSize);
            endInsertRows();
        } else {
            // shrinking
            int firstOldRow = firstRow + newSize;
            int lastOldRow = firstRow + oldSize - 1;
            beginRemoveRows(QModelIndex(), firstOldRow, lastOldRow);
            m_packages = newPkgs;
            m_delegateStates.resize(newTotalSize);
            endRemoveRows();
        }
    } else {
        m_packages = newPkgs;
    }

    emit dataChanged(index(firstRow), index(firstRow + newSize - 1));
    emit contentsChanged();
}

int AircraftItemModel::rowCount(const QModelIndex&) const
{
    return m_cachedLocalAircraftCount + static_cast<int>(m_packages.size());
}

QVariant AircraftItemModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (role == AircraftVariantRole) {
        return m_delegateStates.at(row).variant;
    }

    if (role == AircraftIsFavouriteRole) {
        // recursive call here, hope that's okay
        const auto uri = data(index, AircraftPrimaryURIRole).toUrl();
        return FavouriteAircraftData::instance()->isFavourite(uri);
    }

    if (row >= m_cachedLocalAircraftCount) {
        quint32 packageIndex = static_cast<quint32>(row - m_cachedLocalAircraftCount);
        const PackageRef& pkg(m_packages[packageIndex]);
        InstallRef ex = pkg->existingInstall();

        if (role == AircraftInstallPercentRole) {
            return ex.valid() ? ex->downloadedPercent() : 0;
        } else if (role == AircraftInstallDownloadedSizeRole) {
            return static_cast<quint64>(ex.valid() ? ex->downloadedBytes() : 0);
        } else if (role == AircraftPackageRefRole ) {
            return QVariant::fromValue(pkg);
        }

        return dataFromPackage(pkg, m_delegateStates.at(row), role);
    } else {
        const AircraftItemPtr item(LocalAircraftCache::instance()->itemAt(row));
        return dataFromItem(item, m_delegateStates.at(row), role);
    }
}

QVariant AircraftItemModel::dataFromItem(AircraftItemPtr item, const DelegateState& state, int role) const
{
    if (role == AircraftVariantCountRole) {
        return item->variants.count();
    }

    if (role >= AircraftVariantDescriptionRole) {
        int variantIndex = role - AircraftVariantDescriptionRole;
        if (variantIndex == 0) {
            return item->name();
        }

        Q_ASSERT(variantIndex < item->variants.size());
        return item->variants.at(variantIndex)->name();
    }

    if (role == AircraftPrimaryURIRole) {
        return QUrl::fromLocalFile(item->path);
    }

    if (state.variant) {
        if (state.variant <= static_cast<quint32>(item->variants.count())) {
            // show the selected variant
            item = item->variants.at(state.variant - 1);
        }
    }

    if (role == Qt::DisplayRole) {
        if (item->name().isEmpty()) {
            return tr("Missing description for: %1").arg(item->baseName());
        }

        return item->name();
    } else if (role == AircraftPathRole) {
        return item->path;
    } else if (role == AircraftAuthorsRole) {
        return item->authors;
    } else if ((role >= AircraftRatingRole) && (role < AircraftVariantDescriptionRole)) {
        return item->ratings[role - AircraftRatingRole];
    } else if (role == AircraftPackageIdRole) {
        // can we fake an ID? otherwise fall through to a null variant
    } else if (role == AircraftPackageStatusRole) {
        return LocalAircraftCache::PackageInstalled; // always the case
    } else if (role == Qt::ToolTipRole) {
        return item->path;
    } else if (role == AircraftURIRole) {
        return QUrl::fromLocalFile(item->path);
    } else if (role == AircraftHasRatingsRole) {
        bool have = false;
        for (int i=0; i<4; ++i) {
            have |= (item->ratings[i] > 0);
        }
        return have;
    } else if (role == AircraftLongDescriptionRole) {
        return item->description();
    } else if (role == AircraftIsHelicopterRole) {
        return item->usesHeliports;
    } else if (role == AircraftIsSeaplaneRole) {
        return item->usesSeaports;
    } else if ((role == AircraftInstallDownloadedSizeRole) ||
               (role == AircraftPackageSizeRole))
    {
        return 0;
    } else if (role == AircraftStatusRole) {
        return item->status(0 /* variant is always 0 */);
    } else if (role == AircraftMinVersionRole) {
        return item->minFGVersion;
    }

    return QVariant();
}

QVariant AircraftItemModel::dataFromPackage(const PackageRef& item, const DelegateState& state, int role) const
{
    if (role >= AircraftVariantDescriptionRole) {
        const unsigned int variantIndex = static_cast<unsigned int>(role - AircraftVariantDescriptionRole);
        QString desc = QString::fromStdString(item->nameForVariant(variantIndex));
        if (desc.isEmpty()) {
            desc = tr("Missing description for: %1").arg(QString::fromStdString(item->id()));
        }
        return desc;
    }

    if (role == Qt::DisplayRole) {
        QString desc = QString::fromStdString(item->nameForVariant(state.variant));
        if (desc.isEmpty()) {
            desc = tr("Missing description for: %1").arg(QString::fromStdString(item->id()));
        }
        return desc;
    } else if (role == AircraftPathRole) {
        InstallRef i = item->existingInstall();
        if (i.valid()) {
            return QString::fromStdString(i->primarySetPath().utf8Str());
        }
    } else if (role == AircraftPackageIdRole) {
        return QString::fromStdString(item->variants()[state.variant]);
    } else if (role == AircraftPackageStatusRole) {
        InstallRef i = item->existingInstall();
        if (i.valid()) {
            if (i->isDownloading()) {
                return LocalAircraftCache::PackageDownloading;
            }
            if (i->isQueued()) {
                return LocalAircraftCache::PackageQueued;
            }
            if (i->hasUpdate()) {
                return LocalAircraftCache::PackageUpdateAvailable;
            }

            const auto status = i->status();
            if (isPackageFailure(status))
                return LocalAircraftCache::PackageInstallFailed;

            return LocalAircraftCache::PackageInstalled;
        } else {
            return LocalAircraftCache::PackageNotInstalled;
        }
    } else if (role == AircraftVariantCountRole) {
        // this value wants the number of aditional variants, i.e not
        // including the primary. Hence the -1 term.
        return static_cast<quint32>(item->variants().size() - 1);
    } else if (role == AircraftAuthorsRole) {
        std::string authors = item->getLocalisedProp("author", state.variant);
        if (!authors.empty()) {
            return QString::fromStdString(authors);
        }
    } else if (role == AircraftLongDescriptionRole) {
        std::string longDesc = item->getLocalisedProp("description", state.variant);
        return QString::fromStdString(longDesc).simplified();
    } else if (role == AircraftPackageSizeRole) {
        return static_cast<int>(item->fileSizeBytes());
    } else if (role == AircraftURIRole) {
        return QUrl("package:" + QString::fromStdString(item->qualifiedVariantId(state.variant)));
    } else if (role == AircraftPrimaryURIRole) {
        return QUrl("package:" + QString::fromStdString(item->qualifiedId()));
    } else if (role == AircraftHasRatingsRole) {
        return item->properties()->hasChild("rating");
    } else if ((role >= AircraftRatingRole) && (role < AircraftVariantDescriptionRole)) {
        return packageRating(item, role - AircraftRatingRole);
    } else if (role == AircraftStatusRole) {
        return QmlAircraftInfo::packageAircraftStatus(item);
    } else if (role == AircraftMinVersionRole) {
        const std::string v = item->properties()->getStringValue("minimum-fg-version");
        if (!v.empty()) {
            return QString::fromStdString(v);
        }
    } else if (role == AircraftIsHelicopterRole) {
        return item->hasTag("helicopter");
    } else if (role == AircraftIsSeaplaneRole) {
        return item->hasTag("seaplane") || item->hasTag("floats");
    }

    return QVariant();
}

QVariant AircraftItemModel::packageRating(const PackageRef& p, int ratingIndex) const
{
    return LocalAircraftCache::ratingFromProperties(p->properties()->getChild("rating"), ratingIndex);
}

bool AircraftItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
  {
      int row = index.row();
      quint32 newValue = value.toUInt();

      if (role == AircraftVariantRole) {
          if (m_delegateStates[row].variant == newValue) {
              return true;
          }

          m_delegateStates[row].variant = newValue;
          emit dataChanged(index, index);
          return true;
      } else if (role == AircraftIsFavouriteRole) {
          const auto uri = data(index, AircraftPrimaryURIRole).toUrl();
          bool changed = FavouriteAircraftData::instance()->setFavourite(uri, value.toBool());
          if (changed) {
              emit dataChanged(index, index);
          }
      }

      return false;
}

QHash<int, QByteArray> AircraftItemModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();

    result[Qt::DisplayRole] = "title";
    result[AircraftURIRole] = "uri";
    result[AircraftPackageIdRole] = "package";
    result[AircraftAuthorsRole] = "authors";
    result[AircraftVariantCountRole] = "variantCount";
    result[AircraftLongDescriptionRole] = "description";
    result[AircraftPackageSizeRole] = "packageSizeBytes";
    result[AircraftPackageStatusRole] = "packageStatus";

    result[AircraftInstallDownloadedSizeRole] = "downloadedBytes";
    result[AircraftVariantRole] = "activeVariant";
    result[AircraftIsFavouriteRole] = "favourite";

    result[AircraftStatusRole] = "aircraftStatus";
    result[AircraftMinVersionRole] = "requiredFGVersion";

    result[AircraftHasRatingsRole] = "hasRatings";
    result[AircraftRatingRole] = "ratingFDM";
    result[AircraftRatingRole + 1] = "ratingSystems";
    result[AircraftRatingRole + 2] = "ratingCockpit";
    result[AircraftRatingRole + 3] = "ratingExterior";

    return result;
}

QModelIndex AircraftItemModel::indexOfAircraftURI(QUrl uri) const
{
    if (uri.isEmpty()) {
        return QModelIndex();
    }

    if (uri.isLocalFile()) {
        int row = LocalAircraftCache::instance()->findIndexWithUri(uri);
        if (row >= 0) {
            return index(row);
        }
    } else if (uri.scheme() == "package") {
        QString ident = uri.path();
        int rowOffset = m_cachedLocalAircraftCount;

        PackageRef pkg = m_packageRoot->getPackageById(ident.toStdString());
        if (pkg) {
            const auto numPackages = m_packages.size();
            for (unsigned int i=0; i < numPackages; ++i) {
                if (m_packages.at(i) == pkg) {
                    return index(static_cast<int>(rowOffset + i));
                }
            } // of linear package scan
        }
    } else if (uri.scheme() == "") {
        // Empty URI scheme (no selection), nothing to do
    } else {
        qWarning() << "Unknown aircraft URI scheme" << uri << uri.scheme();
    }

    return QModelIndex();
}

void AircraftItemModel::selectVariantForAircraftURI(QUrl uri)
{
    if (uri.isEmpty()) {
        return;
    }

    int variantIndex = 0;
    QModelIndex modelIndex;

    if (uri.isLocalFile()) {
        int row = LocalAircraftCache::instance()->findIndexWithUri(uri);
        if (row < 0) {
            return;
        }

        modelIndex = index(row);
        // now check if we are actually selecting a variant
        const AircraftItemPtr item = LocalAircraftCache::instance()->itemAt(row);

        const QString path = uri.toLocalFile();
        for (int vr=0; vr < item->variants.size(); ++vr) {
            if (item->variants.at(vr)->path == path) {
                variantIndex = vr + 1;
                break;
            }
        }
    } else if (uri.scheme() == "package") {
        QString ident = uri.path();
        int rowOffset = m_cachedLocalAircraftCount;

        PackageRef pkg = m_packageRoot->getPackageById(ident.toStdString());
        if (pkg) {
            for (size_t i=0; i < m_packages.size(); ++i) {
                if (m_packages[i] == pkg) {
                    modelIndex = index(rowOffset + static_cast<int>(i));
                    variantIndex = pkg->indexOfVariant(ident.toStdString());
                    break;
                }
            } // of linear package scan
        }
    } else {
        qWarning() << "Unknown aircraft URI scheme" << uri << uri.scheme();
        return;
    }

    if (modelIndex.isValid()) {
        setData(modelIndex, variantIndex, AircraftVariantRole);
    }
}

QString AircraftItemModel::nameForAircraftURI(QUrl uri) const
{
    if (uri.isLocalFile()) {
        AircraftItemPtr item = LocalAircraftCache::instance()->findItemWithUri(uri);
        if (!item) {
            return {};
        }

        const QString path = uri.toLocalFile();
        if (item->path == path) {
            return item->name();
        }

        // check variants too
        for (int vr=0; vr < item->variants.size(); ++vr) {
            auto variant = item->variants.at(vr);
            if (variant->path == path) {
                return variant->name();
            }
        }
    } else if (uri.scheme() == "package") {
        QString ident = uri.path();
        PackageRef pkg = m_packageRoot->getPackageById(ident.toStdString());
        if (pkg) {
            const auto variantIndex = pkg->indexOfVariant(ident.toStdString());
            return QString::fromStdString(pkg->nameForVariant(variantIndex));
        }
    } else {
        qWarning() << "Unknown aircraft URI scheme" << uri << uri.scheme();
    }

    return {};
}

int AircraftItemModel::installedAircraftCount() const
{
    int c = m_cachedLocalAircraftCount;

    for (const auto& cat : m_packageRoot->catalogs()) {
        c += static_cast<int>(cat->installedPackages().size());
    }

    return c;
}

void AircraftItemModel::onScanAddedItems(int addedCount)
{
    Q_UNUSED(addedCount)
    const auto items = LocalAircraftCache::instance()->allItems();
    const int newItemCount = items.size() - m_cachedLocalAircraftCount;
    const int firstRow = m_cachedLocalAircraftCount;
    const int lastRow = firstRow + newItemCount - 1;

    beginInsertRows(QModelIndex(), firstRow, lastRow);
    m_delegateStates.insert(m_cachedLocalAircraftCount, newItemCount, {});
    m_cachedLocalAircraftCount += newItemCount;
    endInsertRows();
    emit contentsChanged();
    emit installedAircraftCountChanged();
}

void AircraftItemModel::onLocalCacheCleared()
{
    if (m_cachedLocalAircraftCount > 0) {
        const int firstRow = 0;
        const int lastRow = m_cachedLocalAircraftCount - 1;

        beginRemoveRows(QModelIndex(), firstRow, lastRow);
        m_delegateStates.remove(0, m_cachedLocalAircraftCount);
        m_cachedLocalAircraftCount = 0;
        endRemoveRows();
    }

    emit installedAircraftCountChanged();
}

void AircraftItemModel::installFailed(QModelIndex index, simgear::pkg::Delegate::StatusCode reason)
{
    QString msg;
    switch (reason) {
        case Delegate::FAIL_CHECKSUM:
            msg = tr("Invalid package checksum"); break;
        case Delegate::FAIL_DOWNLOAD:
            msg = tr("Download failed"); break;
        case Delegate::FAIL_EXTRACT:
            msg = tr("Package could not be extracted"); break;
        case Delegate::FAIL_FILESYSTEM:
            msg = tr("A local file-system error occurred"); break;
        case Delegate::FAIL_NOT_FOUND:
            msg = tr("Package file missing from download server"); break;
        case Delegate::FAIL_UNKNOWN:
        default:
            msg = tr("Unknown reason");
    }

    emit aircraftInstallFailed(index, msg);
}

void AircraftItemModel::installSucceeded(QModelIndex index)
{
    emit aircraftInstallCompleted(index);
}

bool AircraftItemModel::isIndexRunnable(const QModelIndex& index) const
{
    if (index.row() < m_cachedLocalAircraftCount) {
        return true; // local file, always runnable
    }

    quint32 packageIndex = static_cast<quint32>(index.row() - m_cachedLocalAircraftCount);
    const PackageRef& pkg(m_packages[packageIndex]);
    InstallRef ex = pkg->existingInstall();
    if (!ex.valid()) {
        return false; // not installed
    }

    return !ex->isDownloading();
}


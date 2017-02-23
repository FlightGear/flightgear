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

#include "AircraftModel.hxx"

#include <QDir>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QDataStream>
#include <QSettings>
#include <QDebug>
#include <QSharedPointer>

// Simgear
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>

// FlightGear
#include <Main/globals.hxx>

const int STANDARD_THUMBNAIL_HEIGHT = 128;
const int STANDARD_THUMBNAIL_WIDTH = 172;
static quint32 CACHE_VERSION = 7;

using namespace simgear::pkg;

AircraftItem::AircraftItem()
{
}

AircraftItem::AircraftItem(QDir dir, QString filePath)
{
    SGPropertyNode root;
    readProperties(filePath.toStdString(), &root);

    if (!root.hasChild("sim")) {
        throw sg_io_exception(std::string("Malformed -set.xml file"), filePath.toStdString());
    }

    SGPropertyNode_ptr sim = root.getNode("sim");

    path = filePath;
    pathModTime = QFileInfo(path).lastModified();
    if (sim->getBoolValue("exclude-from-gui", false)) {
        excluded = true;
        return;
    }

    description = sim->getStringValue("description");
    authors =  sim->getStringValue("author");

    if (sim->hasChild("rating")) {
        SGPropertyNode_ptr ratingsNode = sim->getNode("rating");
        ratings[0] = ratingsNode->getIntValue("FDM");
        ratings[1] = ratingsNode->getIntValue("systems");
        ratings[2] = ratingsNode->getIntValue("cockpit");
        ratings[3] = ratingsNode->getIntValue("model");

    }

    if (sim->hasChild("long-description")) {
        // clean up any XML whitspace in the text.
        longDescription = QString(sim->getStringValue("long-description")).simplified();
    }

    if (sim->hasChild("variant-of")) {
        variantOf = sim->getStringValue("variant-of");
    } else {
        isPrimary = true;
    }

    if (sim->hasChild("primary-set")) {
        isPrimary = sim->getBoolValue("primary-set");
    }

    if (sim->hasChild("tags")) {
        SGPropertyNode_ptr tagsNode = sim->getChild("tags");
        int nChildren = tagsNode->nChildren();
        for (int i = 0; i < nChildren; i++) {
            const SGPropertyNode* c = tagsNode->getChild(i);
            if (strcmp(c->getName(), "tag") == 0) {
                const char* tagName = c->getStringValue();
                usesHeliports |= (strcmp(tagName, "helicopter") == 0);
                // could also consider vtol tag?
                usesSeaports |= (strcmp(tagName, "seaplane") == 0);
                usesSeaports |= (strcmp(tagName, "floats") == 0);
            }
        } // of tags iteration
    } // of set-xml has tags

    if (sim->hasChild("previews")) {
        SGPropertyNode_ptr previewsNode = sim->getChild("previews");
        for (auto previewNode : previewsNode->getChildren("preview")) {
            // add file path as url
            QString pathInXml = QString::fromStdString(previewNode->getStringValue("path"));
            QString previewPath = dir.absoluteFilePath(pathInXml);
            previews.append(QUrl::fromLocalFile(previewPath));
        }
    }

    if (sim->hasChild("thumbnail")) {
        thumbnailPath = sim->getStringValue("thumbnail");
    } else {
        thumbnailPath = "thumbnail.jpg";
    }
}

QString AircraftItem::baseName() const
{
    QString fn = QFileInfo(path).fileName();
    fn.truncate(fn.count() - 8);
    return fn;
}

void AircraftItem::fromDataStream(QDataStream& ds)
{
    ds >> path >> pathModTime >> excluded;
    if (excluded) {
        return;
    }

    ds >> description >> longDescription >> authors >> variantOf >> isPrimary;
    for (int i=0; i<4; ++i) ds >> ratings[i];
    ds >> previews;
    ds >> thumbnailPath;
}

void AircraftItem::toDataStream(QDataStream& ds) const
{
    ds << path << pathModTime << excluded;
    if (excluded) {
        return;
    }

    ds << description << longDescription << authors << variantOf << isPrimary;
    for (int i=0; i<4; ++i) ds << ratings[i];
    ds << previews;
    ds << thumbnailPath;
}

QPixmap AircraftItem::thumbnail(bool loadIfRequired) const
{
    if (m_thumbnail.isNull() && loadIfRequired) {
        QFileInfo info(path);
        QDir dir = info.dir();
        if (dir.exists(thumbnailPath)) {
            m_thumbnail.load(dir.filePath(thumbnailPath));
            // resize to the standard size
            if (m_thumbnail.height() > STANDARD_THUMBNAIL_HEIGHT) {
                m_thumbnail = m_thumbnail.scaledToHeight(STANDARD_THUMBNAIL_HEIGHT, Qt::SmoothTransformation);
            }
        }
    }

    return m_thumbnail;
}

class AircraftScanThread : public QThread
{
    Q_OBJECT
public:
    AircraftScanThread(QStringList dirsToScan) :
        m_dirs(dirsToScan),
        m_done(false)
    {
    }

    ~AircraftScanThread()
    {
    }

    /** thread-safe access to items already scanned */
    QVector<AircraftItemPtr> items()
    {
        QVector<AircraftItemPtr> result;
        QMutexLocker g(&m_lock);
        result.swap(m_items);
        g.unlock();
        return result;
    }

    void setDone()
    {
        m_done = true;
    }
Q_SIGNALS:
    void addedItems();

protected:
    virtual void run()
    {
        readCache();

        Q_FOREACH(QString d, m_dirs) {
            scanAircraftDir(QDir(d));
            if (m_done) {
                return;
            }
        }

        writeCache();
    }

private:
    void readCache()
    {
        QSettings settings;
        QByteArray cacheData = settings.value("aircraft-cache").toByteArray();
        if (!cacheData.isEmpty()) {
            QDataStream ds(cacheData);
            quint32 count, cacheVersion;
            ds >> cacheVersion >> count;

            if (cacheVersion != CACHE_VERSION) {
                return; // mis-matched cache, version, drop
            }

             for (quint32 i=0; i<count; ++i) {
                AircraftItemPtr item(new AircraftItem);
                item->fromDataStream(ds);

                QFileInfo finfo(item->path);
                if (finfo.exists() && (finfo.lastModified() == item->pathModTime)) {
                    // corresponding -set.xml file still exists and is
                    // unmodified
                    m_cachedItems[item->path] = item;
                }
            } // of cached item iteration
        }
    }

    void writeCache()
    {
        QSettings settings;
        QByteArray cacheData;
        {
            QDataStream ds(&cacheData, QIODevice::WriteOnly);
            quint32 count = m_nextCache.count();
            ds << CACHE_VERSION << count;

            Q_FOREACH(AircraftItemPtr item, m_nextCache.values()) {
                item->toDataStream(ds);
            }
        }

        settings.setValue("aircraft-cache", cacheData);
    }

    void scanAircraftDir(QDir path)
    {
        QTime t;
        t.start();

        QStringList filters;
        filters << "*-set.xml";
        Q_FOREACH(QFileInfo child, path.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QDir childDir(child.absoluteFilePath());
            QMap<QString, AircraftItemPtr> baseAircraft;
            QList<AircraftItemPtr> variants;

            Q_FOREACH(QFileInfo xmlChild, childDir.entryInfoList(filters, QDir::Files)) {
                try {
                    QString absolutePath = xmlChild.absoluteFilePath();
                    AircraftItemPtr item;

                    if (m_cachedItems.contains(absolutePath)) {
                        item = m_cachedItems.value(absolutePath);
                    } else {
                        item = AircraftItemPtr(new AircraftItem(childDir, absolutePath));
                    }

                    m_nextCache[absolutePath] = item;

                    if (item->excluded) {
                        continue;
                    }

                    if (item->isPrimary) {
                        baseAircraft.insert(item->baseName(), item);
                    } else {
                        variants.append(item);
                    }
                } catch (sg_exception& e) {
                    continue;
                }

                if (m_done) {
                    return;
                }
            } // of set.xml iteration

            // bind variants to their principals
            Q_FOREACH(AircraftItemPtr item, variants) {
                if (!baseAircraft.contains(item->variantOf)) {
                    qWarning() << "can't find principal aircraft " << item->variantOf << " for variant:" << item->path;
                    continue;
                }

                baseAircraft.value(item->variantOf)->variants.append(item);
            }

            // lock mutex while we modify the items array
            {
                QMutexLocker g(&m_lock);
                m_items+=(baseAircraft.values().toVector());
            }

            emit addedItems();
        } // of subdir iteration
    }

    QMutex m_lock;
    QStringList m_dirs;
    QVector<AircraftItemPtr> m_items;

    QMap<QString, AircraftItemPtr > m_cachedItems;
    QMap<QString, AircraftItemPtr > m_nextCache;

    bool m_done;
};

class PackageDelegate : public simgear::pkg::Delegate
{
public:
    PackageDelegate(AircraftItemModel* model) :
        m_model(model)
    {
        m_model->m_packageRoot->addDelegate(this);
    }

    ~PackageDelegate()
    {
        m_model->m_packageRoot->removeDelegate(this);
    }

protected:
    virtual void catalogRefreshed(CatalogRef aCatalog, StatusCode aReason)
    {
        if (aReason == STATUS_IN_PROGRESS) {
            // nothing to do
        } else if ((aReason == STATUS_REFRESHED) || (aReason == STATUS_SUCCESS)) {
            m_model->refreshPackages();
        } else {
            qWarning() << "failed refresh of "
                << QString::fromStdString(aCatalog->url()) << ":" << aReason << endl;
        }
    }

    virtual void startInstall(InstallRef aInstall)
    {
        QModelIndex mi(indexForPackage(aInstall->package()));
        m_model->dataChanged(mi, mi);
    }

    virtual void installProgress(InstallRef aInstall, unsigned int bytes, unsigned int total)
    {
        Q_UNUSED(bytes);
        Q_UNUSED(total);
        QModelIndex mi(indexForPackage(aInstall->package()));
        m_model->dataChanged(mi, mi);
    }

    virtual void finishInstall(InstallRef aInstall, StatusCode aReason)
    {
        QModelIndex mi(indexForPackage(aInstall->package()));
        m_model->dataChanged(mi, mi);

        if ((aReason != USER_CANCELLED) && (aReason != STATUS_SUCCESS)) {
            m_model->installFailed(mi, aReason);
        }

        if (aReason == STATUS_SUCCESS) {
            m_model->installSucceeded(mi);
        }
    }

    virtual void availablePackagesChanged() Q_DECL_OVERRIDE
    {
        m_model->refreshPackages();
    }

    virtual void dataForThumbnail(const std::string& aThumbnailUrl,
                                  size_t length, const uint8_t* bytes)
    {
        QImage img = QImage::fromData(QByteArray::fromRawData(reinterpret_cast<const char*>(bytes), length));
        if (img.isNull()) {
            qWarning() << "failed to load image data for URL:" <<
                QString::fromStdString(aThumbnailUrl);
            return;
        }

        QPixmap pix = QPixmap::fromImage(img);
        if (pix.height() > STANDARD_THUMBNAIL_HEIGHT) {
            pix = pix.scaledToHeight(STANDARD_THUMBNAIL_HEIGHT, Qt::SmoothTransformation);
        }

        m_model->m_downloadedPixmapCache.insert(QString::fromStdString(aThumbnailUrl), pix);

        // notify any affected items. Linear scan here avoids another map/dict structure.
        for (auto pkg : m_model->m_packages) {
            const int variantCount = pkg->variants().size();
            bool notifyChanged = false;

            for (int v=0; v < variantCount; ++v) {
                const Package::Thumbnail& thumb(pkg->thumbnailForVariant(v));
                if (thumb.url == aThumbnailUrl) {
                    notifyChanged = true;
                }
            }

            if (notifyChanged) {
                QModelIndex mi = indexForPackage(pkg);
                m_model->dataChanged(mi, mi);
            }
        } // of packages iteration
    }

private:
    QModelIndex indexForPackage(const PackageRef& ref) const
    {
        auto it = std::find(m_model->m_packages.begin(),
                            m_model->m_packages.end(), ref);
        if (it == m_model->m_packages.end()) {
            return QModelIndex();
        }

        size_t offset = it - m_model->m_packages.begin();
        return m_model->index(offset + m_model->m_items.size());
    }

    AircraftItemModel* m_model;
};

AircraftItemModel::AircraftItemModel(QObject* pr) :
    QAbstractListModel(pr)
{
}

AircraftItemModel::~AircraftItemModel()
{
    abandonCurrentScan();
    delete m_delegate;
}

void AircraftItemModel::setPackageRoot(const simgear::pkg::RootRef& root)
{
    if (m_packageRoot) {
        delete m_delegate;
        m_delegate = NULL;
    }

    m_packageRoot = root;

    if (m_packageRoot) {
        m_delegate = new PackageDelegate(this);
        // packages may already be refreshed, so pull now
        refreshPackages();
    }
}

void AircraftItemModel::setPaths(QStringList paths)
{
    m_paths = paths;
}

void AircraftItemModel::setMessageWidgetVisible(bool vis)
{
    if (m_showMessageWidget == vis) {
        return;
    }

    m_showMessageWidget = vis;

    if (vis) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_items.prepend(AircraftItemPtr(new AircraftItem));
        m_delegateStates.prepend(DelegateState());
        endInsertRows();
    } else {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_items.removeAt(0);
        m_delegateStates.removeAt(0);
        endRemoveRows();
    }
}

QModelIndex AircraftItemModel::messageWidgetIndex() const
{
    if (!m_showMessageWidget) {
        return QModelIndex();
    }

    return index(0);
}

void AircraftItemModel::scanDirs()
{
	abandonCurrentScan();

    int firstRow = (m_showMessageWidget ? 1 : 0);
	int numToRemove = m_items.size() - firstRow;
	if (numToRemove > 0) {
		int lastRow = firstRow + numToRemove - 1;

		beginRemoveRows(QModelIndex(), firstRow, lastRow);
		m_items.remove(firstRow, numToRemove);
		m_delegateStates.remove(firstRow, numToRemove);
		endRemoveRows();
	}

    QStringList dirs = m_paths;

    Q_FOREACH(SGPath ap, globals->get_aircraft_paths()) {
        dirs << QString::fromStdString(ap.utf8Str());
    }

    SGPath rootAircraft(globals->get_fg_root());
    rootAircraft.append("Aircraft");
    dirs << QString::fromStdString(rootAircraft.utf8Str());

    m_scanThread = new AircraftScanThread(dirs);
    connect(m_scanThread, &AircraftScanThread::finished, this,
            &AircraftItemModel::onScanFinished);
    connect(m_scanThread, &AircraftScanThread::addedItems,
            this, &AircraftItemModel::onScanResults);
    m_scanThread->start();
}

void AircraftItemModel::abandonCurrentScan()
{
    if (m_scanThread) {
        m_scanThread->setDone();
        m_scanThread->wait(1000);
        delete m_scanThread;
        m_scanThread = NULL;
    }
}

void AircraftItemModel::refreshPackages()
{
    simgear::pkg::PackageList newPkgs = m_packageRoot->allPackages();
    const int firstRow = m_items.size();
    const int newSize = newPkgs.size();
    const int newTotalSize = firstRow + newSize;

    if (m_packages.size() != newPkgs.size()) {
        int oldSize = m_packages.size();
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
    emit packagesNeedUpdating(!m_packageRoot->packagesNeedingUpdate().empty());
}

int AircraftItemModel::rowCount(const QModelIndex& parent) const
{
    return m_items.size() + m_packages.size();
}

QVariant AircraftItemModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (m_showMessageWidget) {
        if (row == 0) {
            if (role == AircraftPackageStatusRole) {
                return MessageWidget;
            }

            return QVariant();
        }
    }

    if (role == AircraftVariantRole) {
        return m_delegateStates.at(row).variant;
    }

    if (row >= m_items.size()) {
        quint32 packageIndex = row - m_items.size();
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
        const AircraftItemPtr item(m_items.at(row));
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
        return item->variants.at(variantIndex)->description;
    }

    if (state.variant) {
        if (state.variant <= static_cast<quint32>(item->variants.count())) {
            // show the selected variant
            item = item->variants.at(state.variant - 1);
        }
    }

    if (role == AircraftThumbnailSizeRole) {
        QPixmap pm = item->thumbnail(false);
        if (pm.isNull()) {
            return QSize(STANDARD_THUMBNAIL_WIDTH, STANDARD_THUMBNAIL_HEIGHT);
        }
        return pm.size();
    }

    if (role == Qt::DisplayRole) {
        if (item->description.isEmpty()) {
            return tr("Missing description for: %1").arg(item->baseName());
        }

        return item->description;
    } else if (role == Qt::DecorationRole) {
        return item->thumbnail();
    } else if (role == AircraftPathRole) {
        return item->path;
    } else if (role == AircraftAuthorsRole) {
        return item->authors;
    } else if ((role >= AircraftRatingRole) && (role < AircraftVariantDescriptionRole)) {
        return item->ratings[role - AircraftRatingRole];
    } else if (role == AircraftPreviewsRole) {
        QVariantList result;
        Q_FOREACH(QUrl u, item->previews) {
            result.append(u);
        }
        return result;
    } else if (role == AircraftThumbnailRole) {
        return item->thumbnail();
    } else if (role == AircraftPackageIdRole) {
        // can we fake an ID? otherwise fall through to a null variant
    } else if (role == AircraftPackageStatusRole) {
        return PackageInstalled; // always the case
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
        return item->longDescription;
    } else if (role == AircraftIsHelicopterRole) {
        return item->usesHeliports;
    } else if (role == AircraftIsSeaplaneRole) {
        return item->usesSeaports;
    }

    return QVariant();
}

QVariant AircraftItemModel::dataFromPackage(const PackageRef& item, const DelegateState& state, int role) const
{
    if (role == Qt::DecorationRole) {
        role = AircraftThumbnailRole;
    }

    if (role >= AircraftVariantDescriptionRole) {
        int variantIndex = role - AircraftVariantDescriptionRole;
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
                return PackageDownloading;
            }
            if (i->isQueued()) {
                return PackageQueued;
            }
            if (i->hasUpdate()) {
                return PackageUpdateAvailable;
            }

            return PackageInstalled;
        } else {
            return PackageNotInstalled;
        }
    } else if (role == AircraftVariantCountRole) {
        // this value wants the number of aditional variants, i.e not
        // including the primary. Hence the -1 term.
        return static_cast<quint32>(item->variants().size() - 1);
    } else if (role == AircraftThumbnailSizeRole) {
        QPixmap pm = packageThumbnail(item, state, false).value<QPixmap>();
        if (pm.isNull())
            return QSize(STANDARD_THUMBNAIL_WIDTH, STANDARD_THUMBNAIL_HEIGHT);
        return pm.size();
    } else if (role == AircraftThumbnailRole) {
        return packageThumbnail(item, state);
    } else if (role == AircraftPreviewsRole) {
        return packagePreviews(item, state);
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
    } else if (role == AircraftHasRatingsRole) {
        return item->properties()->hasChild("rating");
    } else if ((role >= AircraftRatingRole) && (role < AircraftVariantDescriptionRole)) {
        int ratingIndex = role - AircraftRatingRole;
        SGPropertyNode* ratings = item->properties()->getChild("rating");
        if (!ratings) {
            return QVariant();
        }
        return ratings->getChild(ratingIndex)->getIntValue();
    }

    return QVariant();
}

QVariant AircraftItemModel::packageThumbnail(PackageRef p, const DelegateState& ds, bool download) const
{
    const Package::Thumbnail& thumb(p->thumbnailForVariant(ds.variant));
    if (thumb.url.empty()) {
        return QVariant();
    }

    QString urlQString(QString::fromStdString(thumb.url));
    if (m_downloadedPixmapCache.contains(urlQString)) {
        // cache hit, easy
        return m_downloadedPixmapCache.value(urlQString);
    }

// check the on-disk store.
    InstallRef ex = p->existingInstall();
    if (ex.valid()) {
        SGPath thumbPath = ex->path() / thumb.path;
        if (thumbPath.exists()) {
            QPixmap pix;
            pix.load(QString::fromStdString(thumbPath.utf8Str()));
            // resize to the standard size
            if (pix.height() > STANDARD_THUMBNAIL_HEIGHT) {
                pix = pix.scaledToHeight(STANDARD_THUMBNAIL_HEIGHT);
            }
            m_downloadedPixmapCache[urlQString] = pix;
            return pix;
        }
    } // of have existing install

    if (download) {
        m_packageRoot->requestThumbnailData(thumb.url);
    }

    return QVariant();
}

QVariant AircraftItemModel::packagePreviews(PackageRef p, const DelegateState& ds) const
{
    const Package::PreviewVec& previews = p->previewsForVariant(ds.variant);
    if (previews.empty()) {
        return QVariant();
    }

    QVariantList result;
    // if we have an install, return file URLs, not remote (http) ones
    InstallRef ex = p->existingInstall();
    if (ex.valid()) {
        for (auto p : previews) {
            SGPath localPreviewPath = ex->path() / p.path;
            if (!localPreviewPath.exists()) {
                qWarning() << "missing local preview" << QString::fromStdString(localPreviewPath.utf8Str());
                continue;
            }
            result.append(QUrl::fromLocalFile(QString::fromStdString(localPreviewPath.utf8Str())));
        }
    }

    // return remote urls
    for (auto p : previews) {
        result.append(QUrl(QString::fromStdString(p.url)));
    }

    return result;
}

bool AircraftItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
  {
      int row = index.row();
      int newValue = value.toInt();

      if (role == AircraftVariantRole) {
          if (m_delegateStates[row].variant == newValue) {
              return true;
          }

          m_delegateStates[row].variant = newValue;
          emit dataChanged(index, index);
          return true;
      }

      return false;
  }

QModelIndex AircraftItemModel::indexOfAircraftURI(QUrl uri) const
{
    if (uri.isEmpty()) {
        return QModelIndex();
    }

    if (uri.isLocalFile()) {
        QString path = uri.toLocalFile();
        for (int row=0; row <m_items.size(); ++row) {
            const AircraftItemPtr item(m_items.at(row));
            if (item->path == path) {
                return index(row);
            }

            // check variants too
            for (int vr=0; vr < item->variants.size(); ++vr) {
                if (item->variants.at(vr)->path == path) {
                    return index(row);
                }
            }
        }
    } else if (uri.scheme() == "package") {
        QString ident = uri.path();
        int rowOffset = m_items.size();

        PackageRef pkg = m_packageRoot->getPackageById(ident.toStdString());
        if (pkg) {
            for (size_t i=0; i < m_packages.size(); ++i) {
                if (m_packages[i] == pkg) {
                    return index(rowOffset + i);
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
        QString path = uri.toLocalFile();
        for (int row=0; row <m_items.size(); ++row) {
            const AircraftItemPtr item(m_items.at(row));
            if (item->path == path) {
                modelIndex = index(row);
                variantIndex = 0;
                break;
            }

            // check variants too
            for (int vr=0; vr < item->variants.size(); ++vr) {
                if (item->variants.at(vr)->path == path) {
                    modelIndex = index(row);
                    variantIndex = vr + 1;
                    break;
                }
            }
        }
    } else if (uri.scheme() == "package") {
        QString ident = uri.path();
        int rowOffset = m_items.size();

        PackageRef pkg = m_packageRoot->getPackageById(ident.toStdString());
        if (pkg) {
            for (size_t i=0; i < m_packages.size(); ++i) {
                if (m_packages[i] == pkg) {
                    modelIndex =  index(rowOffset + i);
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
        QString path = uri.toLocalFile();
        for (int row=0; row <m_items.size(); ++row) {
            const AircraftItemPtr item(m_items.at(row));
            if (item->path == path) {
                return item->description;
            }

            // check variants too
            for (int vr=0; vr < item->variants.size(); ++vr) {
                if (item->variants.at(vr)->path == path) {
                    return item->description;
                }
            }
        }
    } else if (uri.scheme() == "package") {
        QString ident = uri.path();
        PackageRef pkg = m_packageRoot->getPackageById(ident.toStdString());
        if (pkg) {
            int variantIndex = pkg->indexOfVariant(ident.toStdString());
            return QString::fromStdString(pkg->nameForVariant(variantIndex));
        }
    } else {
        qWarning() << "Unknown aircraft URI scheme" << uri << uri.scheme();
        return QString();
    }

    return QString();
}

void AircraftItemModel::onScanResults()
{
    QVector<AircraftItemPtr> newItems = m_scanThread->items();
    if (newItems.isEmpty())
        return;

    int firstRow = m_items.count();
    int lastRow = firstRow + newItems.count() - 1;
    beginInsertRows(QModelIndex(), firstRow, lastRow);
    m_items+=newItems;

    // default variants in all cases
    for (int i=0; i< newItems.count(); ++i) {
        m_delegateStates.insert(firstRow + i, DelegateState());
    }

    endInsertRows();
}

void AircraftItemModel::onScanFinished()
{
    delete m_scanThread;
    m_scanThread = NULL;
    emit scanCompleted();
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
    if (index.row() < m_items.size()) {
        return true; // local file, always runnable
    }

    quint32 packageIndex = index.row() - m_items.size();
    const PackageRef& pkg(m_packages[packageIndex]);
    InstallRef ex = pkg->existingInstall();
    if (!ex.valid()) {
        return false; // not installed
    }

    return !ex->isDownloading();
}

bool AircraftItemModel::isCandidateAircraftPath(QString path)
{
    QStringList filters;
    filters << "*-set.xml";
    int dirCount = 0,
        setXmlCount = 0;

    QDir d(path);
    Q_FOREACH(QFileInfo child, d.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        QDir childDir(child.absoluteFilePath());
        ++dirCount;
        Q_FOREACH(QFileInfo xmlChild, childDir.entryInfoList(filters, QDir::Files)) {
            ++setXmlCount;
        }

        if ((setXmlCount > 0) || (dirCount > 10)) {
            break;
        }
    }

    return (setXmlCount > 0);
}


#include "AircraftModel.moc"

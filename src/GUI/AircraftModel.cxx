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

using namespace simgear::pkg;

AircraftItem::AircraftItem() :
    excluded(false),
    usesHeliports(false),
    usesSeaports(false)
{
    // oh for C++11 initialisers
    for (int i=0; i<4; ++i) ratings[i] = 0;
}

AircraftItem::AircraftItem(QDir dir, QString filePath) :
    excluded(false),
    usesHeliports(false),
    usesSeaports(false)
{
    for (int i=0; i<4; ++i) ratings[i] = 0;

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

    ds >> description >> longDescription >> authors >> variantOf;
    for (int i=0; i<4; ++i) ds >> ratings[i];
}

void AircraftItem::toDataStream(QDataStream& ds) const
{
    ds << path << pathModTime << excluded;
    if (excluded) {
        return;
    }

    ds << description << longDescription << authors << variantOf;
    for (int i=0; i<4; ++i) ds << ratings[i];
}

QPixmap AircraftItem::thumbnail() const
{
    if (m_thumbnail.isNull()) {
        QFileInfo info(path);
        QDir dir = info.dir();
        if (dir.exists("thumbnail.jpg")) {
            m_thumbnail.load(dir.filePath("thumbnail.jpg"));
            // resize to the standard size
            if (m_thumbnail.height() > STANDARD_THUMBNAIL_HEIGHT) {
                m_thumbnail = m_thumbnail.scaledToHeight(STANDARD_THUMBNAIL_HEIGHT);
            }
        }
    }

    return m_thumbnail;
}


static quint32 CACHE_VERSION = 5;

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

                    if (item->variantOf.isNull()) {
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
            qDebug() << "doing refresh of" << QString::fromStdString(aCatalog->url());
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
            pix = pix.scaledToHeight(STANDARD_THUMBNAIL_HEIGHT);
        }
        m_model->m_thumbnailPixmapCache.insert(QString::fromStdString(aThumbnailUrl),
                                               pix);

        // notify any affected items. Linear scan here avoids another map/dict
        // structure.
        PackageList::const_iterator it;
        int i = 0;

        for (it=m_model->m_packages.begin(); it != m_model->m_packages.end(); ++it, ++i) {
            const string_list& urls((*it)->thumbnailUrls());
            string_list::const_iterator cit = std::find(urls.begin(), urls.end(), aThumbnailUrl);
            if (cit != urls.end()) {
                QModelIndex mi(m_model->index(i + m_model->m_items.size()));
                m_model->dataChanged(mi, mi);
            }
        } // of packages iteration
    }

private:
    QModelIndex indexForPackage(const PackageRef& ref) const
    {
        PackageList::const_iterator it = std::find(m_model->m_packages.begin(),
                                                   m_model->m_packages.end(),
                                                   ref);
        if (it == m_model->m_packages.end()) {
            return QModelIndex();
        }

        size_t offset = it - m_model->m_packages.begin();
        return m_model->index(offset + m_model->m_items.size());
    }

    AircraftItemModel* m_model;
};

AircraftItemModel::AircraftItemModel(QObject* pr ) :
    QAbstractListModel(pr),
    m_scanThread(NULL),
    m_showOfficialHangarMessage(false)
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

void AircraftItemModel::setOfficialHangarMessageVisible(bool vis)
{
    if (m_showOfficialHangarMessage == vis) {
        return;
    }

    m_showOfficialHangarMessage = vis;

    if (vis) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_items.prepend(AircraftItemPtr(new AircraftItem));
        m_activeVariant.prepend(0);
        endInsertRows();
    } else {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_items.removeAt(0);
        m_activeVariant.removeAt(0);
        endRemoveRows();
    }
}

QModelIndex AircraftItemModel::officialHangarMessageIndex() const
{
    if (!m_showOfficialHangarMessage) {
        return QModelIndex();
    }

    return index(0);
}

void AircraftItemModel::scanDirs()
{
    abandonCurrentScan();

    int firstRow = (m_showOfficialHangarMessage ? 1 : 0);
    int numToRemove = m_items.size() - firstRow;
    int lastRow = firstRow + numToRemove - 1;

    beginRemoveRows(QModelIndex(), firstRow, lastRow);
    m_items.remove(firstRow, numToRemove);
    m_activeVariant.remove(firstRow, numToRemove);
    endRemoveRows();

    QStringList dirs = m_paths;

    Q_FOREACH(std::string ap, globals->get_aircraft_paths()) {
        dirs << QString::fromStdString(ap);
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
    int firstRow = m_items.size();
    int newSize = newPkgs.size();

    if (m_packages.size() != newPkgs.size()) {
        int oldSize = m_packages.size();
        if (newSize > oldSize) {
            // growing
            int firstNewRow = firstRow + oldSize;
            int lastNewRow = firstRow + newSize - 1;
            beginInsertRows(QModelIndex(), firstNewRow, lastNewRow);
            m_packages = newPkgs;
            m_packageVariant.resize(newSize);
            endInsertRows();
        } else {
            // shrinking
            int firstOldRow = firstRow + newSize;
            int lastOldRow = firstRow + oldSize - 1;
            beginRemoveRows(QModelIndex(), firstOldRow, lastOldRow);
            m_packages = newPkgs;
            m_packageVariant.resize(newSize);
            endRemoveRows();
        }
    } else {
        m_packages = newPkgs;
    }

    emit dataChanged(index(firstRow), index(firstRow + newSize - 1));
}

int AircraftItemModel::rowCount(const QModelIndex& parent) const
{
    return m_items.size() + m_packages.size();
}

QVariant AircraftItemModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (m_showOfficialHangarMessage) {
        if (row == 0) {
            if (role == AircraftPackageStatusRole) {
                return NoOfficialCatalogMessage;
            }

            return QVariant();
        }
    }

    if (row >= m_items.size()) {
        quint32 packageIndex = row - m_items.size();

        if (role == AircraftVariantRole) {
            return m_packageVariant.at(packageIndex);
        }

        const PackageRef& pkg(m_packages[packageIndex]);
        InstallRef ex = pkg->existingInstall();

        if (role == AircraftInstallPercentRole) {
            return ex.valid() ? ex->downloadedPercent() : 0;
        } else if (role == AircraftInstallDownloadedSizeRole) {
            return static_cast<quint64>(ex.valid() ? ex->downloadedBytes() : 0);
        }

        quint32 variantIndex = m_packageVariant.at(packageIndex);
        return dataFromPackage(pkg, variantIndex, role);
    } else {
        if (role == AircraftVariantRole) {
            return m_activeVariant.at(row);
        }

        quint32 variantIndex = m_activeVariant.at(row);
        const AircraftItemPtr item(m_items.at(row));
        return dataFromItem(item, variantIndex, role);
    }
}

QVariant AircraftItemModel::dataFromItem(AircraftItemPtr item, quint32 variantIndex, int role) const
{
    if (role == AircraftVariantCountRole) {
        return item->variants.count();
    }

    if (role == AircraftThumbnailCountRole) {
        QPixmap p = item->thumbnail();
        return p.isNull() ? 0 : 1;
    }

    if (role == AircraftThumbnailSizeRole) {
        return item->thumbnail().size();
    }

    if ((role >= AircraftVariantDescriptionRole) && (role < AircraftThumbnailRole)) {
        int variantIndex = role - AircraftVariantDescriptionRole;
        return item->variants.at(variantIndex)->description;
    }

    if (variantIndex) {
        if (variantIndex <= static_cast<quint32>(item->variants.count())) {
            // show the selected variant
            item = item->variants.at(variantIndex - 1);
        }
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
    } else if (role >= AircraftThumbnailRole) {
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

QVariant AircraftItemModel::dataFromPackage(const PackageRef& item, quint32 variantIndex, int role) const
{
    if (role == Qt::DecorationRole) {
        role = AircraftThumbnailRole; // use first thumbnail
    }

    if ((role >= AircraftVariantDescriptionRole) && (role < AircraftThumbnailRole)) {
        int variantIndex = role - AircraftVariantDescriptionRole;
        QString desc = QString::fromStdString(item->nameForVariant(variantIndex));
        if (desc.isEmpty()) {
            desc = tr("Missing description for: %1").arg(QString::fromStdString(item->id()));
        }
        return desc;
    }

    if (role == Qt::DisplayRole) {
        QString desc = QString::fromStdString(item->nameForVariant(variantIndex));
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
        return QString::fromStdString(item->variants()[variantIndex]);
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
        QPixmap pm = packageThumbnail(item, 0, false).value<QPixmap>();
        if (pm.isNull())
            return QSize(STANDARD_THUMBNAIL_WIDTH, STANDARD_THUMBNAIL_HEIGHT);
        return pm.size();
    } else if (role >= AircraftThumbnailRole) {
        return packageThumbnail(item , role - AircraftThumbnailRole);
    } else if (role == AircraftAuthorsRole) {
        SGPropertyNode* authors = item->properties()->getChild("author");
        if (authors) {
            return QString::fromStdString(authors->getStringValue());
        }
    } else if (role == AircraftLongDescriptionRole) {
        return QString::fromStdString(item->description()).simplified();
    } else if (role == AircraftPackageSizeRole) {
        return static_cast<int>(item->fileSizeBytes());
    } else if (role == AircraftURIRole) {
        return QUrl("package:" + QString::fromStdString(item->qualifiedVariantId(variantIndex)));
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

QVariant AircraftItemModel::packageThumbnail(PackageRef p, int index, bool download) const
{
    const string_list& thumbnails(p->thumbnailUrls());
    if (index >= static_cast<int>(thumbnails.size())) {
        return QVariant();
    }

    std::string thumbnailUrl = thumbnails.at(index);
    QString urlQString(QString::fromStdString(thumbnailUrl));
    if (m_thumbnailPixmapCache.contains(urlQString)) {
        // cache hit, easy
        return m_thumbnailPixmapCache.value(urlQString);
    }

// check the on-disk store. This relies on the order of thumbnails in the
// results of thumbnailUrls and thumbnails corresponding
    InstallRef ex = p->existingInstall();
    if (ex.valid()) {
        const string_list& thumbNames(p->thumbnails());
        if (!thumbNames.empty()) {
            SGPath path(ex->path());
            path.append(p->thumbnails()[index]);
            if (path.exists()) {
                QPixmap pix;
                pix.load(QString::fromStdString(path.utf8Str()));
                // resize to the standard size
                if (pix.height() > STANDARD_THUMBNAIL_HEIGHT) {
                    pix = pix.scaledToHeight(STANDARD_THUMBNAIL_HEIGHT);
                }
                m_thumbnailPixmapCache[urlQString] = pix;
                return pix;
            }
        } // of have thumbnail file names
    } // of have existing install

    if (download) {
        m_packageRoot->requestThumbnailData(thumbnailUrl);
    }
    return QVariant();
}

bool AircraftItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
  {
      int row = index.row();
      if (role == AircraftVariantRole) {
          if (row >= m_activeVariant.size()) {
              row -= m_activeVariant.size();
              m_packageVariant[row] = value.toInt();
          } else {
              m_activeVariant[row] = value.toInt();
          }

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
        m_activeVariant.append(0);
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

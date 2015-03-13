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

// Simgear
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>
#include <simgear/misc/sg_path.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>

// FlightGear
#include <Main/globals.hxx>

using namespace simgear::pkg;

AircraftItem::AircraftItem()
{
    // oh for C++11 initialisers
    for (int i=0; i<4; ++i) ratings[i] = 0;
}

AircraftItem::AircraftItem(QDir dir, QString filePath)
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

    description = sim->getStringValue("description");
    authors =  sim->getStringValue("author");

    if (sim->hasChild("rating")) {
        SGPropertyNode_ptr ratingsNode = sim->getNode("rating");
        ratings[0] = ratingsNode->getIntValue("FDM");
        ratings[1] = ratingsNode->getIntValue("systems");
        ratings[2] = ratingsNode->getIntValue("cockpit");
        ratings[3] = ratingsNode->getIntValue("model");

    }

    if (sim->hasChild("variant-of")) {
        variantOf = sim->getStringValue("variant-of");
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
    ds >> path >> description >> authors >> variantOf;
    for (int i=0; i<4; ++i) ds >> ratings[i];
    ds >> pathModTime;
}

void AircraftItem::toDataStream(QDataStream& ds) const
{
    ds << path << description << authors << variantOf;
    for (int i=0; i<4; ++i) ds << ratings[i];
    ds << pathModTime;
}

QPixmap AircraftItem::thumbnail() const
{
    if (m_thumbnail.isNull()) {
        QFileInfo info(path);
        QDir dir = info.dir();
        if (dir.exists("thumbnail.jpg")) {
            m_thumbnail.load(dir.filePath("thumbnail.jpg"));
            // resize to the standard size
            if (m_thumbnail.height() > 128) {
                m_thumbnail = m_thumbnail.scaledToHeight(128);
            }
        }
    }

    return m_thumbnail;
}


static int CACHE_VERSION = 2;

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
    QList<AircraftItem*> items()
    {
        QList<AircraftItem*> result;
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

             for (int i=0; i<count; ++i) {
                AircraftItem* item = new AircraftItem;
                item->fromDataStream(ds);

                QFileInfo finfo(item->path);
                if (!finfo.exists() || (finfo.lastModified() != item->pathModTime)) {
                    delete item;
                } else {
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

            Q_FOREACH(AircraftItem* item, m_nextCache.values()) {
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
            QMap<QString, AircraftItem*> baseAircraft;
            QList<AircraftItem*> variants;

            Q_FOREACH(QFileInfo xmlChild, childDir.entryInfoList(filters, QDir::Files)) {
                try {
                    QString absolutePath = xmlChild.absoluteFilePath();
                    AircraftItem* item = NULL;

                    if (m_cachedItems.contains(absolutePath)) {
                        item = m_cachedItems.value(absolutePath);
                    } else {
                        item = new AircraftItem(childDir, absolutePath);
                    }

                    m_nextCache[absolutePath] = item;

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
            Q_FOREACH(AircraftItem* item, variants) {
                if (!baseAircraft.contains(item->variantOf)) {
                    qWarning() << "can't find principal aircraft " << item->variantOf << " for variant:" << item->path;
                    delete item;
                    continue;
                }

                baseAircraft.value(item->variantOf)->variants.append(item);
            }

            // lock mutex while we modify the items array
            {
                QMutexLocker g(&m_lock);
                m_items.append(baseAircraft.values());
            }

            emit addedItems();
        } // of subdir iteration

        qDebug() << "scan of" << path << "took" << t.elapsed();
    }

    QMutex m_lock;
    QStringList m_dirs;
    QList<AircraftItem*> m_items;

    QMap<QString, AircraftItem* > m_cachedItems;
    QMap<QString, AircraftItem* > m_nextCache;

    bool m_done;
};

AircraftItemModel::AircraftItemModel(QObject* pr, simgear::pkg::RootRef& rootRef) :
    QAbstractListModel(pr),
    m_scanThread(NULL),
    m_packageRoot(rootRef)
{
}

AircraftItemModel::~AircraftItemModel()
{
    abandonCurrentScan();
}

void AircraftItemModel::setPaths(QStringList paths)
{
    m_paths = paths;
}

void AircraftItemModel::scanDirs()
{
    abandonCurrentScan();

    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    m_activeVariant.clear();
    endResetModel();

    QStringList dirs = m_paths;

    Q_FOREACH(std::string ap, globals->get_aircraft_paths()) {
        dirs << QString::fromStdString(ap);
    }

    SGPath rootAircraft(globals->get_fg_root());
    rootAircraft.append("Aircraft");
    dirs << QString::fromStdString(rootAircraft.str());

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

QVariant AircraftItemModel::data(const QModelIndex& index, int role) const
{
    if (role == AircraftVariantRole) {
        return m_activeVariant.at(index.row());
    }

    const AircraftItem* item(m_items.at(index.row()));
    quint32 variantIndex = m_activeVariant.at(index.row());
    return dataFromItem(item, variantIndex, role);
}

QVariant AircraftItemModel::dataFromItem(const AircraftItem* item, quint32 variantIndex, int role) const
{
    if (role == AircraftVariantCountRole) {
        return item->variants.count();
    }

    if (role == AircraftThumbnailCountRole) {
        QPixmap p = item->thumbnail();
        return p.isNull() ? 0 : 1;
    }

    if ((role >= AircraftVariantDescriptionRole) && (role < AircraftThumbnailRole)) {
        int variantIndex = role - AircraftVariantDescriptionRole;
        return item->variants.at(variantIndex)->description;
    }

    if (variantIndex) {
        if (variantIndex <= item->variants.count()) {
            // show the selected variant
            item = item->variants.at(variantIndex - 1);
        }
    }

    if (role == Qt::DisplayRole) {
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
    }

    return QVariant();
}

QVariant AircraftItemModel::dataFromPackage(const PackageRef& item, quint32 variantIndex, int role) const
{
    if (role == Qt::DisplayRole) {
        return QString::fromStdString(item->description());
    } else if (role == AircraftPathRole) {
        // can we return the theoretical path?
    } else if (role == AircraftPackageIdRole) {
        return QString::fromStdString(item->id());
    } else if (role == AircraftPackageStatusRole) {
        bool installed = item->isInstalled();
        if (installed) {
            InstallRef i = item->existingInstall();
            if (i->isDownloading()) {
                return PackageDownloading;
            }
            if (i->hasUpdate()) {
                return PackageUpdateAvailable;
            }

            return PackageInstalled;
        } else {
            return PackageNotInstalled;
        }
    }

    return QVariant();
}

bool AircraftItemModel::setData(const QModelIndex &index, const QVariant &value, int role)
  {
      if (role == AircraftVariantRole) {
          m_activeVariant[index.row()] = value.toInt();
          emit dataChanged(index, index);
          return true;
      }

      return false;
  }

QModelIndex AircraftItemModel::indexOfAircraftPath(QString path) const
{
    for (int row=0; row <m_items.size(); ++row) {
        const AircraftItem* item(m_items.at(row));
        if (item->path == path) {
            return index(row);
        }
    }

    return QModelIndex();
}

void AircraftItemModel::onScanResults()
{
    QList<AircraftItem*> newItems = m_scanThread->items();
    if (newItems.isEmpty())
        return;

    int firstRow = m_items.count();
    int lastRow = firstRow + newItems.count() - 1;
    beginInsertRows(QModelIndex(), firstRow, lastRow);
    m_items.append(newItems);

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
}

#include "AircraftModel.moc"

// Written by James Turner, started October 2017
//
// Copyright (C) 2017 James Turner <zakalawe@mac.com>
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

#include "LocalAircraftCache.hxx"

#include <QDir>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QDataStream>
#include <QMap>
#include <QSettings>
#include <QDebug>

#include <Main/globals.hxx>
#include <Include/version.h>

#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

static quint32 CACHE_VERSION = 8;

const int STANDARD_THUMBNAIL_HEIGHT = 128;
const int STANDARD_THUMBNAIL_WIDTH = 172;

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

                needsMaintenance |= (strcmp(tagName, "needs-maintenance") == 0);
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

    if (sim->hasChild("minimum-fg-version")) {
        minFGVersion = sim->getStringValue("minimum-fg-version");
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
    ds >> minFGVersion;
    ds >> needsMaintenance >> usesHeliports >> usesSeaports;
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
    ds << minFGVersion;
    ds << needsMaintenance << usesHeliports << usesSeaports;
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

QVariant AircraftItem::status(int variant)
{
    if (needsMaintenance) {
        return LocalAircraftCache::AircraftUnmaintained;
    }

    if (minFGVersion.isEmpty()) {
        return LocalAircraftCache::AircraftOk;
    }

    const int c = simgear::strutils::compare_versions(FLIGHTGEAR_VERSION,
                                                      minFGVersion.toStdString(), 2);
    return (c < 0) ? LocalAircraftCache::AircraftNeedsNewerSimulator
                   : LocalAircraftCache::AircraftOk;

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

std::unique_ptr<LocalAircraftCache> static_cacheInstance;

LocalAircraftCache* LocalAircraftCache::instance()
{
    if (!static_cacheInstance) {
        static_cacheInstance.reset(new LocalAircraftCache);
    }

    return static_cacheInstance.get();
}

LocalAircraftCache::LocalAircraftCache()
{

}

LocalAircraftCache::~LocalAircraftCache()
{
    abandonCurrentScan();

}

void LocalAircraftCache::setPaths(QStringList paths)
{
    m_paths = paths;
}

void LocalAircraftCache::scanDirs()
{
    abandonCurrentScan();

    QStringList dirs = m_paths;

    Q_FOREACH(SGPath ap, globals->get_aircraft_paths()) {
        dirs << QString::fromStdString(ap.utf8Str());
    }

    SGPath rootAircraft(globals->get_fg_root());
    rootAircraft.append("Aircraft");
    dirs << QString::fromStdString(rootAircraft.utf8Str());

    m_scanThread = new AircraftScanThread(dirs);
    connect(m_scanThread, &AircraftScanThread::finished, this,
            &LocalAircraftCache::onScanFinished);
    connect(m_scanThread, &AircraftScanThread::addedItems,
            this, &LocalAircraftCache::onScanResults);
    m_scanThread->start();

    emit scanStarted();
}

int LocalAircraftCache::itemCount() const
{
    return m_items.size();
}

AircraftItemPtr LocalAircraftCache::itemAt(int index) const
{
    return m_items.at(index);
}

int LocalAircraftCache::findIndexWithUri(QUrl aircraftUri) const
{
    QString path = aircraftUri.toLocalFile();
    for (int row=0; row < m_items.size(); ++row) {
        const AircraftItemPtr item(m_items.at(row));
        if (item->path == path) {
            return row;
        }

        // check variants too
        for (int vr=0; vr < item->variants.size(); ++vr) {
            if (item->variants.at(vr)->path == path) {
                return row;
            }
        }
    }

    return -1;
}

QVector<AircraftItemPtr> LocalAircraftCache::newestItems(int count)
{
    QVector<AircraftItemPtr> r;
    r.reserve(count);
    int total = m_items.size();
    for (int i = total - count; i < count; ++i) {
        r.push_back(m_items.at(i));
    }
    return r;
}

AircraftItemPtr LocalAircraftCache::findItemWithUri(QUrl aircraftUri) const
{
    int index = findIndexWithUri(aircraftUri);
    if (index >= 0) {
        return m_items.at(index);
    }

    return {};
}

void LocalAircraftCache::abandonCurrentScan()
{
    if (m_scanThread) {
        m_scanThread->setDone();
        m_scanThread->wait(1000);
        delete m_scanThread;
        m_scanThread = NULL;
    }
}


void LocalAircraftCache::onScanResults()
{
    QVector<AircraftItemPtr> newItems = m_scanThread->items();
    if (newItems.isEmpty())
        return;


    m_items+=newItems;
    emit addedItems(newItems.size());
}

void LocalAircraftCache::onScanFinished()
{
    delete m_scanThread;
    m_scanThread = nullptr;
    emit scanCompleted();
}

bool LocalAircraftCache::isCandidateAircraftPath(QString path)
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

#include "LocalAircraftCache.moc"

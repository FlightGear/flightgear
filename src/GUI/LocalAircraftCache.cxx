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

#include <cassert>

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

static quint32 CACHE_VERSION = 12;

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
	// trim the description to ensure the alphabetical sort
	// doesn't get thrown off by leading whitespace
	description = description.trimmed();
    authors = sim->getStringValue("author");

    if (sim->hasChild("rating")) {
        SGPropertyNode_ptr ratingsNode = sim->getNode("rating");
        for (int i=0; i< 4; ++i) {
            ratings[i] = LocalAircraftCache::ratingFromProperties(ratingsNode, i);
        }
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

                // and actually store the tags
                tags.push_back(QString::fromUtf8(tagName));
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

    homepageUrl = QUrl(QString::fromStdString(sim->getStringValue("urls/home-page")));
    supportUrl = QUrl(QString::fromStdString(sim->getStringValue("urls/support")));
    wikipediaUrl = QUrl(QString::fromStdString(sim->getStringValue("urls/wikipedia")));
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
    ds >> homepageUrl >> supportUrl >> wikipediaUrl;
    ds >> tags;
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
    ds << homepageUrl << supportUrl << wikipediaUrl;
    ds << tags;
}

int AircraftItem::indexOfVariant(QUrl uri) const
{
    const QString path = uri.toLocalFile();
    for (int i=0; i< variants.size(); ++i) {
        if (variants.at(i)->path == path) {
            return i;
        }
    }

    return -1;
}

QVariant AircraftItem::status(int variant)
{
    Q_UNUSED(variant)
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
        Q_ASSERT(m_items.empty());
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
            quint32 count = static_cast<quint32>(m_nextCache.count());
            ds << CACHE_VERSION << count;

            Q_FOREACH(AircraftItemPtr item, m_nextCache.values()) {
                item->toDataStream(ds);
            }
        }

        settings.setValue("aircraft-cache", cacheData);
    }

    void scanAircraftDir(QDir path)
    {
        //QTime t;
        //t.start();

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

                    // should we re-stat here? But we already did so when loading
                    // the cache and dropped any non-valid entries
                    if (m_cachedItems.contains(absolutePath)) {
                        item = m_cachedItems.value(absolutePath);
                    } else {
                        // scan the cached item
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
                } catch (sg_exception&) {
                    continue;
                }

                if (m_done) { // thread termination bail-out
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
            bool didAddItems = false;
            {
                QMutexLocker g(&m_lock);
                m_items+=(baseAircraft.values().toVector());
                didAddItems = !m_items.isEmpty();
            }

            if (didAddItems) {
                emit addedItems();
            }
        } // of subdir iteration

        //qInfo() << "scanning" << path.absolutePath() << "took" << t.elapsed();
    }

    QMutex m_lock;
    QStringList m_dirs;
    QVector<AircraftItemPtr> m_items;

    QMap<QString, AircraftItemPtr > m_cachedItems;
    QMap<QString, AircraftItemPtr > m_nextCache;

    bool m_done;
};

static std::unique_ptr<LocalAircraftCache> static_cacheInstance;

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
    if (paths == m_paths) {
        return;
    }

    m_items.clear();
    emit cleared();
    m_paths = paths;
}

void LocalAircraftCache::scanDirs()
{
    abandonCurrentScan();
    m_items.clear();

    QStringList dirs = m_paths;

    for (SGPath ap : globals->get_aircraft_paths()) {
        dirs << QString::fromStdString(ap.utf8Str());
    }

    SGPath rootAircraft = globals->get_fg_root() / "Aircraft";
    dirs << QString::fromStdString(rootAircraft.utf8Str());

    m_scanThread.reset(new AircraftScanThread(dirs));
    connect(m_scanThread.get(), &AircraftScanThread::finished, this,
            &LocalAircraftCache::onScanFinished);
    // force a queued connection here since we the scan thread object still
    // belongs to the same thread as us, and hence this would otherwise be
    // a direct connection
    connect(m_scanThread.get(), &AircraftScanThread::addedItems,
            this, &LocalAircraftCache::onScanResults,
            Qt::QueuedConnection);
    m_scanThread->start();

    emit scanStarted();
}

int LocalAircraftCache::itemCount() const
{
    return m_items.size();
}

QVector<AircraftItemPtr> LocalAircraftCache::allItems() const
{
    return m_items;
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

AircraftItemPtr LocalAircraftCache::primaryItemFor(AircraftItemPtr item) const
{
    if (!item || item->variantOf.isEmpty())
        return item;

    for (int row=0; row < m_items.size(); ++row) {
        const AircraftItemPtr p(m_items.at(row));
        if (p->baseName() == item->variantOf) {
            return p;
        }
    }

    return {};
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
        // don't fire onScanFinished when the thread ends
        disconnect(m_scanThread.get(), &AircraftScanThread::finished, this,
                &LocalAircraftCache::onScanFinished);

        m_scanThread->setDone();
        if (!m_scanThread->wait(2000)) {
            qWarning() << Q_FUNC_INFO << "scan thread failed to terminate";
        }
        m_scanThread.reset();
    }
}


void LocalAircraftCache::onScanResults()
{
    if (!m_scanThread) {
        return;
    }

    QVector<AircraftItemPtr> newItems = m_scanThread->items();
    if (newItems.isEmpty())
        return;

    m_items+=newItems;
    emit addedItems(newItems.size());
}

void LocalAircraftCache::onScanFinished()
{
    m_scanThread.reset();
    emit scanCompleted();
}

bool LocalAircraftCache::isCandidateAircraftPath(QString path)
{
    QStringList filters;
    filters << "*-set.xml";
    // count of child dirs, if we visited more than ten children without
    // finding a -set.xml file, let's assume we are done. This avoids an
    // exhaustive search of huge directory trees
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

int LocalAircraftCache::ratingFromProperties(SGPropertyNode* node, int ratingIndex)
{
    const char* names[] = {"FDM", "systems", "cockpit", "model"};
    assert((ratingIndex >= 0) && (ratingIndex < 4));
    return node->getIntValue(names[ratingIndex]);
}

#include "LocalAircraftCache.moc"

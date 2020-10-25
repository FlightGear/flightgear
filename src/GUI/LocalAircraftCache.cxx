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

#include <Include/version.h>
#include <Main/globals.hxx>
#include <Main/locale.hxx>
#include <Main/sentryIntegration.hxx>

#include <simgear/misc/ResourceManager.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/structure/exception.hxx>

static quint32 CACHE_VERSION = 13;

const std::vector<QByteArray> static_localizedStringTags = {"name", "desc"};

QDataStream& operator<<(QDataStream& ds, const AircraftItem::LocalizedStrings& ls)
{
    ds << ls.locale << ls.strings;
    return ds;
}

QDataStream& operator>>(QDataStream& ds, AircraftItem::LocalizedStrings& ls)
{
    ds >> ls.locale >> ls.strings;
    return ds;
}

bool AircraftItem::initFromFile(QDir dir, QString filePath)
{
    SGPropertyNode root;
    readProperties(filePath.toStdString(), &root);

    if (!root.hasChild("sim")) {
        qWarning() << "-set.xml has no <sim> element" << filePath;
        return false;
    }

    SGPropertyNode_ptr sim = root.getNode("sim");

    path = filePath;
    pathModTime = QFileInfo(path).lastModified();
    if (sim->getBoolValue("exclude-from-gui", false)) {
        excluded = true;
        return false;
    }

    LocalizedStrings ls;
    ls.locale = "en";
    ls.strings["name"] = QString::fromStdString(sim->getStringValue("description")).trimmed();
    authors = sim->getStringValue("author");

    if (sim->hasChild("rating")) {
        SGPropertyNode_ptr ratingsNode = sim->getNode("rating");
        for (int i=0; i< 4; ++i) {
            ratings[i] = LocalAircraftCache::ratingFromProperties(ratingsNode, i);
        }
    }

    if (sim->hasChild("long-description")) {
        // clean up any XML whitspace in the text.
        ls.strings["desc"] = QString(sim->getStringValue("long-description")).simplified();
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

    _localized.push_front(ls);
    readLocalizedStrings(sim);
    doLocalizeStrings();

    return true;
}

void AircraftItem::readLocalizedStrings(SGPropertyNode_ptr simNode)
{
    if (!simNode->hasChild("localized"))
        return;

    auto localeNode = simNode->getChild("localized");
    const auto num = localeNode->nChildren();
    for (int i = 0; i < num; i++) {
        const SGPropertyNode* c = localeNode->getChild(i);

        LocalizedStrings ls;
        ls.locale = QString::fromStdString(c->getNameString());
        if (c->hasChild("description")) {
            ls.strings["name"] = QString::fromStdString(c->getStringValue("description"));
        }
        if (c->hasChild("long-description")) {
            ls.strings["desc"] = QString::fromStdString(c->getStringValue("long-description")).simplified();
        }

        _localized.push_back(ls);
    }
}

void AircraftItem::doLocalizeStrings()
{
    // default strings are always at the front
    _currentStrings = _localized.front().strings;

    const auto lang = QString::fromStdString(globals->get_locale()->getPreferredLanguage());
    // find the matching locale
    auto it = std::find_if(_localized.begin(), _localized.end(), [lang](const LocalizedStrings& ls) {
        return ls.locale == lang;
    });

    if (it == _localized.end())
        return; // nothing else to do

    for (auto t : static_localizedStringTags) {
        if (it->strings.contains(t)) {
            // copy the value we found
            _currentStrings[t] = it->strings.value(t);
        }
    } // of strings iteration
}

QString AircraftItem::name() const
{
    return _currentStrings.value("name");
}

QString AircraftItem::description() const
{
    return _currentStrings.value("desc");
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

    ds >> authors >> variantOf >> isPrimary;
    for (int i=0; i<4; ++i) ds >> ratings[i];
    ds >> previews;
    ds >> thumbnailPath;
    ds >> minFGVersion;
    ds >> needsMaintenance >> usesHeliports >> usesSeaports;
    ds >> homepageUrl >> supportUrl >> wikipediaUrl;
    ds >> tags;
    ds >> _localized;

    doLocalizeStrings();
}

void AircraftItem::toDataStream(QDataStream& ds) const
{
    ds << path << pathModTime << excluded;
    if (excluded) {
        return;
    }

    ds << authors << variantOf << isPrimary;
    for (int i=0; i<4; ++i) ds << ratings[i];
    ds << previews;
    ds << thumbnailPath;
    ds << minFGVersion;
    ds << needsMaintenance << usesHeliports << usesSeaports;
    ds << homepageUrl << supportUrl << wikipediaUrl;
    ds << tags;
    ds << _localized;
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

namespace {

static std::unique_ptr<LocalAircraftCache> static_cacheInstance;


// ensure references to Aircraft/foo and <my-aircraft-dir>/foo are resolved. This happens when
// aircraft reference a path (probably to themselves) in their -set.xml
class ScanDirProvider : public simgear::ResourceProvider
{
public:
    ScanDirProvider() : simgear::ResourceProvider(simgear::ResourceManager::PRIORITY_NORMAL) {}

    ~ScanDirProvider() = default;

    SGPath resolve(const std::string& aResource, SGPath& aContext) const override
    {
        Q_UNUSED(aContext)

        SGPath ap = _currentAircraftPath / aResource;
        if (ap.exists())
            return ap;

        string_list pieces(sgPathBranchSplit(aResource));
        if ((pieces.size() < 3) || (pieces.front() != "Aircraft")) {
            return SGPath{}; // not an Aircraft path
        }

        const std::string res(aResource, 9); // resource path with 'Aircraft/' removed
        SGPath p = _currentScanPath / res;
        if (p.exists())
            return p;

        return SGPath{};
    }

    void setCurrentPath(const SGPath& p)
    {
        _currentScanPath = p;
    }

    void setCurrentAircraftPath(const SGPath& p)
    {
        _currentAircraftPath = p;
    }

private:
    SGPath _currentScanPath;
    SGPath _currentAircraftPath;
};

class OtherAircraftDirsProvider : public simgear::ResourceProvider
{
public:
    OtherAircraftDirsProvider() : simgear::ResourceProvider(simgear::ResourceManager::PRIORITY_NORMAL) {}

    ~OtherAircraftDirsProvider() = default;

    SGPath resolve(const std::string& aResource, SGPath& aContext) const override
    {
        if (aResource.find("Aircraft/") != 0) {
            return SGPath{}; // not an aircraft path
        }

        const std::string res(aResource, 9); // resource path with 'Aircraft/' removed

        Q_UNUSED(aContext)
        QStringList paths = LocalAircraftCache::instance()->paths();
        Q_FOREACH(auto p, paths) {
            const auto sp = SGPath::fromUtf8(p.toUtf8().toStdString()) / res;
           // qWarning() << "OADP: trying:" << QString::fromStdString(sp.utf8Str());
            if (sp.exists())
                return sp;
        }

        return SGPath{};
    }
};

class AircraftScanThread : public QThread
{
    Q_OBJECT
public:
    AircraftScanThread(QStringList dirsToScan) :
        m_dirs(dirsToScan),
        m_done(false)
    {
        auto rm = simgear::ResourceManager::instance();
        m_currentScanDir.reset(new ScanDirProvider);
        rm->addProvider(m_currentScanDir.get());
    }

    ~AircraftScanThread()
    {
        simgear::ResourceManager::instance()->removeProvider(m_currentScanDir.get());
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
    void run() override
    {
        readCache();

        // avoid filling up Sentry with many reports
        // from unmaintained aircraft. We'll still fail if soemeone tries
        // to use the aircraft, but that's 100x less common.
        flightgear::sentryThreadReportXMLErrors(false);

        Q_FOREACH(QString d, m_dirs) {
            const auto p = SGPath::fromUtf8(d.toUtf8().toStdString());
            m_currentScanDir->setCurrentPath(p);
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

            // ensure aircraft dir is available to simgear::ResourceProvider
            // otherwise some aircraft -set.xml includes fail
            const auto p = SGPath::fromUtf8(child.absoluteFilePath().toUtf8().toStdString());
            m_currentScanDir->setCurrentAircraftPath(p);

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
                        item = AircraftItemPtr(new AircraftItem);
                        bool ok = item->initFromFile(childDir, absolutePath);
                        if (!ok) {
                            continue;
                        }
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
                    qWarning() << "Problems occurred while parsing" << xmlChild.absoluteFilePath() << "(skipping)"
                               << "\n\t" << QString::fromStdString(e.what());
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
    std::unique_ptr<ScanDirProvider> m_currentScanDir;
};

} // of anonymous namespace


class LocalAircraftCache::AircraftCachePrivate
{
public:
    QStringList m_paths;
    std::unique_ptr<AircraftScanThread> m_scanThread;
    QVector<AircraftItemPtr> m_items;
    std::unique_ptr<OtherAircraftDirsProvider> m_otherDirsProvider;
};

LocalAircraftCache* LocalAircraftCache::instance()
{
    if (!static_cacheInstance) {
        static_cacheInstance.reset(new LocalAircraftCache);
    }

    return static_cacheInstance.get();
}

void LocalAircraftCache::reset()
{
    static_cacheInstance.reset();
}

LocalAircraftCache::LocalAircraftCache() :
    d(new AircraftCachePrivate)
{
    d->m_otherDirsProvider.reset(new OtherAircraftDirsProvider);
    auto rm = simgear::ResourceManager::instance();
    rm->addProvider(d->m_otherDirsProvider.get());
}

LocalAircraftCache::~LocalAircraftCache()
{
    abandonCurrentScan();
    if (simgear::ResourceManager::haveInstance()) {
        simgear::ResourceManager::instance()->removeProvider(d->m_otherDirsProvider.get());
    } else {
        // resource manager will already have destroyed the provider. Awkward
        // ownership model :(
        d->m_otherDirsProvider.release();
    }
}

void LocalAircraftCache::setPaths(QStringList paths)
{
    if (paths == d->m_paths) {
        return;
    }

    d->m_items.clear();
    emit cleared();
    d->m_paths = paths;
}

QStringList LocalAircraftCache::paths() const
{
    return d->m_paths;
}

void LocalAircraftCache::scanDirs()
{
    abandonCurrentScan();
    d->m_items.clear();

    QStringList dirs = d->m_paths;

    for (SGPath ap : globals->get_aircraft_paths()) {
        dirs << QString::fromStdString(ap.utf8Str());
    }

    SGPath rootAircraft = globals->get_fg_root() / "Aircraft";
    dirs << QString::fromStdString(rootAircraft.utf8Str());

    d->m_scanThread.reset(new AircraftScanThread(dirs));
    connect(d->m_scanThread.get(), &AircraftScanThread::finished, this,
            &LocalAircraftCache::onScanFinished);
    // force a queued connection here since we the scan thread object still
    // belongs to the same thread as us, and hence this would otherwise be
    // a direct connection
    connect(d->m_scanThread.get(), &AircraftScanThread::addedItems,
            this, &LocalAircraftCache::onScanResults,
            Qt::QueuedConnection);
    d->m_scanThread->start();

    emit scanStarted();
}

int LocalAircraftCache::itemCount() const
{
    return d->m_items.size();
}

QVector<AircraftItemPtr> LocalAircraftCache::allItems() const
{
    return d->m_items;
}

AircraftItemPtr LocalAircraftCache::itemAt(int index) const
{
    return d->m_items.at(index);
}

int LocalAircraftCache::findIndexWithUri(QUrl aircraftUri) const
{
    QString path = aircraftUri.toLocalFile();
    for (int row=0; row < d->m_items.size(); ++row) {
        const AircraftItemPtr item(d->m_items.at(row));
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

    for (int row=0; row < d->m_items.size(); ++row) {
        const AircraftItemPtr p(d->m_items.at(row));
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
        return d->m_items.at(index);
    }

    return {};
}

void LocalAircraftCache::abandonCurrentScan()
{
    if (d->m_scanThread) {
        // don't fire onScanFinished when the thread ends
        disconnect(d->m_scanThread.get(), &AircraftScanThread::finished, this,
                &LocalAircraftCache::onScanFinished);

        d->m_scanThread->setDone();
        if (!d->m_scanThread->wait(2000)) {
            qWarning() << Q_FUNC_INFO << "scan thread failed to terminate";
        }
        d->m_scanThread.reset();
    }
}


void LocalAircraftCache::onScanResults()
{
    if (!d->m_scanThread) {
        return;
    }

    QVector<AircraftItemPtr> newItems = d->m_scanThread->items();
    if (newItems.isEmpty())
        return;

    d->m_items+=newItems;
    emit addedItems(newItems.size());
}

void LocalAircraftCache::onScanFinished()
{
    d->m_scanThread.reset();
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

LocalAircraftCache::ParseSetXMLResult
LocalAircraftCache::readAircraftProperties(const SGPath &setPath, SGPropertyNode_ptr props)
{
    // it woudld be race-y to touch the reosurce provider while the scan thread is running
    // and our provider would confuse current-aircraft-dir lookups as well. Since we
    // can't do thread-specific reosurce providers, we just bail here.
    if (d->m_scanThread) {
        return ParseSetXMLResult::Retry;
    }

    auto rm = simgear::ResourceManager::instance();

    // ensure aircraft relative paths in the -set.xml parsing work

    std::unique_ptr<ScanDirProvider> dp{new ScanDirProvider};
    rm->addProvider(dp.get());

    // we want to know the aircraft directory the aircraft lives in. This might
    // be a manually added path (for local aircraft) or the install dir for the
    // hangar (for packaged aircraft). Becuase -set.xml files are always found at
    // /some/path/foobarAircraft/<aircraft-name>/some-set.xml, and the path we
    // we want here is /some/path/foodbarAircraft, we use dirPath twice, and this
    // works any kind of installed aircraft
    const SGPath aircraftDirPath = setPath.dirPath().dirPath();
    dp->setCurrentPath(aircraftDirPath);

    dp->setCurrentAircraftPath(setPath);

    ParseSetXMLResult result = ParseSetXMLResult::Failed;
    try {
        readProperties(setPath, props);
        result = ParseSetXMLResult::Ok;
    } catch (sg_exception& e) {
        // malformed include or XML
        qWarning() << "Problems occurred while parsing" << QString::fromStdString(setPath.utf8Str())
                   << "\n\t" << QString::fromStdString(e.getFormattedMessage());
    }

    rm->removeProvider(dp.get());
    return result;
}

#include "LocalAircraftCache.moc"

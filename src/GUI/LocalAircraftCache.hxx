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

#ifndef LOCALAIRCRAFTCACHE_HXX
#define LOCALAIRCRAFTCACHE_HXX

#include <memory>
#include <QObject>
#include <QPixmap>
#include <QDateTime>
#include <QUrl>
#include <QSharedPointer>
#include <QDir>
#include <QVariant>

class QDataStream;
struct AircraftItem;
class AircraftScanThread;

typedef QSharedPointer<AircraftItem> AircraftItemPtr;

struct AircraftItem
{
    AircraftItem();

    AircraftItem(QDir dir, QString filePath);

    // the file-name without -set.xml suffix
    QString baseName() const;

    void fromDataStream(QDataStream& ds);

    void toDataStream(QDataStream& ds) const;

    QPixmap thumbnail(bool loadIfRequired = true) const;

    int indexOfVariant(QUrl uri) const;

    bool excluded = false;
    QString path;
    QString description;
    QString longDescription;
    QString authors;
    int ratings[4] = {0, 0, 0, 0};
    QString variantOf;
    QDateTime pathModTime;
    QList<AircraftItemPtr> variants;
    bool usesHeliports = false;
    bool usesSeaports = false;
    QList<QUrl> previews;
    bool isPrimary = false;
    QString thumbnailPath;
    QString minFGVersion;
    bool needsMaintenance = false;

    QVariant status(int variant);
private:
    mutable QPixmap m_thumbnail;
};

class LocalAircraftCache : public QObject
{
    Q_OBJECT
public:
    ~LocalAircraftCache();

    static LocalAircraftCache* instance();


    void setPaths(QStringList paths);

    void scanDirs();


    /**
     * @helper to determine if a particular path is likely to contain
     * aircraft or not. Checks for -set.xml files one level down in the tree.
     *
     */
    static bool isCandidateAircraftPath(QString path);

    int itemCount() const;

    QVector<AircraftItemPtr> allItems() const;

    AircraftItemPtr itemAt(int index) const;

    AircraftItemPtr findItemWithUri(QUrl aircraftUri) const;
    int findIndexWithUri(QUrl aircraftUri) const;

    AircraftItemPtr primaryItemFor(AircraftItemPtr item) const;

    QVariant aircraftStatus(AircraftItemPtr item) const;

    enum AircraftStatus
    {
        AircraftOk = 0,
        AircraftUnmaintained,
        AircraftNeedsNewerSimulator,
        AircraftNeedsOlderSimulator // won't ever occur for the moment
    };

    enum PackageStatus {
        PackageNotInstalled = 0,
        PackageInstalled,
        PackageUpdateAvailable,
        PackageQueued,
        PackageDownloading,
        NotPackaged
    };

    Q_ENUMS(PackageStatus)
    Q_ENUMS(AircraftStatus)
signals:

    void scanStarted();
    void scanCompleted();

    void addedItems(int count);
public slots:

private slots:
    void onScanResults();

    void onScanFinished();

private:
    explicit LocalAircraftCache();

    void abandonCurrentScan();

    QStringList m_paths;
    std::unique_ptr<AircraftScanThread> m_scanThread;
    QVector<AircraftItemPtr> m_items;

};

#endif // LOCALAIRCRAFTCACHE_HXX

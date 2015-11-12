// AircraftModel.hxx - part of GUI launcher using Qt5
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

#ifndef FG_GUI_AIRCRAFT_MODEL
#define FG_GUI_AIRCRAFT_MODEL

#include <QAbstractListModel>
#include <QDateTime>
#include <QDir>
#include <QPixmap>
#include <QStringList>
#include <QSharedPointer>
#include <QUrl>

#include <simgear/package/Root.hxx>

const int AircraftPathRole = Qt::UserRole + 1;
const int AircraftAuthorsRole = Qt::UserRole + 2;
const int AircraftVariantRole = Qt::UserRole + 3;
const int AircraftVariantCountRole = Qt::UserRole + 4;
const int AircraftThumbnailCountRole = Qt::UserRole + 5;
const int AircraftPackageIdRole = Qt::UserRole + 6;
const int AircraftPackageStatusRole = Qt::UserRole + 7;
const int AircraftPackageProgressRole = Qt::UserRole + 8;
const int AircraftLongDescriptionRole = Qt::UserRole + 9;
const int AircraftHasRatingsRole = Qt::UserRole + 10;
const int AircraftInstallPercentRole = Qt::UserRole + 11;
const int AircraftPackageSizeRole = Qt::UserRole + 12;
const int AircraftInstallDownloadedSizeRole = Qt::UserRole + 13;
const int AircraftURIRole = Qt::UserRole + 14;
const int AircraftThumbnailSizeRole = Qt::UserRole + 15;

const int AircraftRatingRole = Qt::UserRole + 100;
const int AircraftVariantDescriptionRole = Qt::UserRole + 200;
const int AircraftThumbnailRole = Qt::UserRole + 300;

class AircraftScanThread;
class QDataStream;
class PackageDelegate;
struct AircraftItem;
typedef QSharedPointer<AircraftItem> AircraftItemPtr;

struct AircraftItem
{
    AircraftItem();

    AircraftItem(QDir dir, QString filePath);
    
    // the file-name without -set.xml suffix
    QString baseName() const;
    
    void fromDataStream(QDataStream& ds);

    void toDataStream(QDataStream& ds) const;

    QPixmap thumbnail() const;

    bool excluded;
    QString path;
    QString description;
    QString authors;
    int ratings[4];
    QString variantOf;
    QDateTime pathModTime;

    QList<AircraftItemPtr> variants;
private:
    mutable QPixmap m_thumbnail;
};


enum AircraftItemStatus {
    PackageNotInstalled = 0,
    PackageInstalled,
    PackageUpdateAvailable,
    PackageQueued,
    PackageDownloading
};

class AircraftItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AircraftItemModel(QObject* pr, const simgear::pkg::RootRef& root);

    ~AircraftItemModel();

    void setPaths(QStringList paths);

    void scanDirs();

    virtual int rowCount(const QModelIndex& parent) const;
    
    virtual QVariant data(const QModelIndex& index, int role) const;
    
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    /**
     * given a -set.xml path, return the corresponding model index, if one
     * exists.
     */

    QModelIndex indexOfAircraftURI(QUrl uri) const;
    
    /**
     * return if a given aircraft is ready to be run, or not. Aircraft which
     * are not installed, or are downloading, are not runnable.
     */
    bool isIndexRunnable(const QModelIndex& index) const;
    
signals:
    void aircraftInstallFailed(QModelIndex index, QString errorMessage);
    
    void aircraftInstallCompleted(QModelIndex index);
    
    void scanCompleted();

private slots:
    void onScanResults();
    
    void onScanFinished();

private:
    friend class PackageDelegate;

    QVariant dataFromItem(AircraftItemPtr item, quint32 variantIndex, int role) const;

    QVariant dataFromPackage(const simgear::pkg::PackageRef& item,
                             quint32 variantIndex, int role) const;

    QVariant packageThumbnail(simgear::pkg::PackageRef p, int index, bool download = true) const;
    
    void abandonCurrentScan();
    void refreshPackages();
    
    void installSucceeded(QModelIndex index);
    void installFailed(QModelIndex index, simgear::pkg::Delegate::StatusCode reason);
    
    QStringList m_paths;
    AircraftScanThread* m_scanThread;
    QVector<AircraftItemPtr> m_items;
    PackageDelegate* m_delegate;
    
    QVector<quint32> m_activeVariant;
    QVector<quint32> m_packageVariant;
    
    simgear::pkg::RootRef m_packageRoot;
    simgear::pkg::PackageList m_packages;
        
    mutable QHash<QString, QPixmap> m_thumbnailPixmapCache;
};

#endif // of FG_GUI_AIRCRAFT_MODEL

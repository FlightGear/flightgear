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

#include <simgear/package/Delegate.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>

const int AircraftPathRole = Qt::UserRole + 1;
const int AircraftAuthorsRole = Qt::UserRole + 2;
const int AircraftVariantRole = Qt::UserRole + 3;
const int AircraftVariantCountRole = Qt::UserRole + 4;
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
const int AircraftIsHelicopterRole = Qt::UserRole + 16;
const int AircraftIsSeaplaneRole = Qt::UserRole + 17;
const int AircraftPackageRefRole = Qt::UserRole + 19;
const int AircraftThumbnailRole = Qt::UserRole + 20;
const int AircraftPreviewsRole = Qt::UserRole + 21;

const int AircraftRatingRole = Qt::UserRole + 100;
const int AircraftVariantDescriptionRole = Qt::UserRole + 200;

class AircraftScanThread;
class QDataStream;
class PackageDelegate;
struct AircraftItem;
typedef QSharedPointer<AircraftItem> AircraftItemPtr;

Q_DECLARE_METATYPE(simgear::pkg::PackageRef)

struct AircraftItem
{
    AircraftItem();

    AircraftItem(QDir dir, QString filePath);
    
    // the file-name without -set.xml suffix
    QString baseName() const;
    
    void fromDataStream(QDataStream& ds);

    void toDataStream(QDataStream& ds) const;

    QPixmap thumbnail(bool loadIfRequired = true) const;

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
private:
    mutable QPixmap m_thumbnail;
};


enum AircraftItemStatus {
    PackageNotInstalled = 0,
    PackageInstalled,
    PackageUpdateAvailable,
    PackageQueued,
    PackageDownloading,
    MessageWidget
};

class AircraftItemModel : public QAbstractListModel
{
    Q_OBJECT
public:
    AircraftItemModel(QObject* pr);

    ~AircraftItemModel();

    void setPackageRoot(const simgear::pkg::RootRef& root);

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
     * ensure the appropriate variant index is active in the model, for the
     * corresponding aircraft URI
     */
    void selectVariantForAircraftURI(QUrl uri);

    /**
     * return if a given aircraft is ready to be run, or not. Aircraft which
     * are not installed, or are downloading, are not runnable.
     */
    bool isIndexRunnable(const QModelIndex& index) const;

    /**
     * Retrieve the display name for an aircraft specified by URI, without
     * changing the current variant state
     */
    QString nameForAircraftURI(QUrl uri) const;

    /**
     * should we show reserve index 0 for a message widget? Which is set
     * by the layer above via setIndexWidget
     */
    void setMessageWidgetVisible(bool vis);

    QModelIndex messageWidgetIndex() const;

    /**
     * @helper to determine if a particular path is likely to contain
     * aircraft or not. Checks for -set.xml files one level down in the tree.
     *
     */
    static bool isCandidateAircraftPath(QString path);
signals:
    void aircraftInstallFailed(QModelIndex index, QString errorMessage);
    
    void aircraftInstallCompleted(QModelIndex index);
    
    void scanCompleted();

    void packagesNeedUpdating(bool yes);
private slots:
    void onScanResults();
    
    void onScanFinished();

private:
    friend class PackageDelegate;

    /**
     * struct to record persistent state from the item-delegate, about the
     * currently visible variant, thumbnail and similar.
     */
    struct DelegateState
    {
        quint32 variant = 0;
        quint32 thumbnail = 0;
    };

    QVariant dataFromItem(AircraftItemPtr item, const DelegateState& state, int role) const;

    QVariant dataFromPackage(const simgear::pkg::PackageRef& item,
                             const DelegateState& state, int role) const;

    QVariant packageThumbnail(simgear::pkg::PackageRef p,
                              const DelegateState& state, bool download = true) const;

    QVariant packagePreviews(simgear::pkg::PackageRef p, const DelegateState &ds) const;

    void abandonCurrentScan();
    void refreshPackages();
    
    void installSucceeded(QModelIndex index);
    void installFailed(QModelIndex index, simgear::pkg::Delegate::StatusCode reason);
    
    QStringList m_paths;
    AircraftScanThread* m_scanThread = nullptr;
    QVector<AircraftItemPtr> m_items;
    PackageDelegate* m_delegate = nullptr;
    bool m_showMessageWidget = false;


    QVector<DelegateState> m_delegateStates;

    simgear::pkg::RootRef m_packageRoot;
    simgear::pkg::PackageList m_packages;
        
    mutable QHash<QString, QPixmap> m_downloadedPixmapCache;
};

#endif // of FG_GUI_AIRCRAFT_MODEL

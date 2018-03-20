// LauncherMainWindow.hxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started October 2015.
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

#ifndef LAUNCHER_MAIN_WINDOW_HXX
#define LAUNCHER_MAIN_WINDOW_HXX

#include <QMainWindow>
#include <QScopedPointer>
#include <QStringList>
#include <QModelIndex>
#include <QTimer>
#include <QUrl>
#include <QQuickWidget>

#include <simgear/package/Package.hxx>
#include <simgear/package/Catalog.hxx>

namespace Ui
{
    class Launcher;
}

class QModelIndex;
class AircraftProxyModel;
class AircraftItemModel;
class QCheckBox;
class CatalogListModel;
class QQmlEngine;
class LaunchConfig;
class ExtraSettingsSection;
class ViewCommandLinePage;
class MPServersModel;
class QQuickItem;
class QmlAircraftInfo;
class QStandardItemModel;
class RecentAircraftModel;
class RecentLocationsModel;

class LauncherMainWindow : public QMainWindow
{
    Q_OBJECT

    Q_PROPERTY(bool showNoOfficialHanger READ showNoOfficialHanger NOTIFY showNoOfficialHangarChanged)

    Q_PROPERTY(AircraftProxyModel* installedAircraftModel MEMBER m_installedAircraftModel CONSTANT)
    Q_PROPERTY(AircraftProxyModel* browseAircraftModel MEMBER m_browseAircraftModel CONSTANT)
    Q_PROPERTY(AircraftProxyModel* searchAircraftModel MEMBER m_aircraftSearchModel CONSTANT)

    Q_PROPERTY(AircraftItemModel* baseAircraftModel MEMBER m_aircraftModel CONSTANT)

    Q_PROPERTY(QUrl selectedAircraft READ selectedAircraft WRITE setSelectedAircraft NOTIFY selectedAircraftChanged)


    Q_PROPERTY(QmlAircraftInfo* selectedAircraftInfo READ selectedAircraftInfo NOTIFY selectedAircraftChanged)

    Q_PROPERTY(bool isSearchActive READ isSearchActive NOTIFY searchChanged)
    Q_PROPERTY(QString settingsSearchTerm READ settingsSearchTerm WRITE setSettingsSearchTerm NOTIFY searchChanged)

    Q_PROPERTY(QStringList settingsSummary READ settingsSummary WRITE setSettingsSummary NOTIFY summaryChanged)
    Q_PROPERTY(QStringList environmentSummary READ environmentSummary WRITE setEnvironmentSummary NOTIFY summaryChanged)

    Q_PROPERTY(QString locationDescription READ locationDescription NOTIFY summaryChanged)
    Q_PROPERTY(QStringList combinedSummary READ combinedSummary NOTIFY summaryChanged)

    Q_PROPERTY(QString versionString READ versionString CONSTANT)

    Q_PROPERTY(RecentAircraftModel* aircraftHistory READ aircraftHistory CONSTANT)
    Q_PROPERTY(RecentLocationsModel* locationHistory READ locationHistory CONSTANT)

public:
    LauncherMainWindow();
    virtual ~LauncherMainWindow();

    bool execInApp();

    bool wasRejected();

    Q_INVOKABLE bool validateMetarString(QString metar);

    Q_INVOKABLE void requestInstallUpdate(QUrl aircraftUri);

    Q_INVOKABLE void requestUninstall(QUrl aircraftUri);

    Q_INVOKABLE void requestInstallCancel(QUrl aircraftUri);

    Q_INVOKABLE void downloadDirChanged(QString path);

    Q_INVOKABLE void requestUpdateAllAircraft();

    Q_INVOKABLE void queryMPServers();

    bool showNoOfficialHanger() const;

    Q_INVOKABLE void officialCatalogAction(QString s);

    QUrl selectedAircraft() const;

    // work around the fact, that this is not available on QQuickItem until 5.7
    Q_INVOKABLE QPointF mapToGlobal(QQuickItem* item, const QPointF& pos) const;

    QmlAircraftInfo* selectedAircraftInfo() const;
    
    Q_INVOKABLE void restoreLocation(QVariant var);

    Q_INVOKABLE bool matchesSearch(QString term, QStringList keywords) const;

    bool isSearchActive() const;

    QString settingsSearchTerm() const
    {
        return m_settingsSearchTerm;
    }

    QStringList settingsSummary() const;

    QStringList environmentSummary() const;

    QStringList combinedSummary() const;

    QString locationDescription() const;

    QString versionString() const;

    RecentAircraftModel* aircraftHistory();

    RecentLocationsModel* locationHistory();

    Q_INVOKABLE void launchUrl(QUrl url);

    // list of QUrls containing the default splash images from FGData.
    // used on the summary screen
    Q_INVOKABLE QVariantList defaultSplashUrls() const;


    Q_INVOKABLE QString selectAircraftStateAutomatically();

public slots:
    void setSelectedAircraft(QUrl selectedAircraft);

    void setSettingsSearchTerm(QString settingsSearchTerm);

    void setSettingsSummary(QStringList settingsSummary);

    void setEnvironmentSummary(QStringList environmentSummary);

signals:
    void showNoOfficialHangarChanged();

    void selectedAircraftChanged(QUrl selectedAircraft);

    void searchChanged();

    void summaryChanged();

    /**
     * @brief requestSaveState - signal to request QML settings to save their
     * state to persistent storage
     */
    void requestSaveState();
protected:
    virtual void closeEvent(QCloseEvent *event) override;

private slots:
    // run is used when the launcher is invoked before the main app is
    // started
    void onRun();

    // apply is used in-app, where we must set properties and trigger
    // a reset; setting command line options won't help us.
    void onApply();

    void onQuit();

    void onSubsytemIdleTimeout();

    void onAircraftInstalledCompleted(QModelIndex index);
    void onAircraftInstallFailed(QModelIndex index, QString errorMessage);

    void onRestoreDefaults();
    void onViewCommandLine();

    void onClickToolboxButton();

    void setSceneryPaths();
    void onAircraftPathsChanged();
    void onChangeDataDir();
    void onQuickStatusChanged(QQuickWidget::Status status);
private:

    /**
     * Check if the passed index is the selected aircraft, and if so, refresh
     * the associated UI data
     */
    void maybeUpdateSelectedAircraft(QModelIndex index);
    void updateSelectedAircraft();

    void restoreSettings();
    void saveSettings();

    simgear::pkg::PackageRef packageForAircraftURI(QUrl uri) const;

    // need to wait after a model reset before restoring selection and
    // scrolling, to give the view time it seems.
    void delayedAircraftModelReset();

    bool shouldShowOfficialCatalogMessage() const;

    void collectAircraftArgs();
    void initQML();


    QScopedPointer<Ui::Launcher> m_ui;
    AircraftProxyModel* m_installedAircraftModel;
    AircraftItemModel* m_aircraftModel;
    AircraftProxyModel* m_aircraftSearchModel;
    AircraftProxyModel* m_browseAircraftModel;
    MPServersModel* m_serversModel = nullptr;

    QUrl m_selectedAircraft;
    QTimer* m_subsystemIdleTimer;
    bool m_inAppMode = false;
    bool m_runInApp = false;
    bool m_accepted = false;
    int m_ratingFilters[4] = {3, 3, 3, 3};
    QQmlEngine* m_qmlEngine = nullptr;
    LaunchConfig* m_config = nullptr;
    ViewCommandLinePage* m_viewCommandLinePage = nullptr;
    QmlAircraftInfo* m_selectedAircraftInfo = nullptr;
    QString m_settingsSearchTerm;
    QStringList m_settingsSummary,
    m_environmentSummary;
    RecentAircraftModel* m_aircraftHistory = nullptr;
    RecentLocationsModel* m_locationHistory = nullptr;
};

#endif // of LAUNCHER_MAIN_WINDOW_HXX

// LauncherController.hxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started March 2018.
//
// Copyright (C) 2018 James Turner <james@flightgear.org>
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

#ifndef LAUNCHERCONTROLLER_HXX
#define LAUNCHERCONTROLLER_HXX

#include <QObject>
#include <QUrl>
#include <QModelIndex>

#include <simgear/package/Package.hxx>
#include <simgear/package/Catalog.hxx>

// forward decls
class QTimer;
class QWindow;
class AircraftProxyModel;
class QmlAircraftInfo;
class RecentAircraftModel;
class RecentLocationsModel;
class MPServersModel;
class AircraftItemModel;
class QQuickItem;
class LaunchConfig;
class LocationController;

class LauncherController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(AircraftProxyModel* installedAircraftModel MEMBER m_installedAircraftModel CONSTANT)
    Q_PROPERTY(AircraftProxyModel* browseAircraftModel MEMBER m_browseAircraftModel CONSTANT)
    Q_PROPERTY(AircraftProxyModel* searchAircraftModel MEMBER m_aircraftSearchModel CONSTANT)

    Q_PROPERTY(AircraftItemModel* baseAircraftModel MEMBER m_aircraftModel CONSTANT)

    Q_PROPERTY(LocationController* location MEMBER m_location CONSTANT)

    Q_PROPERTY(MPServersModel* mpServersModel MEMBER m_serversModel CONSTANT)

    Q_PROPERTY(QUrl selectedAircraft READ selectedAircraft WRITE setSelectedAircraft NOTIFY selectedAircraftChanged)

    Q_PROPERTY(QmlAircraftInfo* selectedAircraftInfo READ selectedAircraftInfo NOTIFY selectedAircraftChanged)

    Q_PROPERTY(bool isSearchActive READ isSearchActive NOTIFY searchChanged)
    Q_PROPERTY(QString settingsSearchTerm READ settingsSearchTerm WRITE setSettingsSearchTerm NOTIFY searchChanged)

    Q_PROPERTY(QStringList settingsSummary READ settingsSummary WRITE setSettingsSummary NOTIFY summaryChanged)
    Q_PROPERTY(QStringList environmentSummary READ environmentSummary WRITE setEnvironmentSummary NOTIFY summaryChanged)

    Q_PROPERTY(QStringList combinedSummary READ combinedSummary NOTIFY summaryChanged)

    Q_PROPERTY(QString versionString READ versionString CONSTANT)

    Q_PROPERTY(RecentAircraftModel* aircraftHistory READ aircraftHistory CONSTANT)
    Q_PROPERTY(RecentLocationsModel* locationHistory READ locationHistory CONSTANT)

    Q_PROPERTY(bool canFly READ canFly NOTIFY canFlyChanged)

    Q_PROPERTY(AircraftType aircraftType READ aircraftType NOTIFY selectedAircraftChanged)
public:
    explicit LauncherController(QObject *parent, QWindow* win);

    void initQML();

    Q_INVOKABLE bool validateMetarString(QString metar);

    Q_INVOKABLE void requestInstallUpdate(QUrl aircraftUri);

    Q_INVOKABLE void requestUninstall(QUrl aircraftUri);

    Q_INVOKABLE void requestInstallCancel(QUrl aircraftUri);

    Q_INVOKABLE void downloadDirChanged(QString path);

    Q_INVOKABLE void requestUpdateAllAircraft();

    Q_INVOKABLE void queryMPServers();

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

    QString versionString() const;

    RecentAircraftModel* aircraftHistory();

    RecentLocationsModel* locationHistory();

    Q_INVOKABLE void launchUrl(QUrl url);

    // list of QUrls containing the default splash images from FGData.
    // used on the summary screen
    Q_INVOKABLE QVariantList defaultSplashUrls() const;


    Q_INVOKABLE QString selectAircraftStateAutomatically();

    LaunchConfig* config() const
    { return m_config; }

    void doRun();
    void doApply();

    bool canFly() const;

    AircraftItemModel* baseAircraftModel() const
    { return m_aircraftModel; }

    void restoreSettings();
    void saveSettings();

    LocationController* location() const
    { return m_location; }

    enum AircraftType
    {
        Airplane = 0,
        Seaplane,
        Helicopter,
        Airship
    };

    Q_ENUMS(AircraftType)

    AircraftType aircraftType() const
    { return m_aircraftType; }

	void setInAppMode();
	bool keepRunningInAppMode() const;
	bool inAppResult() const;
signals:

    void selectedAircraftChanged(QUrl selectedAircraft);

    void searchChanged();
    void summaryChanged();

    void canFlyChanged();

    /**
     * @brief requestSaveState - signal to request QML settings to save their
     * state to persistent storage
     */
    void requestSaveState();

    void viewCommandLine();

public slots:
    void setSelectedAircraft(QUrl selectedAircraft);

    void setSettingsSearchTerm(QString settingsSearchTerm);

    void setSettingsSummary(QStringList settingsSummary);

    void setEnvironmentSummary(QStringList environmentSummary);

    void fly();

private slots:

    void onAircraftInstalledCompleted(QModelIndex index);
    void onAircraftInstallFailed(QModelIndex index, QString errorMessage);

private:
    /**
     * Check if the passed index is the selected aircraft, and if so, refresh
     * the associated UI data
     */
    void maybeUpdateSelectedAircraft(QModelIndex index);
    void updateSelectedAircraft();


    simgear::pkg::PackageRef packageForAircraftURI(QUrl uri) const;

    // need to wait after a model reset before restoring selection and
    // scrolling, to give the view time it seems.
    void delayedAircraftModelReset();

    void collectAircraftArgs();

private:
    QWindow* m_window = nullptr;

    AircraftProxyModel* m_installedAircraftModel;
    AircraftItemModel* m_aircraftModel;
    AircraftProxyModel* m_aircraftSearchModel;
    AircraftProxyModel* m_browseAircraftModel;
    MPServersModel* m_serversModel = nullptr;
    LocationController* m_location = nullptr;

    QUrl m_selectedAircraft;
    AircraftType m_aircraftType = Airplane;
    int m_ratingFilters[4] = {3, 3, 3, 3};
    LaunchConfig* m_config = nullptr;
    QmlAircraftInfo* m_selectedAircraftInfo = nullptr;
    QString m_settingsSearchTerm;
    QStringList m_settingsSummary, m_environmentSummary;
    RecentAircraftModel* m_aircraftHistory = nullptr;
    RecentLocationsModel* m_locationHistory = nullptr;

    QTimer* m_subsystemIdleTimer = nullptr;

	bool m_inAppMode = false;
	bool m_keepRunningInAppMode = false;
	bool m_appModeResult = true;
};

#endif // LAUNCHERCONTROLLER_HXX

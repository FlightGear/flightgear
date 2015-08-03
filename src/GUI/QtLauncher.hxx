// QtLauncher.hxx - GUI launcher dialog using Qt5
//
// Written by James Turner, started December 2014.
//
// Copyright (C) 2014 James Turner <zakalawe@mac.com>
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

#ifndef FG_QTLAUNCHER_HXX
#define FG_QTLAUNCHER_HXX

#include <QDialog>
#include <QScopedPointer>
#include <QStringList>
#include <QModelIndex>
#include <QTimer>
#include <QUrl>


#include <Airports/airport.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Catalog.hxx>

namespace Ui
{
    class Launcher;
}

class AirportSearchModel;
class QModelIndex;
class AircraftProxyModel;
class AircraftItemModel;
class QCheckBox;
class CatalogListModel;

class QtLauncher : public QDialog
{
    Q_OBJECT
public:
    QtLauncher();
    virtual ~QtLauncher();

    static void initApp(int& argc, char** argv);

    static bool runLauncherDialog();

private slots:
    void onRun();
    void onQuit();

    void onSearchAirports();

    void onAirportChanged();

    void onAirportChoiceSelected(const QModelIndex& index);
    void onAircraftSelected(const QModelIndex& index);
    void onRequestPackageInstall(const QModelIndex& index);
    void onCancelDownload(const QModelIndex& index);
    
    void onPopupAirportHistory();
    void onPopupAircraftHistory();

    void onEditRatingsFilter();

    void updateAirportDescription();
    void updateSettingsSummary();

    void onAirportSearchComplete();

    void onRembrandtToggled(bool b);
    void onToggleTerrasync(bool enabled);

    void onSubsytemIdleTimeout();

    void onEditPaths();
    
    void onAirportDiagramClicked(FGRunwayRef rwy);

    void onAircraftInstalledCompleted(QModelIndex index);
    void onAircraftInstallFailed(QModelIndex index, QString errorMessage);
private:
    void setAirport(FGAirportRef ref);
    void updateSelectedAircraft();

    void restoreSettings();
    void saveSettings();
    
    QModelIndex proxyIndexForAircraftURI(QUrl uri) const;
    QModelIndex sourceIndexForAircraftURI(QUrl uri) const;

    void setEnableDisableOptionFromCheckbox(QCheckBox* cbox, QString name) const;

    simgear::pkg::PackageRef packageForAircraftURI(QUrl uri) const;
    
    QScopedPointer<Ui::Launcher> m_ui;
    AirportSearchModel* m_airportsModel;
    AircraftProxyModel* m_aircraftProxy;
    AircraftItemModel* m_aircraftModel;
    FGAirportRef m_selectedAirport;

    QUrl m_selectedAircraft;
    QList<QUrl> m_recentAircraft;
    QStringList m_recentAirports;
    QTimer* m_subsystemIdleTimer;

    int m_ratingFilters[4];
};

#endif // of FG_QTLAUNCHER_HXX

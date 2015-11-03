// QtLauncher_private.hxx - GUI launcher dialog using Qt5
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

#ifndef FG_QTLAUNCHER_PRIVATE_HXX
#define FG_QTLAUNCHER_PRIVATE_HXX

#include <QDialog>
#include <QScopedPointer>
#include <QStringList>
#include <QModelIndex>
#include <QTimer>
#include <QUrl>

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

class QtLauncher : public QDialog
{
    Q_OBJECT
public:
    QtLauncher();
    virtual ~QtLauncher();

    void setInAppMode();
private slots:
    // run is used when the launcher is invoked before the main app is
    // started
    void onRun();

    // apply is used in-app, where we must set properties and trigger
    // a reset; setting command line options won't help us.
    void onApply();
    
    void onQuit();


    void onAircraftSelected(const QModelIndex& index);
    void onRequestPackageInstall(const QModelIndex& index);
    void onCancelDownload(const QModelIndex& index);

    void onPopupAircraftHistory();

    void onEditRatingsFilter();

    void updateSettingsSummary();


    void onRembrandtToggled(bool b);
    void onToggleTerrasync(bool enabled);

    void onSubsytemIdleTimeout();

    void onEditPaths();


    void onAircraftInstalledCompleted(QModelIndex index);
    void onAircraftInstallFailed(QModelIndex index, QString errorMessage);
private:

    /**
     * Check if the passed index is the selected aircraft, and if so, refresh
     * the associated UI data
     */
    void maybeUpdateSelectedAircraft(QModelIndex index);
    void updateSelectedAircraft();

    void restoreSettings();
    void saveSettings();

    QModelIndex proxyIndexForAircraftURI(QUrl uri) const;
    QModelIndex sourceIndexForAircraftURI(QUrl uri) const;

    void setEnableDisableOptionFromCheckbox(QCheckBox* cbox, QString name) const;

    simgear::pkg::PackageRef packageForAircraftURI(QUrl uri) const;

    QScopedPointer<Ui::Launcher> m_ui;
    AircraftProxyModel* m_aircraftProxy;
    AircraftItemModel* m_aircraftModel;

    QUrl m_selectedAircraft;
    QList<QUrl> m_recentAircraft;
    QTimer* m_subsystemIdleTimer;
    bool m_inAppMode;

    int m_ratingFilters[4];
};

#endif // of FG_QTLAUNCHER_PRIVATE_HXX

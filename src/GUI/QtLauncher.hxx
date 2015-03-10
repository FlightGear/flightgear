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

#include <Airports/airport.hxx>

namespace Ui
{
    class Launcher;
}

class AirportSearchModel;
class QModelIndex;
class AircraftProxyModel;
class QCheckBox;

class QtLauncher : public QDialog
{
    Q_OBJECT
public:
    QtLauncher();
    virtual ~QtLauncher();

    static void initApp(int argc, char** argv);

    static bool runLauncherDialog();

private slots:
    void onRun();
    void onQuit();

    void onSearchAirports();

    void onAirportChanged();

    void onAirportChoiceSelected(const QModelIndex& index);
    void onAircraftSelected(const QModelIndex& index);

    void onPopupAirportHistory();
    void onPopupAircraftHistory();

    void onOpenCustomAircraftDir();

    void onEditRatingsFilter();

    void updateAirportDescription();
    void updateSettingsSummary();

    void onAirportSearchComplete();

    void onAddSceneryPath();
    void onRemoveSceneryPath();

    void onRembrandtToggled(bool b);

    void onSubsytemIdleTimeout();
private:
    void setAirport(FGAirportRef ref);
    void updateSelectedAircraft();

    void restoreSettings();
    void saveSettings();
    
    QModelIndex proxyIndexForAircraftPath(QString path) const;
    QModelIndex sourceIndexForAircraftPath(QString path) const;

    void setEnableDisableOptionFromCheckbox(QCheckBox* cbox, QString name) const;

    QScopedPointer<Ui::Launcher> m_ui;
    AirportSearchModel* m_airportsModel;
    AircraftProxyModel* m_aircraftProxy;

    FGAirportRef m_selectedAirport;

    QString m_selectedAircraft;
    QStringList m_recentAircraft,
        m_recentAirports;
    QString m_customAircraftDir;
    QTimer* m_subsystemIdleTimer;

    int m_ratingFilters[4];
};

#endif // of FG_QTLAUNCHER_HXX

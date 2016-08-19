// LocationWidget.hxx - GUI launcher dialog using Qt5
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

#ifndef LOCATIONWIDGET_H
#define LOCATIONWIDGET_H

#include <QWidget>

#include <QToolButton>

#include <Navaids/positioned.hxx>
#include <Airports/airports_fwd.hxx>

#include "QtLauncher_fwd.hxx"

namespace Ui {
    class LocationWidget;
}

class NavSearchModel;

class LocationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LocationWidget(QWidget *parent = 0);
    ~LocationWidget();

    void saveSettings();
    void restoreSettings();

    QString locationDescription() const;

    void setBaseLocation(FGPositionedRef ref);

    void setAircraftType(LauncherAircraftType ty);

    bool shouldStartPaused() const;

    void setLocationProperties();
Q_SIGNALS:
    void descriptionChanged(QString t);

private Q_SLOTS:
    void updateDescription();
    void onLocationChanged();
    void onOffsetDataChanged();
    void onHeadingChanged();
private:

    void onSearch();
    void onSearchResultSelected(const QModelIndex& index);
    void onSearchComplete();

    void onAirportRunwayClicked(FGRunwayRef rwy);
    void onAirportParkingClicked(FGParkingRef park);

    void onOffsetBearingTrueChanged(bool on);

    void addToRecent(FGPositionedRef pos);

    void onOffsetEnabledToggled(bool on);
    void onBackToSearch();
    void setNavRadioOption();
    void onShowHistory();

    void applyPositionOffset();

    Ui::LocationWidget *m_ui;

    NavSearchModel* m_searchModel;

    FGPositionedRef m_location;
    bool m_locationIsLatLon;
    SGGeod m_geodLocation;

    QToolButton* m_backButton;

    FGPositionedList m_recentLocations;
    LauncherAircraftType m_aircraftType;
};

#endif // LOCATIONWIDGET_H

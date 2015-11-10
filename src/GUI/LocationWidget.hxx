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

    bool shouldStartPaused() const;

    void setLocationOptions();
Q_SIGNALS:
    void descriptionChanged(QString t);

private Q_SLOTS:
    void updateDescription();

    void onLocationChanged();

    void onOffsetDataChanged();

private:

    void onSearch();


    void onSearchResultSelected(const QModelIndex& index);
    void onPopupHistory();
    void onSearchComplete();

    void onAirportDiagramClicked(FGRunwayRef rwy);
    void onOffsetBearingTrueChanged(bool on);


    Ui::LocationWidget *m_ui;

    NavSearchModel* m_searchModel;

    FGPositionedRef m_location;
    bool m_locationIsLatLon;
    SGGeod m_geodLocation;

    QVector<PositionedID> m_recentAirports;

    QToolButton* m_backButton;

    void onOffsetEnabledToggled(bool on);
    void onBackToSearch();
    void setNavRadioOption();
};

#endif // LOCATIONWIDGET_H

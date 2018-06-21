// LocationController.hxx - GUI launcher dialog using Qt5
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

#ifndef LOCATION_CONTROLLER_HXX
#define LOCATION_CONTROLLER_HXX

#include <QObjectList>

#include <Navaids/positioned.hxx>
#include <Airports/airports_fwd.hxx>

#include "QtLauncher_fwd.hxx"
#include "LaunchConfig.hxx"
#include "QmlPositioned.hxx"

class NavSearchModel;

class LocationController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString description READ description NOTIFY descriptionChanged)

    Q_PROPERTY(NavSearchModel* searchModel MEMBER m_searchModel CONSTANT)

    Q_PROPERTY(QList<QObject*> airportRunways READ airportRunways NOTIFY baseLocationChanged)
    Q_PROPERTY(QList<QObject*> airportParkings READ airportParkings NOTIFY baseLocationChanged)

    Q_PROPERTY(bool offsetEnabled READ offsetEnabled WRITE setOffsetEnabled NOTIFY offsetChanged)
    Q_PROPERTY(int offsetRadial READ offsetRadial WRITE setOffsetRadial NOTIFY offsetChanged)
    Q_PROPERTY(bool offsetBearingIsTrue MEMBER m_offsetBearingIsTrue NOTIFY offsetChanged)
    Q_PROPERTY(double offsetNm READ offsetNm WRITE setOffsetNm NOTIFY offsetChanged)

    Q_PROPERTY(bool headingEnabled MEMBER m_headingEnabled NOTIFY configChanged)
    Q_PROPERTY(bool speedEnabled MEMBER m_speedEnabled NOTIFY configChanged)

    Q_PROPERTY(AltitudeType altitudeType MEMBER m_altitudeType NOTIFY configChanged)

    Q_PROPERTY(int headingDeg MEMBER m_headingDeg NOTIFY configChanged)
    Q_PROPERTY(int altitudeFt MEMBER m_altitudeFt NOTIFY configChanged)
    Q_PROPERTY(int flightLevel MEMBER m_flightLevel NOTIFY configChanged)

    Q_PROPERTY(int airspeedKnots MEMBER m_airspeedKnots NOTIFY configChanged)
    Q_PROPERTY(bool onFinal READ onFinal WRITE setOnFinal NOTIFY configChanged)

    Q_PROPERTY(bool isAirportLocation READ isAirportLocation NOTIFY baseLocationChanged)
    Q_PROPERTY(bool useActiveRunway READ useActiveRunway WRITE setUseActiveRunway NOTIFY configChanged)
    Q_PROPERTY(bool useAvailableParking READ useAvailableParking WRITE setUseAvailableParking NOTIFY configChanged)

    Q_PROPERTY(bool tuneNAV1 READ tuneNAV1 WRITE setTuneNAV1 NOTIFY configChanged)
    Q_PROPERTY(QmlGeod baseGeod READ baseGeod WRITE setBaseGeod NOTIFY baseLocationChanged)

    Q_PROPERTY(QmlPositioned* detail READ detail CONSTANT)
public:
    explicit LocationController(QObject *parent = nullptr);
    ~LocationController();

    enum AltitudeType
    {
        Off = 0,
        MSL_Feet,
        AGL_Feet,
        FlightLevel
    };

    Q_ENUMS(AltitudeType)

    void setLaunchConfig(LaunchConfig* config);

    QString description() const;

    void setBaseLocation(FGPositionedRef ref);

    bool shouldStartPaused() const;

    void setLocationProperties();

    void restoreLocation(QVariantMap l);
    QVariantMap saveLocation() const;

    void restoreSettings();

    /// used to automatically select aircraft state
    bool isParkedLocation() const;

    /// used to automatically select aircraft state
    bool isAirborneLocation() const;

    int offsetRadial() const;

    double offsetNm() const
    {
        return m_offsetNm;
    }

    Q_INVOKABLE void setBaseLocation(QmlPositioned* pos);

    Q_INVOKABLE void setDetailLocation(QmlPositioned* pos);

    QmlGeod baseGeod() const;
    void setBaseGeod(QmlGeod geod);

    bool isAirportLocation() const;

    bool offsetEnabled() const
    {
        return m_offsetEnabled;
    }

    bool onFinal() const
    {
        return m_onFinal;
    }

    void setUseActiveRunway(bool b);

    bool useActiveRunway() const
    {
        return m_useActiveRunway;
    }

    Q_INVOKABLE void addToRecent(QmlPositioned* pos);

    QObjectList airportRunways() const;
    QObjectList airportParkings() const;

    Q_INVOKABLE void showHistoryInSearchModel();

    Q_INVOKABLE QmlGeod parseStringAsGeod(QString string) const;

    bool tuneNAV1() const
    {
        return m_tuneNAV1;
    }

    QmlPositioned* detail() const;

    bool useAvailableParking() const
    {
        return m_useAvailableParking;
    }

public slots:
    void setOffsetRadial(int offsetRadial);

    void setOffsetNm(double offsetNm);

    void setOffsetEnabled(bool offsetEnabled);

    void setOnFinal(bool onFinal);

    void setTuneNAV1(bool tuneNAV1);

    void setUseAvailableParking(bool useAvailableParking);

Q_SIGNALS:
    void descriptionChanged();
    void offsetChanged();
    void baseLocationChanged();
    void configChanged();

private Q_SLOTS:
    void onCollectConfig();
private:

    void onSearchComplete();

    void onAirportRunwayClicked(FGRunwayRef rwy);
    void onAirportParkingClicked(FGParkingRef park);


    void addToRecent(FGPositionedRef pos);

    void setNavRadioOption();

    void applyPositionOffset();

    NavSearchModel* m_searchModel = nullptr;

    FGPositionedRef m_location;
    FGAirportRef m_airportLocation; // valid if m_location is an FGAirport
    FGPositionedRef m_detailLocation; // parking stand or runway detail
    bool m_locationIsLatLon = false;
    SGGeod m_geodLocation;

    FGPositionedList m_recentLocations;
    LaunchConfig* m_config = nullptr;
    QmlPositioned* m_detailQml = nullptr;

    bool m_offsetEnabled = false;
    int m_offsetRadial = 0;
    double m_offsetNm = 0.0;
    bool m_offsetBearingIsTrue = false;
    int m_headingDeg = 0;
    int m_altitudeFt= 0;
    int m_airspeedKnots = 150;
    bool m_onFinal = false;
    bool m_useActiveRunway = true;
    bool m_tuneNAV1 = false;
    bool m_useAvailableParking;
    bool m_headingEnabled = false;
    bool m_speedEnabled = false;
    AltitudeType m_altitudeType = Off;
    int m_flightLevel = 0;
};

#endif // LOCATION_CONTROLLER_HXX

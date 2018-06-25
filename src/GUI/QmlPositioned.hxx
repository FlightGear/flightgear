// QmlPositioned.hxx - Expose NavData to Qml
//
// Written by James Turner, started April 2018.
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

#ifndef QMLPOSITIONED_HXX
#define QMLPOSITIONED_HXX

#include <QObject>

#include <Navaids/positioned.hxx>

/**
 * @brief Expose an SGGeod as Qml-friendly class
 */
class QmlGeod
{
    Q_GADGET


    Q_PROPERTY(double latitudeDeg READ latitudeDeg WRITE setLatitudeDeg)
    Q_PROPERTY(double longitudeDeg READ longitudeDeg WRITE setLongitudeDeg)

    Q_PROPERTY(double latitudeRad READ latitudeRad WRITE setLatitudeRad)
    Q_PROPERTY(double longitudeRad READ longitudeRad WRITE setLongitudeRad)

    Q_PROPERTY(double elevationM READ elevationM WRITE setElevationM)
    Q_PROPERTY(double elevationFt READ elevationFt WRITE setElevationFt)

    Q_PROPERTY(bool valid READ valid)
public:
    QmlGeod();
    QmlGeod(const SGGeod& geod);

    SGGeod geod() const
    { return m_data; }


    double latitudeDeg() const;
    double longitudeDeg() const;
    double latitudeRad() const;
    double longitudeRad() const;
    double elevationM() const;
    double elevationFt() const;

    bool valid() const;

    enum Format {
        DecimalDegrees = 0,
        SignedDecimalDegrees
    };

    Q_INVOKABLE QString toString(Format fmt) const;

    Q_ENUMS(Format);

public slots:
    void setLatitudeDeg(double latitudeDeg);
    void setLongitudeDeg(double longitudeDeg);
    void setLatitudeRad(double latitudeRad);
    void setLongitudeRad(double longitudeRad);
    void setElevationM(double elevationM);
    void setElevationFt(double elevationFt);

private:
    SGGeod m_data;
};


Q_DECLARE_METATYPE(QmlGeod)

class QmlPositioned : public QObject
{
    Q_OBJECT
public:
    explicit QmlPositioned(QObject *parent = nullptr);

    explicit QmlPositioned(FGPositionedRef p);

    // proxy FGPositioned type values
    enum Type {
        Invalid = FGPositioned::INVALID,
        Airport =  FGPositioned::AIRPORT,
        Heliport = FGPositioned::HELIPORT,
        Seaport = FGPositioned::SEAPORT,
        Runway = FGPositioned::RUNWAY,
        Helipad = FGPositioned::HELIPAD,
        Taxiway = FGPositioned::TAXIWAY,
        Pavement = FGPositioned::PAVEMENT,
        Waypoint = FGPositioned::WAYPOINT,
        Fix = FGPositioned::FIX,
        NDB = FGPositioned::NDB,
        VOR = FGPositioned::VOR,
        ILS = FGPositioned::ILS,
        Localizer = FGPositioned::LOC,
        Glideslope = FGPositioned::GS,
        OuterMarker = FGPositioned::OM,
        MiddleMarker = FGPositioned::MM,
        InnerMarker = FGPositioned::IM,
        DME = FGPositioned::DME,
        TACAN = FGPositioned::TACAN,
        MobileTACAN = FGPositioned::MOBILE_TACAN,
        Tower = FGPositioned::TOWER,
        Parking = FGPositioned::PARKING,
        Country = FGPositioned::COUNTRY,
        City = FGPositioned::CITY,
        Town = FGPositioned::TOWN,
        Village = FGPositioned::VILLAGE
    };

    Q_ENUMS(Type)

    Q_PROPERTY(QString ident READ ident NOTIFY infoChanged)
    Q_PROPERTY(QString name READ name NOTIFY infoChanged)
    Q_PROPERTY(Type type READ type NOTIFY infoChanged)

    Q_PROPERTY(bool valid READ valid NOTIFY infoChanged)
    Q_PROPERTY(QmlGeod* geod READ geod NOTIFY infoChanged)
    Q_PROPERTY(qlonglong guid READ guid WRITE setGuid NOTIFY guidChanged)

    Q_PROPERTY(bool isAirportType READ isAirportType NOTIFY infoChanged)
    Q_PROPERTY(bool isRunwayType READ isRunwayType NOTIFY infoChanged)
    Q_PROPERTY(bool isNavaidType READ isNavaidType NOTIFY infoChanged)

    Q_PROPERTY(double navaidFrequencyMHz READ navaidFrequencyMHz NOTIFY infoChanged)
    Q_PROPERTY(double navaidRangeNm READ navaidRangeNm NOTIFY infoChanged)

    Q_PROPERTY(QmlPositioned* colocatedDME READ colocatedDME NOTIFY infoChanged)
    Q_PROPERTY(QmlPositioned* navaidRunway READ navaidRunway NOTIFY infoChanged)
    Q_PROPERTY(QmlPositioned* owningAirport READ owningAirport NOTIFY infoChanged)

    Q_PROPERTY(double runwayHeadingDeg READ runwayHeadingDeg NOTIFY infoChanged)
    Q_PROPERTY(double runwayLengthFt READ runwayLengthFt NOTIFY infoChanged)

    Q_PROPERTY(bool airportHasParkings READ airportHasParkings NOTIFY infoChanged)

    void setInner(FGPositionedRef p);
    FGPositionedRef inner() const;
    bool valid() const;

    QString ident() const;
    QString name() const;
    qlonglong guid() const;
    Type type() const;

    bool isAirportType() const;
    bool isRunwayType() const;
    bool isNavaidType() const;

    QmlGeod* geod() const;

    double navaidFrequencyMHz() const;
    double navaidRangeNm() const;

    QmlPositioned* navaidRunway() const;
    QmlPositioned* colocatedDME() const;

    // owning airport if one exists
    QmlPositioned* owningAirport() const;

    double runwayHeadingDeg() const;
    double runwayLengthFt() const;

    Q_INVOKABLE bool equals(QmlPositioned* other) const;

    bool airportHasParkings() const;

public slots:
    void setGuid(qlonglong guid);

signals:
    void guidChanged();
    void infoChanged();

private:
    FGPositionedRef m_pos;
};

bool operator==(const QmlPositioned& p1, const QmlPositioned& p2);

#endif // QMLPOSITIONED_HXX

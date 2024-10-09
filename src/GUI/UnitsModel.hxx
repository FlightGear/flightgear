// UnitsModel.cxx - part of GUI launcher using Qt5
//
// Written by James Turner, started July 2018
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

#ifndef UNITSMODEL_HXX
#define UNITSMODEL_HXX

#include <vector>

#include <QAbstractListModel>
#include <QValidator>

class QDataStream;

class Units : public QObject
{
    Q_OBJECT
public:


    /**
     * @brief This enum stores units / types of values used in the
     * simulator. They're not strictly all units, but they map to the
     * same concept for the user: selecting the dimension of values they
     * are inputting
     */
    enum Type
    {
        NoUnits = 0,
        FeetMSL,
        FeetAGL,
        FeetAboveFieldElevation,
        FlightLevel,
        MetersMSL,
        Knots,
        Mach,
        KilometersPerHour,
        DegreesTrue,
        DegreesMagnetic,
        TimeUTC,
        TimeLocal,
        NauticalMiles,
        Kilometers,
        FreqMHz,    // for VORs, LOCs
        FreqKHz     // for NDBs
    };

    enum Mode
    {
        Altitude = 0,   // MSL, FlightLevel
        AltitudeIncludingAGL,
        AltitudeIncludingMeters,
        Speed,  // Mach or knots or KM/H
        SpeedWithoutMach = 4,
        Heading, // degrees true or magnetic
        Timezone,
        Distance = 7, // Nm or Kilometers only for now
        AltitudeIncludingMetersAndAboveField,
        HeadingOnlyTrue
    };

    Q_ENUMS(Mode)
    Q_ENUMS(Type)
};


class QuantityValue
{
    Q_GADGET

    Q_PROPERTY(double value MEMBER value)
    Q_PROPERTY(Units::Type unit MEMBER unit)

public:
    QuantityValue();

    QuantityValue(Units::Type u, double v);

    QuantityValue(Units::Type u, int v);

    QuantityValue convertToUnit(Units::Type u) const;

    Q_INVOKABLE QuantityValue convertToUnit(int u) const;

    Q_INVOKABLE QString toString() const;

    Q_INVOKABLE bool isValid() const;

    // precision aware comparisom
    bool operator==(const QuantityValue& v) const;
    bool operator!=(const QuantityValue& v) const;

    double value = 0.0;
    Units::Type unit = Units::NoUnits;
};

QDataStream &operator<<(QDataStream &out, const QuantityValue &value);
QDataStream &operator>>(QDataStream &in, QuantityValue &value);

Q_DECLARE_METATYPE(QuantityValue)

class UnitsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(Units::Mode mode READ mode WRITE setMode NOTIFY modeChanged)

    Q_PROPERTY(int numChoices READ numChoices NOTIFY modeChanged)

    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectionChanged)
    Q_PROPERTY(int selectedUnit READ selectedUnit WRITE setSelectedUnit NOTIFY selectionChanged)

    Q_PROPERTY(double minValue READ minValue NOTIFY selectionChanged)
    Q_PROPERTY(double maxValue READ maxValue NOTIFY selectionChanged)
    Q_PROPERTY(double stepSize READ stepSize NOTIFY selectionChanged)
    Q_PROPERTY(int numDecimals READ numDecimals NOTIFY selectionChanged)
    Q_PROPERTY(QString maxTextForMetrics READ maxTextForMetrics NOTIFY selectionChanged)
    Q_PROPERTY(QString shortText READ shortText NOTIFY selectionChanged)
    Q_PROPERTY(bool isPrefix READ isPrefix NOTIFY selectionChanged)
    Q_PROPERTY(bool wraps READ doesWrap NOTIFY selectionChanged)

    Q_PROPERTY(QValidator* validator READ validator NOTIFY selectionChanged)
public:
    UnitsModel();

    using UnitVec = std::vector<Units::Type>;

    int rowCount(const QModelIndex &parent) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    Units::Mode mode() const
    {
        return m_mode;
    }

    int selectedIndex() const
    {
        return static_cast<int>(m_activeIndex);
    }

    double minValue() const;
    double maxValue() const;
    double stepSize() const;
    int numDecimals() const;
    QValidator *validator() const;
    QString maxTextForMetrics() const;
    bool isPrefix() const;
    bool doesWrap() const;

    QString shortText() const;
    Units::Type selectedUnit() const;
    int numChoices() const;

    Q_INVOKABLE bool isUnitInMode(int unit) const;
public slots:
    void setMode(Units::Mode mode);

    void setSelectedIndex(int selectedIndex);
    void setSelectedUnit(int u);

signals:
    void modeChanged(Units::Mode mode);

    void selectionChanged(int selectedIndex);

private:
    Units::Mode m_mode = Units::Altitude;
    quint32 m_activeIndex = 0;
    UnitVec m_enabledUnits;
};

#endif // UNITSMODEL_HXX

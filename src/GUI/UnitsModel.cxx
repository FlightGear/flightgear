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

#include "UnitsModel.hxx"

#include <cmath>
#include <cassert>
#include <algorithm>

#include <simgear/constants.h>

#include <QIntValidator>
#include <QDoubleValidator>
#include <QDataStream>
#include <QDebug>
#include <QGuiApplication>

namespace
{

struct UnitData
{
    UnitData(const char* sn, const char* ln, QString metrics, bool pfx = false) :
        shortName(sn), longName(ln),
        maxTextForMetrics(metrics),
        isPrefix(pfx) {}

    UnitData(const char* sn, const char* ln,
             QString metrics,
             bool pfx,
             double min, double max,
             double step = 1.0,
             bool wraps = false,
             int dps = 0) :
        shortName(sn), longName(ln),
        maxTextForMetrics(metrics),
        isPrefix(pfx),
        valueWraps(wraps),
        minValue(min), maxValue(max), stepSize(step),
        decimals(dps)
    {}

    const char* shortName;
    const char* longName;
    QString maxTextForMetrics;
    bool isPrefix = false;
    bool valueWraps = false;
    double minValue = 0.0;
    double maxValue = 9999999.0;
    double stepSize = 1.0;
    int decimals = 0;
};

std::vector<UnitData> static_unitData = {
    { "", "", "" }, // noUnits
    { QT_TRANSLATE_NOOP("UnitsModel", "ft"), QT_TRANSLATE_NOOP("UnitsModel", "feet above sea-level (MSL)"), "000000", false, -2000, 180000, 50},
    { QT_TRANSLATE_NOOP("UnitsModel", "ft AGL"),  QT_TRANSLATE_NOOP("UnitsModel", "feet above ground level (AGL)"), "000000", false, 0, 180000, 50},
    { QT_TRANSLATE_NOOP("UnitsModel", "ft above field"),  QT_TRANSLATE_NOOP("UnitsModel", "feet above airfield"), "000000", false, 0, 180000, 50},
    { QT_TRANSLATE_NOOP("UnitsModel", "FL"),  QT_TRANSLATE_NOOP("UnitsModel", "Flight-level"), "000", true /* prefix */, 0.0, 500.0, 5.0},
    { QT_TRANSLATE_NOOP("UnitsModel", "m"),  QT_TRANSLATE_NOOP("UnitsModel", "meters above sea-level (MSL)"), "000000", false, -500, 100000, 50},
    { QT_TRANSLATE_NOOP("UnitsModel", "kts"),  QT_TRANSLATE_NOOP("UnitsModel", "Knots"), "9999", false, 0, 999999, 10.0},
    { QT_TRANSLATE_NOOP("UnitsModel", "M"),  QT_TRANSLATE_NOOP("UnitsModel", "Mach"), "00.000", true /* prefix */, 0.0, 99.0, 0.05, false /* no wrap */, 3 /* decimal places */},
    { QT_TRANSLATE_NOOP("UnitsModel", "KM/H"),  QT_TRANSLATE_NOOP("UnitsModel", "Kilometers/hour"), "9999", false, 0, 999999, 10.0},
    { QT_TRANSLATE_NOOP("UnitsModel", "°True"),  QT_TRANSLATE_NOOP("UnitsModel", "degrees true"), "000", false, 0, 359, 5.0, true /* wraps */},
    { QT_TRANSLATE_NOOP("UnitsModel", "°Mag"),  QT_TRANSLATE_NOOP("UnitsModel", "degrees magnetic"), "000", false, 0, 359, 5.0, true /* wraps */},
    { QT_TRANSLATE_NOOP("UnitsModel", "UTC"),  QT_TRANSLATE_NOOP("UnitsModel", "Universal coordinated time"), ""},
    { QT_TRANSLATE_NOOP("UnitsModel", "Local"),  QT_TRANSLATE_NOOP("UnitsModel", "Local time"), ""},
    { QT_TRANSLATE_NOOP("UnitsModel", "Nm"),  QT_TRANSLATE_NOOP("UnitsModel", "Nautical miles"), "00000", false, 0, 999999, 1.0, false /* no wrap */, 1 /* decimal places */},
    { QT_TRANSLATE_NOOP("UnitsModel", "Km"),  QT_TRANSLATE_NOOP("UnitsModel", "Kilometers"), "00000", false, 0, 999999, 1.0, false /* no wrap */, 1 /* decimal places */},

    {  QT_TRANSLATE_NOOP("UnitsModel", "MHz"),  QT_TRANSLATE_NOOP("UnitsModel", "MHz"), "00000", false, 105, 140, 0.025, false /* no wrap */, 3 /* decimal places */},
    {  QT_TRANSLATE_NOOP("UnitsModel", "kHz"),  QT_TRANSLATE_NOOP("UnitsModel", "kHz"), "00000", false, 200, 400, 1.0, false /* no wrap */, 0 /* decimal places */}

};

// order here corresponds to the Mode enum
std::vector<UnitsModel::UnitVec> static_modeData = {
    { Units::FeetMSL, Units::FlightLevel},
    { Units::FeetMSL, Units::FeetAGL, Units::FlightLevel},
    { Units::FeetMSL, Units::MetersMSL, Units::FlightLevel},
    { Units::Knots, Units::Mach, Units::KilometersPerHour },
    { Units::Knots, Units::KilometersPerHour },
    { Units::DegreesMagnetic, Units::DegreesTrue },
    { Units::TimeLocal, Units::TimeUTC},
    { Units::NauticalMiles, Units::Kilometers },
    { Units::FeetMSL, Units::MetersMSL, Units::FeetAboveFieldElevation},
    { Units::DegreesTrue },
};

const int UnitLongNameRole = Qt::UserRole + 1;
const int UnitIsPrefixRole = Qt::UserRole + 2;
const int UnitMinValueRole = Qt::UserRole + 3;
const int UnitMaxValueRole = Qt::UserRole + 4;
const int UnitStepSizeRole = Qt::UserRole + 5;
const int UnitDecimalsRole = Qt::UserRole + 6;
const int UnitValueWrapsRole = Qt::UserRole + 7;

} // of anonymous namespace

UnitsModel::UnitsModel()
{


    m_enabledUnits = static_modeData.at(m_mode);
}

int UnitsModel::rowCount(const QModelIndex &) const
{
    return static_cast<int>(m_enabledUnits.size());
}

QVariant UnitsModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if ((row < 0) || (static_cast<size_t>(row) >= m_enabledUnits.size()))
        return {};

    const Units::Type u = m_enabledUnits.at(row);
    const UnitData& ud = static_unitData.at(u);

    switch (role) {
    case Qt::DisplayRole:
      return qApp->translate("UnitsModel", ud.shortName);

    case UnitLongNameRole:
      return qApp->translate("UnitsModel", ud.longName);

    case UnitIsPrefixRole: return ud.isPrefix;
    case UnitMinValueRole: return ud.minValue;
    case UnitMaxValueRole: return ud.maxValue;
    case UnitStepSizeRole: return ud.stepSize;
    case UnitValueWrapsRole: return ud.valueWraps;
    case UnitDecimalsRole: return ud.decimals;
    default:
        break;
    }

    return {};
}

QValidator* UnitsModel::validator() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    if (ud.decimals > 0) {
        return new QDoubleValidator(ud.minValue, ud.maxValue, ud.decimals);
    }

    if ((u == Units::TimeLocal) || (u == Units::TimeUTC)) {
        return nullptr; // no validation
    }

    return new QIntValidator(static_cast<int>(ud.minValue),
                             static_cast<int>(ud.maxValue));
}

QString UnitsModel::maxTextForMetrics() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return ud.maxTextForMetrics;
}

bool UnitsModel::isPrefix() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return ud.isPrefix;
}

bool UnitsModel::doesWrap() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return ud.valueWraps;
}

QString UnitsModel::shortText() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return qApp->translate("UnitsModel",ud.shortName);
}

Units::Type UnitsModel::selectedUnit() const
{
    return m_enabledUnits.at(m_activeIndex);
}

int UnitsModel::numChoices() const
{
    return static_cast<int>(m_enabledUnits.size());
}

bool UnitsModel::isUnitInMode(int unit) const
{
    auto it = std::find(m_enabledUnits.begin(), m_enabledUnits.end(), unit);
    return it != m_enabledUnits.end();
}

QHash<int, QByteArray> UnitsModel::roleNames() const
{
    QHash<int, QByteArray> result = QAbstractListModel::roleNames();

    result[Qt::DisplayRole] = "shortName";
    result[UnitLongNameRole] = "longName";
    result[UnitIsPrefixRole] = "isPrefix";
    result[UnitValueWrapsRole] = "valueDoesWrap";
    result[UnitMinValueRole] = "minValue";
    result[UnitMaxValueRole] = "maxValue";
    result[UnitStepSizeRole] = "stepSize";
    result[UnitDecimalsRole] = "decimalPlaces";

    return result;
}

int UnitsModel::numDecimals() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return ud.decimals;
}

double UnitsModel::minValue() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return ud.minValue;
}

double UnitsModel::maxValue() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return ud.maxValue;
}

double UnitsModel::stepSize() const
{
    const auto u = m_enabledUnits.at(m_activeIndex);
    const UnitData& ud = static_unitData.at(u);
    return ud.stepSize;
}

void UnitsModel::setMode(Units::Mode mode)
{
    if (m_mode == mode)
        return;

    m_mode = mode;
    emit modeChanged(m_mode);

    beginResetModel();
    m_enabledUnits = static_modeData.at(mode);
    endResetModel();
}

void UnitsModel::setSelectedIndex(int selectedIndex)
{
    if (m_activeIndex == static_cast<quint32>(selectedIndex))
        return;

    if ((selectedIndex < 0) || (static_cast<size_t>(selectedIndex) >= m_enabledUnits.size()))
        return;

    m_activeIndex = selectedIndex;
    emit selectionChanged(m_activeIndex);
}

void UnitsModel::setSelectedUnit(int u)
{
    auto it = std::find(m_enabledUnits.begin(), m_enabledUnits.end(), static_cast<Units::Type>(u));
    if (it == m_enabledUnits.end()) {
        qWarning() << Q_FUNC_INFO << "unit" << u << "not enabled for mode" << m_mode;
        return;
    }

    auto index = std::distance(m_enabledUnits.begin(), it);
    if (index != m_activeIndex) {
        m_activeIndex = static_cast<quint32>(index);
        emit selectionChanged(m_activeIndex);
    }
}

QuantityValue::QuantityValue()
{
}

QuantityValue::QuantityValue(Units::Type u, double v) :
    value(v),
    unit(u)
{
}

QuantityValue::QuantityValue(Units::Type u, int v) :
    value(static_cast<double>(v)),
    unit(u)
{
    assert(static_unitData.at(u).decimals == 0);
}

QuantityValue QuantityValue::convertToUnit(Units::Type u) const
{
    // special case a no-change
    if (unit == u)
        return *this;

    if (unit == Units::NoUnits) {
        return {u, 0.0};
    }

    switch (u) {
    case Units::NauticalMiles:
    {
        if (unit == Units::Kilometers) {
            return {u, value * SG_METER_TO_NM * 1000};
        }
        break;
    }

    case Units::Kilometers:
    {
        if (unit == Units::NauticalMiles) {
            return {u, value * SG_NM_TO_METER * 0.001};
        }
        break;
    }

    case Units::FeetMSL:
    {
        if (unit == Units::FlightLevel) {
            return {u, value * 100};
        } else if (unit == Units::MetersMSL) {
            return {u, value * SG_METER_TO_FEET};
        } else if ((unit == Units::FeetAGL) || (unit == Units::FeetAboveFieldElevation)) {
            return {u, value};
        }
        break;
    }

    case Units::FeetAboveFieldElevation:
    case Units::FeetAGL:
        // treat as FeetMSL
        return {u, convertToUnit(Units::FeetMSL).value};

    case Units::MetersMSL:
        return {u, convertToUnit(Units::FeetMSL).value * SG_FEET_TO_METER};

    case Units::Mach:
    {
        if (unit == Units::Knots) {
            // obviously this depends on altitude, let's
            // use the value at sea level for now
            return {u, value / 667.0};
        } else if (unit == Units::KilometersPerHour) {
            return convertToUnit(Units::Knots).convertToUnit(u);
        }
        break;
    }

    case Units::Knots:
    {
        if (unit == Units::Mach) {
            // obviously this depends on altitude, let's
            // use the value at sea level for now
            return {u, value * 667.0};
        } else if (unit == Units::KilometersPerHour) {
            return {u, value * SG_KMH_TO_MPS * SG_MPS_TO_KT};
        }
        break;
    }

    case Units::KilometersPerHour:
        if (unit == Units::Knots) {
            return {u, value * SG_KT_TO_MPS * SG_MPS_TO_KMH};
        } else {
            // round-trip via Knots
            return convertToUnit(Units::Knots).convertToUnit(u);
        }

    case Units::DegreesMagnetic:
    case Units::DegreesTrue:
    {
        // we don't have a location to apply mag-var, so just keep the
        // current value
        if ((unit == Units::DegreesMagnetic) || (unit == Units::DegreesTrue)) {
            return {u, value};
        }
        break;
    }

    case Units::FlightLevel:
        if (unit == Units::FeetMSL) {
            return {u, static_cast<double>(static_cast<int>(value / 100))};
        } else {
            return convertToUnit(Units::FeetMSL).convertToUnit(u);
        }

    default:
        qWarning() << Q_FUNC_INFO << "unhandled case:" << u << "from" << unit;
        break;
    }

    return {};
}

QuantityValue QuantityValue::convertToUnit(int u) const
{
    return convertToUnit(static_cast<Units::Type>(u));
}

QString QuantityValue::toString() const
{
    if (unit == Units::NoUnits)
        return "<no unit>";

    const auto& data = static_unitData.at(unit);
    int dp = data.decimals;
    QString prefix;
    QString suffix = qApp->translate("UnitsModel", data.shortName);
    if (data.isPrefix)
        std::swap(prefix, suffix);

    if (dp == 0) {
        return prefix + QString::number(static_cast<int>(value)) + suffix;
    }

    return prefix + QString::number(value, 'f', dp) + suffix;
}

bool QuantityValue::isValid() const
{
    return unit != Units::NoUnits;
}

bool QuantityValue::operator==(const QuantityValue &v) const
{
    if (v.unit != unit)
        return false;

    int dp = static_unitData.at(unit).decimals;
    const auto aInt = static_cast<qlonglong>(value * pow(10, dp));
    const auto bInt = static_cast<qlonglong>(v.value * pow(10, dp));
    return aInt == bInt;
}

bool QuantityValue::operator!=(const QuantityValue &v) const
{
    return !(*this == v);
}

QDataStream &operator<<(QDataStream &out, const QuantityValue &value)
{
    out << static_cast<quint8>(value.unit);
    if (value.unit != Units::NoUnits)
        out << value.value;
    return out;
}

QDataStream &operator>>(QDataStream &in, QuantityValue &value)
{
    quint8 unit;
    in >> unit;
    value.unit = static_cast<Units::Type>(unit);
    if (unit != Units::NoUnits)
        in >> value.value;
    return in;
}

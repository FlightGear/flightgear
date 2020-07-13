// QmlPositioned.cxx - Expose NavData to Qml
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

#include "QmlPositioned.hxx"

#include <QDebug>

#include <simgear/misc/strutils.hxx>

#include <Navaids/NavDataCache.hxx>
#include <Navaids/navrecord.hxx>
#include <Airports/airport.hxx>
#include <Airports/groundnetwork.hxx>
#include <Airports/runways.hxx>

using namespace flightgear;
using namespace simgear::strutils;

QmlGeod::QmlGeod() :
    m_data(SGGeod::fromDeg(-9999.0, -9999.0))
{
}

QmlGeod::QmlGeod(const SGGeod &geod) :
    m_data(geod)
{
}

double QmlGeod::latitudeDeg() const
{
    return m_data.getLatitudeDeg();
}

double QmlGeod::longitudeDeg() const
{
    return m_data.getLongitudeDeg();
}

double QmlGeod::latitudeRad() const
{
    return m_data.getLatitudeRad();
}

double QmlGeod::longitudeRad() const
{
    return m_data.getLongitudeRad();
}

double QmlGeod::elevationFt() const
{
    return m_data.getElevationFt();
}

bool QmlGeod::valid() const
{
    return m_data.isValid();
}

QString QmlGeod::toString(QmlGeod::Format fmt) const
{
    if (!m_data.isValid())
        return "<invalid coordinate>";

    LatLonFormat internalFormat = LatLonFormat::DECIMAL_DEGREES;
    switch (fmt) {
    case DecimalDegrees: internalFormat = LatLonFormat::DECIMAL_DEGREES; break;
    case SignedDecimalDegrees: internalFormat = LatLonFormat::SIGNED_DECIMAL_DEGREES; break;
    }

    const auto s = formatGeodAsString(m_data, internalFormat, DegreeSymbol::UTF8_DEGREE);
    return QString::fromStdString(s);
}

double QmlGeod::elevationM() const
{
    return m_data.getElevationM();
}

void QmlGeod::setLatitudeDeg(double latitudeDeg)
{
    if (qFuzzyCompare(m_data.getLatitudeDeg(), latitudeDeg))
        return;

    m_data.setLatitudeDeg(latitudeDeg);
  //  emit latitudeChanged();
}

void QmlGeod::setLongitudeDeg(double longitudeDeg)
{
    if (qFuzzyCompare(m_data.getLongitudeDeg(), longitudeDeg))
        return;

    m_data.setLongitudeDeg(longitudeDeg);
  //  emit longitudeChanged();
}

void QmlGeod::setLatitudeRad(double latitudeRad)
{
    if (qFuzzyCompare(m_data.getLatitudeRad(), latitudeRad))
        return;

    m_data.setLatitudeRad(latitudeRad);
   // emit latitudeChanged();
}

void QmlGeod::setLongitudeRad(double longitudeRad)
{
    if (qFuzzyCompare(m_data.getLongitudeRad(), longitudeRad))
        return;

    m_data.setLongitudeRad(longitudeRad);
   // emit longitudeChanged();
}

void QmlGeod::setElevationM(double elevationM)
{
    if (qFuzzyCompare(m_data.getElevationM(), elevationM))
        return;

    m_data.setElevationM(elevationM);
  //  emit elevationChanged();
}

void QmlGeod::setElevationFt(double elevationFt)
{
    if (qFuzzyCompare(m_data.getElevationFt(), elevationFt))
        return;

    m_data.setElevationFt(elevationFt);
   // emit elevationChanged();
}

/////////////////////////////////////////////////////////////////////////////////

QmlPositioned::QmlPositioned(QObject *parent) : QObject(parent)
{

}

QmlPositioned::QmlPositioned(FGPositionedRef p) :
    m_pos(p)
{
}

void QmlPositioned::setInner(FGPositionedRef p)
{
    if (!p) {
        m_pos.clear();
    } else {
        m_pos = p;
    }

    emit infoChanged();
}

FGPositionedRef QmlPositioned::inner() const
{
    return m_pos;
}

bool QmlPositioned::valid() const
{
    return m_pos.valid();
}

QString QmlPositioned::ident() const
{
    if (!m_pos)
        return {};

    return QString::fromStdString(m_pos->ident());
}

QString QmlPositioned::name() const
{
    if (!m_pos)
        return {};

    return QString::fromStdString(m_pos->name());
}

qlonglong QmlPositioned::guid() const
{
    if (!m_pos)
        return 0;

    return m_pos->guid();
}

QmlPositioned::Type QmlPositioned::type() const
{
    if (!m_pos)
        return Invalid;

    return static_cast<Type>(m_pos->type());
}

void QmlPositioned::setGuid(qlonglong guid)
{
    if (m_pos && (m_pos->guid()) == guid)
        return;

    m_pos = NavDataCache::instance()->loadById(guid);

    emit guidChanged();
    emit infoChanged();
}

bool QmlPositioned::isAirportType() const
{
    return FGPositioned::isAirportType(m_pos.get());
}

bool QmlPositioned::isRunwayType() const
{
    return FGPositioned::isRunwayType(m_pos.get());
}

bool QmlPositioned::isNavaidType() const
{
    return FGPositioned::isNavaidType(m_pos.get());
}

QmlGeod* QmlPositioned::geod() const
{
    if (!m_pos)
        return nullptr;

    return new QmlGeod(m_pos->geod());
}

double QmlPositioned::navaidFrequencyMHz() const
{
    FGNavRecord* nav = fgpositioned_cast<FGNavRecord>(m_pos);
    if (!nav)
        return 0.0;

    qWarning() << Q_FUNC_INFO << "check me!";
    return nav->get_freq() / 1000.0;
}

double QmlPositioned::navaidRangeNm() const
{
    FGNavRecord* nav = fgpositioned_cast<FGNavRecord>(m_pos);
    if (!nav)
        return 0.0;

    return nav->get_range();
}

QmlPositioned *QmlPositioned::navaidRunway() const
{
    FGNavRecord* nav = fgpositioned_cast<FGNavRecord>(m_pos);
    if (!nav || !nav->runway())
        return nullptr;

    return new QmlPositioned(nav->runway());
}

QmlPositioned *QmlPositioned::colocatedDME() const
{
    FGNavRecord* nav = fgpositioned_cast<FGNavRecord>(m_pos);
    if (!nav || !nav->hasDME())
        return nullptr;

    return new QmlPositioned(flightgear::NavDataCache::instance()->loadById(nav->colocatedDME()));
}

QmlPositioned *QmlPositioned::owningAirport() const
{
    FGRunway* runway = fgpositioned_cast<FGRunway>(m_pos);
    if (runway) {
        return new QmlPositioned(runway->airport());
    }

    return nullptr;
}

double QmlPositioned::runwayHeadingDeg() const
{
    FGRunway* runway = fgpositioned_cast<FGRunway>(m_pos);
    if (runway) {
        return runway->headingDeg();
    }

    return 0.0;
}

double QmlPositioned::runwayLengthFt() const
{
    FGRunway* runway = fgpositioned_cast<FGRunway>(m_pos);
    if (runway) {
        return runway->lengthFt();
    }

    return 0.0;
}

bool QmlPositioned::equals(QmlPositioned *other) const
{
    return (other && (other->inner() == inner()));
}

bool QmlPositioned::airportHasParkings() const
{
    if (!isAirportType())
        return false;

    FGAirport* apt = fgpositioned_cast<FGAirport>(m_pos);
    if (!apt)
        return false;

    return !apt->groundNetwork()->allParkings().empty();
}

bool operator==(const QmlPositioned& p1, const QmlPositioned& p2)
{
    return p1.inner() == p2.inner();
}


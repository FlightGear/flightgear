// TankProperties.cxx -- expose (fuel)-tank properties 
//
// Written by Torsten Dreyer, started January 2011.
//
// Copyright (C) 2011  Torsten Dreyer - Torsten (at) t3r _dot_ de
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
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "TankProperties.hxx"

#include <simgear/math/SGMath.hxx>
#include <simgear/sg_inlines.h>
#include <Main/fg_props.hxx>

static const double LBS_PER_KG = 2.20462262;
static const double KG_PER_LBS = 1.0/LBS_PER_KG;
static const double USGAL_PER_M3 = 1000.0/3.785411784;
static const double M3_PER_USGAL = 1.0/USGAL_PER_M3;
static const double IMPGAL_PER_M3 = 1000.0/4.54609;
static const double M3_PER_IMPGAL = 1.0/IMPGAL_PER_M3;

TankProperties::TankProperties(SGPropertyNode_ptr rootNode ) :
  _content_kg(0.0),
  _density_kgpm3(0.0),
  _capacity_m3(0.0),
  _unusable_m3(0.0)
{
  _tiedProperties.setRoot( rootNode );
}

void TankProperties::bind()
{
  _tiedProperties.Tie("level-kg", this, &TankProperties::getContent_kg, &TankProperties::setContent_kg );
  _tiedProperties.Tie("density-kgpm3", this, &TankProperties::getDensity_kgpm3, &TankProperties::setDensity_kgpm3 );
  _tiedProperties.Tie("capacity-m3", this, &TankProperties::getCapacity_m3, &TankProperties::setCapacity_m3 );
  _tiedProperties.Tie("unusable-m3", this, &TankProperties::getUnusable_m3, &TankProperties::setUnusable_m3 );
  _tiedProperties.Tie("level-m3", this, &TankProperties::getContent_m3, &TankProperties::setContent_m3 );
  _tiedProperties.Tie("level-norm", this, &TankProperties::getContent_norm, &TankProperties::setContent_norm );

  _tiedProperties.Tie("density-ppg", this, &TankProperties::getDensity_ppg, &TankProperties::setDensity_ppg );
  _tiedProperties.Tie("level-lbs", this, &TankProperties::getContent_lbs, &TankProperties::setContent_lbs );
  _tiedProperties.Tie("level-gal_us", this, &TankProperties::getContent_gal_us, &TankProperties::setContent_gal_us );
  _tiedProperties.Tie("level-gal_imp", this, &TankProperties::getContent_gal_imp, &TankProperties::setContent_gal_imp );

  _tiedProperties.Tie("capacity-gal_us", this, &TankProperties::getCapacity_gal_us, &TankProperties::setCapacity_gal_us );
  _tiedProperties.Tie("unusable-gal_us", this, &TankProperties::getUnusable_gal_us, &TankProperties::setUnusable_gal_us );

  _tiedProperties.Tie("capacity-gal_imp", this, &TankProperties::getCapacity_gal_imp, &TankProperties::setCapacity_gal_imp );
  _tiedProperties.Tie("unusable-gal_imp", this, &TankProperties::getUnusable_gal_imp, &TankProperties::setUnusable_gal_imp );

  _tiedProperties.Tie("empty", this, &TankProperties::getEmpty );
}

TankProperties::~TankProperties()
{
}

void TankProperties::unbind()
{
    _tiedProperties.Untie();
}

double TankProperties::getContent_kg() const
{
  return _content_kg;
}

void TankProperties::setContent_kg( double value )
{
  _content_kg = SG_MAX2<double>(value, 0.0);
}

double TankProperties::getDensity_kgpm3() const
{
  return _density_kgpm3;
}

void TankProperties::setDensity_kgpm3( double value )
{
  _density_kgpm3 = SG_MAX2<double>(value, 0.0);
}

double TankProperties::getDensity_ppg() const
{
  return _density_kgpm3 * LBS_PER_KG / USGAL_PER_M3;
}

void TankProperties::setDensity_ppg( double value )
{
  _density_kgpm3 = SG_MAX2<double>(value * KG_PER_LBS / M3_PER_USGAL, 0.0);
}

double TankProperties::getContent_lbs() const
{
  return _content_kg * LBS_PER_KG;
}

void TankProperties::setContent_lbs( double value )
{
  _content_kg = SG_MAX2<double>(value * KG_PER_LBS, 0.0);
}

double TankProperties::getContent_m3() const
{
  return _density_kgpm3 > SGLimitsd::min() ? _content_kg / _density_kgpm3 : 0.0;
}

void TankProperties::setContent_m3( double value )
{
  // ugly hack to allow setting of a volumetric content without having the density
  _content_kg = SG_MAX2<double>(value * (_density_kgpm3>0.0?_density_kgpm3:755.0), 0.0);
}

double TankProperties::getContent_gal_us() const
{
  return getContent_m3() * USGAL_PER_M3;
}

void TankProperties::setContent_gal_us( double value )
{
  setContent_m3( value * M3_PER_USGAL );
}

double TankProperties::getContent_gal_imp() const
{
  return getContent_m3() * IMPGAL_PER_M3;
}

void TankProperties::setContent_gal_imp( double value )
{
  setContent_m3( value * M3_PER_IMPGAL );
}

double TankProperties::getCapacity_m3() const
{
  return _capacity_m3;
}

void TankProperties::setCapacity_m3( double value )
{
  _capacity_m3 = SG_MAX2<double>(value, 0.0);
}

double TankProperties::getCapacity_gal_us() const
{
  return _capacity_m3 * USGAL_PER_M3;
}

void TankProperties::setCapacity_gal_us( double value )
{
  _capacity_m3 = SG_MAX2<double>(value * M3_PER_USGAL, 0.0);
}

double TankProperties::getCapacity_gal_imp() const
{
  return _capacity_m3 * IMPGAL_PER_M3;
}

void TankProperties::setCapacity_gal_imp( double value )
{
  _capacity_m3 = SG_MAX2<double>(value * M3_PER_IMPGAL, 0.0);
}

double TankProperties::getUnusable_m3() const
{
  return _unusable_m3;
}

void TankProperties::setUnusable_m3( double value )
{
  _unusable_m3 = SG_MAX2<double>(value, 0.0);
}

double TankProperties::getUnusable_gal_us() const
{
  return _unusable_m3 * USGAL_PER_M3;
}

void TankProperties::setUnusable_gal_us( double value )
{
  _unusable_m3 = SG_MAX2<double>(value * M3_PER_USGAL, 0.0);
}


double TankProperties::getUnusable_gal_imp() const
{
  return _unusable_m3 * IMPGAL_PER_M3;
}

void TankProperties::setUnusable_gal_imp( double value )
{
  _unusable_m3 = SG_MAX2<double>(value * M3_PER_IMPGAL, 0.0);
}

double TankProperties::getContent_norm() const
{
  return  _capacity_m3 > SGLimitsd::min() ? getContent_m3() / _capacity_m3 : 0.0;
}

void TankProperties::setContent_norm( double value )
{
  setContent_m3(_capacity_m3 * value);
}

bool TankProperties::getEmpty() const
{
  return getContent_m3() <= _unusable_m3;
}

TankPropertiesList::TankPropertiesList( SGPropertyNode_ptr rootNode )
{
  // we don't have a global rule how many tanks we support, so I assume eight.
  // Because hard coded values suck, make it settable by a property.
  // If tanks were configured, use that number
  int n = rootNode->getChildren("tank").size();
  if( n == 0 ) n = rootNode->getIntValue( "numtanks", 8 );
  for( int i = 0; i < n; i++ ) {
    push_back( new TankProperties( rootNode->getChild( "tank", i, true ) ) );
  }

  _tiedProperties.setRoot( rootNode );
}

double TankPropertiesList::getTotalContent_lbs() const
{
  double value = 0.0;
  for( const_iterator it = begin(); it != end(); ++it )
    value += (*it)->getContent_lbs();
  return value;
}

double TankPropertiesList::getTotalContent_kg() const
{
  double value = 0.0;
  for( const_iterator it = begin(); it != end(); ++it )
    value += (*it)->getContent_kg();
  return value;
}

double TankPropertiesList::getTotalContent_gal_us() const
{
  double value = 0.0;
  for( const_iterator it = begin(); it != end(); ++it )
    value += (*it)->getContent_gal_us();
  return value;
}

double TankPropertiesList::getTotalContent_gal_imp() const
{
  double value = 0.0;
  for( const_iterator it = begin(); it != end(); ++it )
    value += (*it)->getContent_gal_imp();
  return value;
}

double TankPropertiesList::getTotalContent_m3() const
{
  double value = 0.0;
  for( const_iterator it = begin(); it != end(); ++it )
    value += (*it)->getContent_m3();
  return value;
}

double TankPropertiesList::getTotalContent_norm() const
{
  double content = 0.0;
  double capacity = 0.0;
  for( const_iterator it = begin(); it != end(); ++it ) {
    content += (*it)->getContent_m3();
    capacity += (*it)->getCapacity_m3();
  }
  return capacity > SGLimitsd::min() ? content / capacity : 0.0;
}

void TankPropertiesList::bind()
{
    _tiedProperties.Tie("total-fuel-kg", this, &TankPropertiesList::getTotalContent_kg );
    _tiedProperties.Tie("total-fuel-lbs", this, &TankPropertiesList::getTotalContent_lbs );
    _tiedProperties.Tie("total-fuel-gal_us", this, &TankPropertiesList::getTotalContent_gal_us );
    _tiedProperties.Tie("total-fuel-gals", this, &TankPropertiesList::getTotalContent_gal_us );
    _tiedProperties.Tie("total-fuel-gal_imp", this, &TankPropertiesList::getTotalContent_gal_imp );
    _tiedProperties.Tie("total-fuel-norm", this, &TankPropertiesList::getTotalContent_norm );
    for( const_iterator it = begin(); it != end(); ++it ) {
      (*it)->bind();
    }
}

void TankPropertiesList::unbind()
{
    for( const_iterator it = begin(); it != end(); ++it ) {
      (*it)->unbind();
    }
    _tiedProperties.Untie();
}

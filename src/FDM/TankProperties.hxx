// TankProperties.hxx -- expose (fuel)-tank properties 
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

#ifndef __TANK_PROPERTIES_HXX
#define __TANK_PROPERTIES_HXX

#include <simgear/props/props.hxx>
#include <simgear/props/tiedpropertylist.hxx>

class TankProperties : public SGReferenced
{
public:
  TankProperties(SGPropertyNode_ptr rootNode );
  virtual ~TankProperties();

  TankProperties( const TankProperties & );
  const TankProperties & operator = ( const TankProperties & );

  void bind();
  void unbind();
  
  double getContent_kg() const;
  void setContent_kg( double value );

  double getDensity_kgpm3() const;
  void setDensity_kgpm3( double value );

  double getDensity_ppg() const;
  void setDensity_ppg( double value );

  double getContent_lbs() const;
  void setContent_lbs( double value );

  double getContent_m3() const;
  void setContent_m3( double value );

  double getContent_gal_us() const;
  void setContent_gal_us( double value );

  double getContent_gal_imp() const;
  void setContent_gal_imp( double value );

  double getCapacity_m3() const;
  void setCapacity_m3( double value );

  double getCapacity_gal_us() const;
  void setCapacity_gal_us( double value );

  double getCapacity_gal_imp() const;
  void setCapacity_gal_imp( double value );

  double getUnusable_m3() const;
  void setUnusable_m3( double value );

  double getUnusable_gal_us() const;
  void setUnusable_gal_us( double value );

  double getUnusable_gal_imp() const;
  void setUnusable_gal_imp( double value );

  double getContent_norm() const;
  void setContent_norm( double value );

  bool getEmpty() const;

protected:
  simgear::TiedPropertyList _tiedProperties;

  double _content_kg;
  double _density_kgpm3;
  double _capacity_m3;
  double _unusable_m3;
};

class TankPropertiesList : std::vector<SGSharedPtr<TankProperties> > {
public:
  TankPropertiesList( SGPropertyNode_ptr rootNode );

  void bind();
  void unbind();

  double getTotalContent_lbs() const;
  double getTotalContent_kg() const;
  double getTotalContent_gal_us() const;
  double getTotalContent_gal_imp() const;
  double getTotalContent_m3() const;
  double getTotalContent_norm() const;
  
private:
  simgear::TiedPropertyList _tiedProperties;
};

#endif // __TANK_PROPERTIES_HXX

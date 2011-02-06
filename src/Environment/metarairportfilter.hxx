// metarairportfilter.hxx -- Implementation of AirportFilter
//
// Written by Torsten Dreyer, August 2010
//
// Copyright (C) 2010  Torsten Dreyer Torsten(at)t3r(dot)de
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

#ifndef __METARAIRPORTFILTER_HXX
#define __METARAIRPORTFILTER_HXX

#include <Airports/simple.hxx>
#include <simgear/props/props.hxx>

namespace Environment {

/**
 * @brief A AirportFilter for selection airports that provide a METAR
 * Singleton implementation of FGAirport::AirportFilter
 */
class MetarAirportFilter : public FGAirport::AirportFilter {
public:
    static MetarAirportFilter * instance();
protected:
    MetarAirportFilter() {}
    virtual bool passAirport(FGAirport* aApt) const {
        return aApt->getMetar();
    }
  
  // permit heliports and seaports too
  virtual FGPositioned::Type maxType() const
  { return FGPositioned::SEAPORT; }
private:
  static MetarAirportFilter * _instance;
};

}
#endif

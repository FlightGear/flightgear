// metar interface class
//
// Written by Melchior FRANZ, started January 2005.
//
// Copyright (C) 2005  Melchior FRANZ - mfranz@aon.at
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
// $Id$

#ifndef _FGMETAR_HXX
#define _FGMETAR_HXX

#include <vector>
#include <map>
#include <string>
#include <time.h>

#include <simgear/environment/metar.hxx>

using std::vector;
using std::map;
using std::string;


class FGMetar : public SGMetar, public SGReferenced {
public:
	FGMetar(const string& icao);

	long	getAge_min()			const;
	time_t	getTime()			const { return _time; }
	double	getRain()			const { return _rain / 3.0; }
	double	getHail()			const { return _hail / 3.0; }
	double	getSnow()			const { return _snow / 3.0; }
	bool	getSnowCover()			const { return _snow_cover; }

private:
	time_t	_rq_time;
	time_t	_time;
	bool	_snow_cover;
};

#endif // _FGMETAR_HXX

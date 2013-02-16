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

/**
 * @file fgmetar.cxx
 * Implements FGMetar class that inherits from SGMetar.
 *
 * o provide defaults for unset values
 * o interpolate/randomize data (GREATER_THAN)
 * o derive additional values (time, age, snow cover)
 * o consider minimum identifier (CAVOK, mil. color codes)
 *
 * TODO
 * - NSC & mil. color codes
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <simgear/math/sg_random.h>
#include <simgear/timing/sg_time.hxx>
#include <Main/fg_props.hxx>

#include "fgmetar.hxx"


FGMetar::FGMetar(const string& icao) :
	SGMetar(icao),
	_snow_cover(false)
{
	int i;
	double d;

	// CAVOK: visibility >= 10km; lowest cloud layer >= 5000 ft; any coverage
	if (getCAVOK()) {
		if (_min_visibility.getVisibility_m() == SGMetarNaN)
			_min_visibility.set(12000.0);

		if (_max_visibility.getVisibility_m() == SGMetarNaN)
			_min_visibility.set(12000.0);

		vector<SGMetarCloud> cv = _clouds;;
		if (cv.empty()) {
			SGMetarCloud cl;
			cl.set(5500 * SG_FEET_TO_METER, SGMetarCloud::COVERAGE_SCATTERED);
			_clouds.push_back(cl);
		}
	}

	// visibility
	d = _min_visibility.getVisibility_m();
	if (d == SGMetarNaN)
		d = 10000.0;
	if (_min_visibility.getModifier() == SGMetarVisibility::GREATER_THAN)
		d += 15000.0;// * sg_random();
	_min_visibility.set(d);

	if (_max_visibility.getVisibility_m() == SGMetarNaN)
		_max_visibility.set(d);

	for (i = 0; i < 8; i++) {
		d = _dir_visibility[i].getVisibility_m();
		if (d == SGMetarNaN)
			_dir_visibility[i].set(10000.0);
		if (_dir_visibility[i].getModifier() == SGMetarVisibility::GREATER_THAN)
			d += 15000.0;// * sg_random();
		_dir_visibility[i].set(d);
	}

	// wind
	if (_wind_dir == -1) {
		if (_wind_range_from == -1) {
			_wind_dir = 0;
			_wind_range_from = 0;
			_wind_range_to = 359;
		} else {
			_wind_dir = (_wind_range_from + _wind_range_to) / 2;
		}
	} else if (_wind_range_from == -1) {
		_wind_range_from = _wind_range_to = _wind_dir;
	}

	if (_wind_speed == SGMetarNaN)
		_wind_speed = 0.0;
	if (_gust_speed == SGMetarNaN)
		_gust_speed = 0.0;

	// clouds
	vector<SGMetarCloud> cv = _clouds;
	vector<SGMetarCloud>::iterator cloud, cv_end = cv.end();

	for (i = 0, cloud = cv.begin(); cloud != cv_end; ++cloud, i++) {
		SGMetarCloud::Coverage cov = cloud->getCoverage();
		if (cov == SGMetarCloud::COVERAGE_NIL)
			cov = SGMetarCloud::COVERAGE_CLEAR;

		double alt = cloud->getAltitude_ft();
		if (alt == SGMetarNaN)
			alt = -9999;

		cloud->set(alt, cov);
	}


	// temperature/pressure
	if (_temp == SGMetarNaN)
		_temp = 15.0;

	if (_dewp == SGMetarNaN)
		_dewp = 0.0;

	if (_pressure == SGMetarNaN)
		_pressure = 30.0 * SG_INHG_TO_PA;

	// snow cover
	map<string, SGMetarRunway> rm = getRunways();
	map<string, SGMetarRunway>::const_iterator runway, rm_end = rm.end();
	for (runway = rm.begin(); runway != rm_end; ++runway) {
		SGMetarRunway rwy = runway->second;
		if (rwy.getDeposit() >= 3 ) {
			_snow_cover = true;
			break;
		}
	}
	if (_temp < 5.0 && _snow)
		_snow_cover = true;
	if (_temp < 1.0 && getRelHumidity() > 80)
		_snow_cover = true;

	_time = sgTimeGetGMT(_year - 1900, _month - 1, _day, _hour, _minute, 0);

	SG_LOG(SG_ENVIRONMENT, SG_INFO, _data);
	if (_x_proxy)
		SG_LOG(SG_ENVIRONMENT, SG_INFO, "METAR from proxy");
	else
		SG_LOG(SG_ENVIRONMENT, SG_INFO, "METAR from weather.noaa.gov");
}


long FGMetar::getAge_min() const
{
	time_t now = _x_proxy ? _rq_time : time(0);
	return (now - _time) / 60;
}


// metar interface class
//
// Written by Melchior FRANZ, started December 2003.
//
// Copyright (C) 2003  Melchior FRANZ - mfranz@aon.at
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
// Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
//
// $Id$

#ifndef _METAR_HXX
#define _METAR_HXX

#include <vector>
#include <map>
#include <string>

#include <simgear/constants.h>

SG_USING_STD(vector);
SG_USING_STD(map);
SG_USING_STD(string);

const double FGMetarNaN = -1E20;
#define NaN FGMetarNaN

struct Token {
	char	*id;
	char	*text;
};


class Metar;

class FGMetarVisibility {
	friend class Metar;
public:
	FGMetarVisibility() :
		_distance(NaN),
		_direction(-1),
		_modifier(EQUALS),
		_tendency(NONE) {}

	enum Modifier {
		NOGO,
		EQUALS,
		LESS_THAN,
		GREATER_THAN
	};

	enum Tendency {
		NONE,
		STABLE,
		INCREASING,
		DECREASING
	};

	inline double	getVisibility_m()	const { return _distance; }
	inline double	getVisibility_ft()	const { return _distance == NaN ? NaN : _distance * SG_METER_TO_FEET; }
	inline double	getVisibility_sm()	const { return _distance == NaN ? NaN : _distance * SG_METER_TO_SM; }
	inline int	getDirection()		const { return _direction; }
	inline int	getModifier()		const { return _modifier; }
	inline int	getTendency()		const { return _tendency; }

protected:
	double	_distance;
	int	_direction;
	int	_modifier;
	int	_tendency;
};


// runway condition (surface and visibility)
class FGMetarRunway {
	friend class Metar;
public:
	FGMetarRunway() :
		_deposit(0),
		_extent(-1),
		_extent_string(0),
		_depth(NaN),
		_friction(NaN),
		_friction_string(0),
		_comment(0),
		_wind_shear(false) {}

	inline const char		*getDeposit()		const { return _deposit; }
	inline double			getExtent()		const { return _extent; }
	inline const char		*getExtentString()	const { return _extent_string; }
	inline double			getDepth()		const { return _depth; }
	inline double			getFriction()		const { return _friction; }
	inline const char		*getFrictionString()	const { return _friction_string; }
	inline const char		*getComment()		const { return _comment; }
	inline const bool		getWindShear()		const { return _wind_shear; }
	inline FGMetarVisibility	getMinVisibility()	const { return _min_visibility; }
	inline FGMetarVisibility	getMaxVisibility()	const { return _max_visibility; }

protected:
	FGMetarVisibility _min_visibility;
	FGMetarVisibility _max_visibility;
	const char	*_deposit;
	int		_extent;
	const char	*_extent_string;
	double		_depth;
	double		_friction;
	const char	*_friction_string;
	const char	*_comment;
	bool		_wind_shear;
};


// cloud layer
class FGMetarCloud {
	friend class Metar;
public:
	FGMetarCloud() :
		_coverage(-1),
		_altitude(NaN),
		_type(0),
		_type_long(0) {}

	inline int	getCoverage()		const { return _coverage; }
	inline double	getAltitude_m()		const { return _altitude; }
	inline double	getAltitude_ft()	const { return _altitude == NaN ? NaN : _altitude * SG_METER_TO_FEET; }
	inline char	*getTypeString()	const { return _type; }
	inline char	*getTypeLongString()	const { return _type_long; }

protected:
	int	_coverage;		// quarters: 0 -> clear ... 4 -> overcast
	double	_altitude;		// 1000 m
	char	*_type;			// CU
	char	*_type_long;		// cumulus
};


class Metar {
public:
	Metar(const char *m);
	Metar(const string m) { Metar(m.c_str()); }
	~Metar();

	enum ReportType {
		NONE,
		AUTO,
		COR,
		RTD
	};

	inline const char *getData()		const { return _data; }
	inline const char *getUnusedData()	const { return _m; }
	inline const char *getId()		const { return _icao; }
	inline int	getYear()		const { return _year; }
	inline int	getMonth()		const { return _month; }
	inline int	getDay()		const { return _day; }
	inline int	getHour()		const { return _hour; }
	inline int	getMinute()		const { return _minute; }
	inline int	getReportType()		const { return _report_type; }

	inline int	getWindDir()		const { return _wind_dir; }
	inline double	getWindSpeed_mps()	const { return _wind_speed; }
	inline double	getWindSpeed_kmh()	const { return _wind_speed == NaN ? NaN : _wind_speed * 3.6; }
	inline double	getWindSpeed_kt()	const { return _wind_speed == NaN ? NaN : _wind_speed * SG_MPS_TO_KT; }
	inline double	getWindSpeed_mph()	const { return _wind_speed == NaN ? NaN : _wind_speed * SG_MPS_TO_MPH; }

	inline double	getGustSpeed_mps()	const { return _gust_speed; }
	inline double	getGustSpeed_kmh()	const { return _gust_speed == NaN ? NaN : _gust_speed * 3.6; }
	inline double	getGustSpeed_kt()	const { return _gust_speed == NaN ? NaN : _gust_speed * SG_MPS_TO_KT; }
	inline double	getGustSpeed_mph()	const { return _gust_speed == NaN ? NaN : _gust_speed * SG_MPS_TO_MPH; }

	inline int	getWindRangeFrom()	const { return _wind_range_from; }
	inline int	getWindRangeTo()	const { return _wind_range_to; }

	inline FGMetarVisibility& getMinVisibility()	{ return _min_visibility; }
	inline FGMetarVisibility& getMaxVisibility()	{ return _max_visibility; }
	inline FGMetarVisibility& getVertVisibility()	{ return _vert_visibility; }
	inline FGMetarVisibility *getDirVisibility()	{ return _dir_visibility; }

	inline double	getTemperature_C()	const { return _temp; }
	inline double	getTemperature_F()	const { return _temp == NaN ? NaN : 1.8 * _temp + 32; }
	inline double	getDewpoint_C()		const { return _dewp; }
	inline double	getDewpoint_F()		const { return _dewp == NaN ? NaN : 1.8 * _dewp + 32; }
	inline double	getPressure_hPa()	const { return _pressure == NaN ? NaN : _pressure / 100; }
	inline double	getPressure_inHg()	const { return _pressure == NaN ? NaN : _pressure * SG_PA_TO_INHG; }

	double		getRelHumidity()	const;

	inline vector<FGMetarCloud>& getClouds()	{ return _clouds; }
	inline map<string, FGMetarRunway>& getRunways()	{ return _runways; }
	inline vector<string>& getWeather()		{ return _weather; }

protected:
	int	_grpcount;
	char	*_data;
	char	*_m;
	char	_icao[5];
	int	_year;
	int	_month;
	int	_day;
	int	_hour;
	int	_minute;
	int	_report_type;
	int	_wind_dir;
	double	_wind_speed;
	double	_gust_speed;
	int	_wind_range_from;
	int	_wind_range_to;
	double	_temp;
	double	_dewp;
	double	_pressure;

	FGMetarVisibility		_min_visibility;
	FGMetarVisibility		_max_visibility;
	FGMetarVisibility		_vert_visibility;
	FGMetarVisibility		_dir_visibility[8];
	vector<FGMetarCloud>		_clouds;
	map<string, FGMetarRunway>	_runways;
	vector<string>			_weather;

	bool	scanPreambleDate();
	bool	scanPreambleTime();

	bool	scanType();
	bool	scanId();
	bool	scanDate();
	bool	scanModifier();
	bool	scanWind();
	bool	scanVariability();
	bool	scanVisibility();
	bool	scanRwyVisRange();
	bool	scanSkyCondition();
	bool	scanWeather();
	bool	scanTemperature();
	bool	scanPressure();
	bool	scanRunwayReport();
	bool	scanWindShear();
	bool	scanTrendForecast();
	bool	scanColorState();
	bool	scanRemark();
	bool	scanRemainder();

	int	scanNumber(char **str, int *num, int min, int max = 0);
	bool	scanBoundary(char **str);
	const struct Token *scanToken(char **str, const struct Token *list);
	char	*loadData(const char *id);
	void	normalizeData();
};

#undef NaN
#endif // _METAR_HXX

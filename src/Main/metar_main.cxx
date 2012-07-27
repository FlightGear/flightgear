// metar interface class demo
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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include <iomanip>
#include <sstream>
#include <iostream>
#include <string.h>
#include <time.h>
#include <cstdlib>

#include <simgear/debug/logstream.hxx>
#include <simgear/environment/metar.hxx>
#include <simgear/structure/exception.hxx>

#include <simgear/io/HTTPClient.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/timing/timestamp.hxx>

using namespace std;
using namespace simgear;

class MetarRequest : public HTTP::Request
{
public:
    bool complete;
    bool failed;
    string metarData;
    bool fromProxy;
    
    MetarRequest(const std::string& stationId) : 
        HTTP::Request("http://weather.noaa.gov/pub/data/observations/metar/stations/" + stationId + ".TXT"),
        complete(false),
        failed(false)
    {
        fromProxy = false;
    }
    
protected:
    
    virtual void responseHeader(const string& key, const string& value)
    {
        if (key == "x-metarproxy") {
            fromProxy = true;
        }
    }

    virtual void gotBodyData(const char* s, int n)
    {
        metarData += string(s, n);
    }

    virtual void responseComplete()
    {
        if (responseCode() == 200) {
            complete = true;
        } else {
            SG_LOG(SG_ENVIRONMENT, SG_WARN, "metar download failed:" << url() << ": reason:" << responseReason());
            failed = true;
        }
    }
};

// text color
#if defined(__linux__) || defined(__sun) || defined(__CYGWIN__) \
    || defined( __FreeBSD__ ) || defined ( sgi )
#	define R "\033[31;1m"		// red
#	define G "\033[32;1m"		// green
#	define Y "\033[33;1m"		// yellow
#	define B "\033[34;1m"		// blue
#	define M "\033[35;1m"		// magenta
#	define C "\033[36;1m"		// cyan
#	define W "\033[37;1m"		// white
#	define N "\033[m"		// normal
#else
#	define R ""
#	define G ""
#	define Y ""
#	define B ""
#	define M ""
#	define C ""
#	define W ""
#	define N ""
#endif



const char *azimuthName(double d)
{
	const char *dir[] = {
		"N", "NNE", "NE", "ENE",
		"E", "ESE", "SE", "SSE",
		"S", "SSW", "SW", "WSW",
		"W", "WNW", "NW", "NNW"
	};
	d += 11.25;
	while (d < 0)
		d += 360;
	while (d >= 360)
		d -= 360;
	return dir[int(d / 22.5)];
}


// round double to 10^g
double rnd(double r, int g = 0)
{
	double f = pow(10.0, g);
	return f * floor(r / f + 0.5);
}


ostream& operator<<(ostream& s, const SGMetarVisibility& v)
{
	ostringstream buf;
	int m = v.getModifier();
	const char *mod;
	if (m == SGMetarVisibility::GREATER_THAN)
		mod = ">=";
	else if (m == SGMetarVisibility::LESS_THAN)
		mod = "<";
	else
		mod = "";
	buf << mod;

	double dist = rnd(v.getVisibility_m(), 1);
	if (dist < 1000.0)
		buf << rnd(dist, 1) << " m";
	else
		buf << rnd(dist / 1000.0, -1) << " km";

	const char *dir = "";
	int i;
	if ((i = v.getDirection()) != -1) {
		dir = azimuthName(i);
		buf << " " << dir;
	}
	buf << "\t\t\t\t\t" << mod << rnd(v.getVisibility_sm(), -1) << " US-miles " << dir;
	return s << buf.str();
}


void printReport(SGMetar *m)
{
#define NaN SGMetarNaN
	const char *s;
	char buf[256];
	double d;
	int i, lineno;

	if ((i = m->getReportType()) == SGMetar::AUTO)
		s = "\t\t(automatically generated)";
	else if (i == SGMetar::COR)
		s = "\t\t(manually corrected)";
	else if (i == SGMetar::RTD)
		s = "\t\t(routine delayed)";
	else
		s = "";

	cout << "METAR Report" << s << endl;
	cout << "============" << endl;
	cout << "Airport-Id:\t\t" << m->getId() << endl;


	// date/time
	int year = m->getYear();
	int month = m->getMonth();
	cout << "Report time:\t\t" << year << '/' << month << '/' << m->getDay();
	cout << ' ' << m->getHour() << ':';
	cout << setw(2) << setfill('0') << m->getMinute() << " UTC" << endl;


	// visibility
	SGMetarVisibility minvis = m->getMinVisibility();
	SGMetarVisibility maxvis = m->getMaxVisibility();
	double min = minvis.getVisibility_m();
	double max = maxvis.getVisibility_m();
	if (min != NaN) {
		if (max != NaN) {
			cout << "min. Visibility:\t" << minvis << endl;
			cout << "max. Visibility:\t" << maxvis << endl;
		} else
			cout << "Visibility:\t\t" << minvis << endl;
	}


	// directed visibility
	const SGMetarVisibility *dirvis = m->getDirVisibility();
	for (i = 0; i < 8; i++, dirvis++)
		if (dirvis->getVisibility_m() != NaN)
			cout << "\t\t\t" << *dirvis << endl;


	// vertical visibility
	SGMetarVisibility vertvis = m->getVertVisibility();
	if ((d = vertvis.getVisibility_ft()) != NaN)
		cout << "Vert. visibility:\t" << vertvis << endl;
	else if (vertvis.getModifier() == SGMetarVisibility::NOGO)
		cout << "Vert. visibility:\timpossible to determine" << endl;


	// wind
	d = m->getWindSpeed_kmh();
	cout << "Wind:\t\t\t";
	if (d < .1)
		cout << "none" << endl;
	else {
		if ((i = m->getWindDir()) == -1)
			cout << "from variable directions";
		else
			cout << "from the " << azimuthName(i) << " (" << i << "°)";
		cout << " at " << rnd(d, -1) << " km/h";

		cout << "\t\t" << rnd(m->getWindSpeed_kt(), -1) << " kt";
		cout << " = " << rnd(m->getWindSpeed_mph(), -1) << " mph";
		cout << " = " << rnd(m->getWindSpeed_mps(), -1) << " m/s";
		cout << endl;

		if ((d = m->getGustSpeed_kmh()) != NaN) {
			cout << "\t\t\twith gusts at " << rnd(d, -1) << " km/h";
			cout << "\t\t\t" << rnd(m->getGustSpeed_kt(), -1) << " kt";
			cout << " = " << rnd(m->getGustSpeed_mph(), -1) << " mph";
			cout << " = " << rnd(m->getGustSpeed_mps(), -1) << " m/s";
			cout << endl;
		}

		int from = m->getWindRangeFrom();
		int to = m->getWindRangeTo();
		if (from != to) {
			cout << "\t\t\tvariable from " << azimuthName(from);
			cout << " to " << azimuthName(to);
			cout << " (" << from << "°--" << to << "°)" << endl;
		}
	}


	// temperature/humidity/air pressure
	if ((d = m->getTemperature_C()) != NaN) {
		cout << "Temperature:\t\t" << d << "°C\t\t\t\t\t";
		cout << rnd(m->getTemperature_F(), -1) << "°F" << endl;

		if ((d = m->getDewpoint_C()) != NaN) {
			cout << "Dewpoint:\t\t" << d << "°C\t\t\t\t\t";
			cout << rnd(m->getDewpoint_F(), -1) << "°F"  << endl;
			cout << "Rel. Humidity:\t\t" << rnd(m->getRelHumidity()) << "%" << endl;
		}
	}
	if ((d = m->getPressure_hPa()) != NaN) {
		cout << "Pressure:\t\t" << rnd(d) << " hPa\t\t\t\t";
		cout << rnd(m->getPressure_inHg(), -2) << " in. Hg" << endl;
	}


	// weather phenomena
	vector<string> wv = m->getWeather();
	vector<string>::iterator weather;
	for (i = 0, weather = wv.begin(); weather != wv.end(); weather++, i++) {
		cout << (i ? ", " : "Weather:\t\t") << weather->c_str();
	}
	if (i)
		cout << endl;


	// cloud layers
	const char *coverage_string[5] = {
		"clear skies", "few clouds", "scattered clouds", "broken clouds", "sky overcast"
	};
	vector<SGMetarCloud> cv = m->getClouds();
	vector<SGMetarCloud>::iterator cloud;
	for (lineno = 0, cloud = cv.begin(); cloud != cv.end(); cloud++, lineno++) {
		cout << (lineno ? "\t\t\t" : "Sky condition:\t\t");

		if ((i = cloud->getCoverage()) != -1)
			cout << coverage_string[i];
		if ((d = cloud->getAltitude_ft()) != NaN)
			cout << " at " << rnd(d, 1) << " ft";
		if ((s = cloud->getTypeLongString()))
			cout << " (" << s << ')';
		if (d != NaN)
			cout << "\t\t\t" << rnd(cloud->getAltitude_m(), 1) << " m";
		cout << endl;
	}


	// runways
	map<string, SGMetarRunway> rm = m->getRunways();
	map<string, SGMetarRunway>::iterator runway;
	for (runway = rm.begin(); runway != rm.end(); runway++) {
		lineno = 0;
		if (!strcmp(runway->first.c_str(), "ALL"))
			cout << "All runways:\t\t";
		else
			cout << "Runway " << runway->first << ":\t\t";
		SGMetarRunway rwy = runway->second;

		// assemble surface string
		vector<string> surface;
		if ((s = rwy.getDepositString()) && strlen(s))
			surface.push_back(s);
		if ((s = rwy.getExtentString()) && strlen(s))
			surface.push_back(s);
		if ((d = rwy.getDepth()) != NaN) {
			sprintf(buf, "%.1lf mm", d * 1000.0);
			surface.push_back(buf);
		}
		if ((s = rwy.getFrictionString()) && strlen(s))
			surface.push_back(s);
		if ((d = rwy.getFriction()) != NaN) {
			sprintf(buf, "friction: %.2lf", d);
			surface.push_back(buf);
		}

		if (surface.size()) {
			vector<string>::iterator rwysurf = surface.begin();
			for (i = 0; rwysurf != surface.end(); rwysurf++, i++) {
				if (i)
					cout << ", ";
				cout << *rwysurf;
			}
			lineno++;
		}

		// assemble visibility string
		SGMetarVisibility minvis = rwy.getMinVisibility();
		SGMetarVisibility maxvis = rwy.getMaxVisibility();
		if ((d = minvis.getVisibility_m()) != NaN) {
			if (lineno++)
				cout << endl << "\t\t\t";
			cout << minvis;
		}
		if (maxvis.getVisibility_m() != d) {
			cout << endl << "\t\t\t" << maxvis << endl;
			lineno++;
		}

		if (rwy.getWindShear()) {
			if (lineno++)
				cout << endl << "\t\t\t";
			cout << "critical wind shear" << endl;
		}
		cout << endl;
	}
	cout << endl;
#undef NaN
}


void printArgs(SGMetar *m, double airport_elevation)
{
#define NaN SGMetarNaN
	vector<string> args;
	char buf[256];
	int i;

	// ICAO id
	sprintf(buf, "--airport=%s ", m->getId());
	args.push_back(buf);

	// report time
	sprintf(buf, "--start-date-gmt=%4d:%02d:%02d:%02d:%02d:00 ",
			m->getYear(), m->getMonth(), m->getDay(),
			m->getHour(), m->getMinute());
	args.push_back(buf);

	// cloud layers
	const char *coverage_string[5] = {
		"clear", "few", "scattered", "broken", "overcast"
	};
	vector<SGMetarCloud> cv = m->getClouds();
	vector<SGMetarCloud>::iterator cloud;
	for (i = 0, cloud = cv.begin(); i < 5; i++) {
		int coverage = 0;
		double altitude = -99999;
		if (cloud != cv.end()) {
			coverage = cloud->getCoverage();
			altitude = coverage ? cloud->getAltitude_ft() + airport_elevation : -99999;
			cloud++;
		}
		sprintf(buf, "--prop:/environment/clouds/layer[%d]/coverage=%s ", i, coverage_string[coverage]);
		args.push_back(buf);
		sprintf(buf, "--prop:/environment/clouds/layer[%d]/elevation-ft=%.0lf ", i, altitude);
		args.push_back(buf);
		sprintf(buf, "--prop:/environment/clouds/layer[%d]/thickness-ft=500 ", i);
		args.push_back(buf);
	}

	// environment (temperature, dewpoint, visibility, pressure)
	// metar sets don't provide aloft information; we have to
	// set the same values for all boundary levels
	int wind_dir = m->getWindDir();
	double visibility = m->getMinVisibility().getVisibility_m();
	double dewpoint = m->getDewpoint_C();
	double temperature = m->getTemperature_C();
	double pressure = m->getPressure_inHg();
	double wind_speed = m->getWindSpeed_kt();
	double elevation = -100;
	for (i = 0; i < 3; i++, elevation += 2000.0) {
		sprintf(buf, "--prop:/environment/config/boundary/entry[%d]/", i);
		int pos = strlen(buf);

		sprintf(&buf[pos], "elevation-ft=%.0lf", elevation);
		args.push_back(buf);
		sprintf(&buf[pos], "turbulence-norm=%.0lf", 0.0);
		args.push_back(buf);

		if (visibility != NaN) {
			sprintf(&buf[pos], "visibility-m=%.0lf", visibility);
			args.push_back(buf);
		}
		if (temperature != NaN) {
			sprintf(&buf[pos], "temperature-degc=%.0lf", temperature);
			args.push_back(buf);
		}
		if (dewpoint != NaN) {
			sprintf(&buf[pos], "dewpoint-degc=%.0lf", dewpoint);
			args.push_back(buf);
		}
		if (pressure != NaN) {
			sprintf(&buf[pos], "pressure-sea-level-inhg=%.0lf", pressure);
			args.push_back(buf);
		}
		if (wind_dir != NaN) {
			sprintf(&buf[pos], "wind-from-heading-deg=%d", wind_dir);
			args.push_back(buf);
		}
		if (wind_speed != NaN) {
			sprintf(&buf[pos], "wind-speed-kt=%.0lf", wind_speed);
			args.push_back(buf);
		}
	}

	// wind dir@speed
	int range_from = m->getWindRangeFrom();
	int range_to = m->getWindRangeTo();
	double gust_speed = m->getGustSpeed_kt();
	if (wind_speed != NaN && wind_dir != -1) {
		strcpy(buf, "--wind=");
		if (range_from != -1 && range_to != -1)
			sprintf(&buf[strlen(buf)], "%d:%d", range_from, range_to);
		else
			sprintf(&buf[strlen(buf)], "%d", wind_dir);
		sprintf(&buf[strlen(buf)], "@%.0lf", wind_speed);
		if (gust_speed != NaN)
			sprintf(&buf[strlen(buf)], ":%.0lf", gust_speed);
		args.push_back(buf);
	}
	

	// output everything
	//cout << "fgfs" << endl;
	vector<string>::iterator arg;
	for (i = 0, arg = args.begin(); arg != args.end(); i++, arg++) {
		cout << "\t" << *arg << endl;
	}
	cout << endl;
#undef NaN
}


void getproxy(string& host, string& port)
{
	host = "";
	port = "80";

	const char *p = getenv("http_proxy");
	if (!p)
		return;
	while (isspace(*p))
		p++;
	if (!strncmp(p, "http://", 7))
		p += 7;
	if (!*p)
		return;

	char s[256], *t;
	strncpy(s, p, 255);
	s[255] = '\0';

	for (t = s + strlen(s); t > s; t--)
		if (!isspace(t[-1]) && t[-1] != '/')
			break;
	*t = '\0';

	t = strchr(s, ':');
	if (t) {
		*t++ = '\0';
		port = t;
	}
	host = s;
}


void usage()
{
	printf(
		"Usage: metar [-v] [-e elevation] [-r|-c] <list of ICAO airport ids or METAR strings>\n"
		"       metar -h\n"
		"\n"
		"       -h|--help            show this help\n"
		"       -v|--verbose         verbose output\n"
		"       -r|--report          print report (default)\n"
		"       -c|--command-line    print command line\n"
		"       -e E|--elevation E   set airport elevation to E meters\n"
		"                            (added to cloud bases in command line mode)\n"
		"Environment:\n"
		"       http_proxy           set proxy in the form \"http://host:port/\"\n"
		"\n"
		"Examples:\n"
		"       $ metar ksfo koak\n"
		"       $ metar -c ksfo -r ksfo\n"
		"       $ metar \"LOWL 161500Z 19004KT 160V240 9999 FEW035 SCT300 29/23 Q1006 NOSIG\"\n"
		"       $ fgfs  `metar -e 183 -c loww`\n"
		"       $ http_proxy=http://localhost:3128/ metar ksfo\n"
		"\n"
	);
}

int main(int argc, char *argv[])
{
	bool report = true;
	bool verbose = false;
	double elevation = 0.0;

	if (argc <= 1) {
		usage();
		return 0;
	}

	string proxy_host, proxy_port;
	getproxy(proxy_host, proxy_port);

  Socket::initSockets();
  
    HTTP::Client http;
    http.setProxy(proxy_host, atoi(proxy_port.c_str()));
    
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
			usage();
		else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose"))
			verbose = true;
		else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--report"))
			report = true;
		else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--command-line"))
			report = false;
		else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--elevation")) {
			if (++i >= argc) {
				cerr << "-e option used without elevation" << endl;
				return 1;
			}
			elevation = strtod(argv[i], 0);
		} else {
			static bool shown = false;
			if (verbose && !shown) {
				cerr << "Proxy host: '" << proxy_host << "'" << endl;
				cerr << "Proxy port: '" << proxy_port << "'" << endl << endl;
				shown = true;
			}

			try {
                MetarRequest* mr = new MetarRequest(argv[i]);
                HTTP::Request_ptr own(mr);
                http.makeRequest(mr);
                
            // spin until the request completes, fails or times out
                SGTimeStamp start(SGTimeStamp::now());
                while (start.elapsedMSec() <  8000) {
                    http.update();
                    if (mr->complete || mr->failed) {
                        break;
                    }
                    SGTimeStamp::sleepForMSec(1);
                }
                
                if (!mr->complete) {
                    throw sg_io_exception("metar download failed (or timed out)");
                }
				SGMetar *m = new SGMetar(mr->metarData);
				
				//SGMetar *m = new SGMetar("2004/01/11 01:20\nLOWG 110120Z AUTO VRB01KT 0050 1600N R35/0600 FG M06/M06 Q1019 88//////\n");

				if (verbose) {
					cerr << G"INPUT: " << m->getData() << ""N << endl;

					const char *unused = m->getUnusedData();
					if (*unused)
						cerr << R"UNUSED: " << unused << ""N << endl;
				}

				if (report)
					printReport(m);
				else
					printArgs(m, elevation);

				delete m;
			} catch (const sg_io_exception& e) {
				cerr << R"ERROR: " << e.getFormattedMessage().c_str() << ""N << endl << endl;
			}
		}
	}
	return 0;
}



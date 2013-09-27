/*
 * fgcom - VoIP-Client for the FlightGear-Radio-Infrastructure
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */

#include <simgear/debug/logstream.hxx>
#include "fgcom.hxx"

#define EARTHRADIUS 6370.0         //radius of earth
#define UF 0.01745329251994329509  //conversion factor pi/180 degree->rad

double
distance (double lat1, double lon1, double lat2, double lon2)
{
  double d;

  d = sin (lat1 * UF) * sin (lat2 * UF);
  d += cos (lat1 * UF) * cos (lat2 * UF) * cos ((lon2 - lon1) * UF);

  return (acos (d) * EARTHRADIUS);
}

void
icao2number (char *icao, float frequency, char *buf)
{
  char icao_work[5];

  if (strlen (icao) == 0)
    strcpy (icao, "ZZZZ");

  sprintf (icao_work, "%4s", icao);
  sprintf (buf, "%02d%02d%02d%02d%02d%06d", DEFAULT_CODE, icao_work[0],
	   icao_work[1], icao_work[2], icao_work[3],
	   (int) (frequency * 1000 + 0.5));
  buf[16] = '\0';
}

void
icao2atisnumber (char *icao, float frequency, char *buf)
{
  char icao_work[5];

  if (strlen (icao) == 0)
    strcpy (icao, "ZZZZ");

  sprintf (icao_work, "%4s", icao);
  sprintf (buf, "%02d%02d%02d%02d%02d%06d", ATIS_CODE, icao_work[0],
          icao_work[1], icao_work[2], icao_work[3],
          (int) (frequency * 1000 + 0.5));
  buf[16] = '\0';
}


const char *
icaobypos (struct airport *airports, double frequency,
	   double plane_lat, double plane_lon, double range)
{
  double r;
  int frq = (int) (frequency * 1000 + 0.5);

  if( (frq%10) !=0 && (frq%5) == 0 ){
    frequency -= 0.005;
    frequency  = ceilf(frequency*1000.0)/1000.0;
  }

  if (frequency >= DEFAULT_LOWER_FRQ_LIMIT
      && frequency <= DEFAULT_UPPER_FRQ_LIMIT)
    {
      while (airports->next != NULL)
	{
	  if ( ceilf(airports->frequency*1000.0)/1000.0 == frequency || airports->frequency == frequency)
	    {
	      r = distance(plane_lat, plane_lon, airports->lat, airports->lon);
              SG_LOG( SG_GENERAL, SG_DEBUG, "icaobypos() - APT: " << airports->text << " (" << airports->icao << " " << airports->type << ")" );
              SG_LOG( SG_GENERAL, SG_DEBUG, "icaobypos() - APT lat: " << airports->lat << " APT lon: " << airports->lon );
              SG_LOG( SG_GENERAL, SG_DEBUG, "icaobypos() - Plane lat: " << plane_lat << " Plane lon: " << plane_lon );
              SG_LOG( SG_GENERAL, SG_DEBUG, "icaobypos() - Distance to " << airports->icao << ": " << r << " Km" );
	      if (r <= range)
		{
                  SG_LOG( SG_GENERAL, SG_ALERT, "Airport " << airports->text << " (" << airports->icao
						<< " " << airports->type << " at " << frequency << " MHz)"
						<< " is in range (" << r << " km)" );
		  return (airports->icao);
		}
	    }
	  airports = airports->next;
	}
      return ("");
    }

  return ("");
}

struct pos
posbyicao (struct airport *airports, char *icao)
{
  struct pos p;

  p.lon = 0.0;
  p.lat = 0.0;

  while (airports->next != NULL)
    {
      if (!strcmp (airports->icao, icao))
	{
          SG_LOG( SG_GENERAL, SG_DEBUG, "posbyicao() - APT: " << airports->text << " (" << airports->icao << " " << airports->type << ")" );
          SG_LOG( SG_GENERAL, SG_DEBUG, "posbyicao() - APT lat: " << airports->lat << " APT lon:" << airports->lon );
	  p.lon = airports->lon;
	  p.lat = airports->lat;
	  return (p);
	}
      airports = airports->next;
    }
  return p;
}

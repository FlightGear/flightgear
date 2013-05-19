#include "fgcom.h"

extern double earthradius;
extern double uf;

double
distance (double lat1, double lon1, double lat2, double lon2)
{
  double d;

  d = sin (lat1 * uf) * sin (lat2 * uf);
  d += cos (lat1 * uf) * cos (lat2 * uf) * cos ((lon2 - lon1) * uf);

  return (acos (d) * earthradius);
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

  if (frequency >= DEFAULT_LOWER_FRQ_LIMIT
      && frequency <= DEFAULT_UPPER_FRQ_LIMIT)
    {
      while (airports->next != NULL)
	{
	  if (airports->frequency == frequency)
	    {
	      r =
		distance (plane_lat, plane_lon, airports->lat, airports->lon);
#ifdef DEBUG
	      printf ("  DEBUG: APT: %s (%s %s)\n", airports->text,
		      airports->icao, airports->type);
	      printf ("  APT lat: %2.6f   APT lon: %2.6f\n",
		      airports->lat, airports->lon);
	      printf ("Plane lat: %2.6f Plane lon: %2.6f\n", plane_lat,
		      plane_lon);
	      printf ("distance to %s: %-4.1f km\n", airports->icao, r);
#endif

	      if (r <= range)
		{
		  printf
		    ("Airport %s (%s %s at %3.3f MHz) is in range (%4.1f km)\n",
		     airports->text, airports->icao, airports->type,
		     frequency, r);
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
#ifdef DEBUG
	  printf ("  DEBUG: APT: %s (%s %s)\n", airports->text,
		  airports->icao, airports->type);
	  printf ("  APT lat: %2.6f   APT lon: %2.6f\n",
		  airports->lat, airports->lon);
#endif
	  p.lon = airports->lon;
	  p.lat = airports->lat;
	  return (p);
	}
      airports = airports->next;
    }
  return p;
}


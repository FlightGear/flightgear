

#ifndef __POSITION_H__
#define __POSITION_H__

/* Compute distance between two points. */
double distance (double lat1, double lon1, double lat2, double lon2);

/* Build the phone number based on the ICAO code and airport frequency. */
void icao2number (char *icao, float frequency, char *buf);
void icao2atisnumber (char *icao, float frequency, char *buf);

/* Search for the closest airport on selected frequency. */
const char *
icaobypos (struct airport *airports, double frequency, double plane_lat,
	   double plane_lon, double range);
#endif

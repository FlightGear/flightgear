#include <math.h>
#include <stdio.h>
#include <time.h>

int main() {
    time_t cur_time, start, start_gmt, now, now_gmt;
    struct tm *gmt, mt;
    long int offset;
    double diff, part, days, hours, lon, lst;
    int i;

    cur_time = time(NULL);

    printf("Time = %ld\n", cur_time);

    gmt = gmtime(&cur_time);

    printf("GMT = %d/%d/%2d %d:%02d:%02d\n", 
           gmt->tm_mon, gmt->tm_mday, gmt->tm_year,
           gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

    mt.tm_mon = 2;
    mt.tm_mday = 21;
    mt.tm_year = 98;
    mt.tm_hour = 12;
    mt.tm_min = 0;
    mt.tm_sec = 0;

    start = mktime(&mt);

    offset = -(timezone / 3600 - daylight);

    printf("Raw time zone offset = %ld\n", timezone);
    printf("Daylight Savings = %d\n", daylight);

    printf("Local hours from GMT = %d\n", offset);

    start_gmt = start - timezone + (daylight * 3600);

    printf("March 21 noon (CST) = %ld\n", start);
    printf("March 21 noon (GMT) = %ld\n", start_gmt);

    for ( i = 0; i < 12; i++ ) {
	mt.tm_mon = i;
	mt.tm_mday = 21;
	mt.tm_year = gmt->tm_year;
	mt.tm_hour = 12;
	mt.tm_min = 0;
	mt.tm_sec = 0;

	now = mktime(&mt);

	offset = -(timezone / 3600 - daylight);

	printf("Raw time zone offset = %ld\n", timezone);
	printf("Daylight Savings = %d\n", daylight);
	
	printf("Local hours from GMT = %d\n", offset);

	now_gmt = now - timezone + (daylight * 3600);

	printf("%d/%d/%d noon (CST) = %ld\n", i+1, 21, 97, now);
	printf("%d/%d/%d noon (GMT) = %ld\n", i+1, 21, 97, now_gmt);

	diff = (now_gmt - start_gmt) / (3600.0 * 24.0);

	printf("Time since 3/21/%2d GMT = %.2f\n", gmt->tm_year, diff);

	part = fmod(diff, 1.0);
	days = diff - part;
	/* hours = gmt->tm_hour + gmt->tm_min/60.0 + gmt->tm_sec/3600.0; */
	hours = 12;
	lon = -112;

	lst = (days + lon)/15.0 + hours - 12;

	while ( lst < 0.0 ) {
	    lst += 24.0;
	}

	printf("days = %.1f  hours = %.2f  lon = %.2f  lst = %.2f\n", 
	       days, hours, lon, lst);
    }

    return(0);
}

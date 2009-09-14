#include <stdio.h>

#include <windows.h>
#include <Windows32/Structures.h>
#include <Windows32/Functions.h>

int main() {
    SYSTEMTIME st;
    TIME_ZONE_INFORMATION tzinfo;
    DWORD t;
    int i;

    GetSystemTime(&st);
    printf("System Time = %d %d %d %d %d %d %d %d\n", 
	   st.wYear,
	   st.wMonth,
	   st.wDayOfWeek,
	   st.wDay,
	   st.wHour,
	   st.wMinute,
	   st.wSecond,
	   st.wMilliseconds
	   );

    t = GetTimeZoneInformation( &tzinfo );

    printf("time zone info return = %d\n", t);

    printf("Bias = %ld\n", tzinfo.Bias);

    printf("Standard Name = ");
    i = 0;
    while ( tzinfo.StandardName[i] != 0 ) {
	printf("%c", tzinfo.StandardName[i]);
	i++;
    }
    printf("\n");

    printf("System Time = %d %d %d %d %d %d %d %d\n", 
	   tzinfo.StandardDate.wYear,
	   tzinfo.StandardDate.wMonth,
	   tzinfo.StandardDate.wDayOfWeek,
	   tzinfo.StandardDate.wDay,
	   tzinfo.StandardDate.wHour,
	   tzinfo.StandardDate.wMinute,
	   tzinfo.StandardDate.wSecond,
	   tzinfo.StandardDate.wMilliseconds
	   );

    printf("Standard Bias = %d\n", tzinfo.StandardBias);

    printf("Daylight Name = ");
    i = 0;
    while ( tzinfo.DaylightName[i] != 0 ) {
	printf("%c", tzinfo.DaylightName[i]);
	i++;
    }
    printf("\n");

    printf("Daylight Date = %d\n", tzinfo.DaylightDate);

    printf("Daylight Bias = %ld\n", tzinfo.DaylightBias);

    return(1);
}

#include "Math.hpp"
#include "Atmosphere.hpp"
namespace yasim {

// Copied from McCormick, who got it from "The ARDC Model Atmosphere"
// Note that there's an error in the text in the first entry,
// McCormick lists 299.16/101325/1.22500, but those don't agree with
// R=287.  I chose to correct the temperature to 288.20, since 79F is
// pretty hot for a "standard" atmosphere.
//                             meters   kelvin     kPa   kg/m^3
float Atmosphere::data[][4] = {{ 0,     288.20, 101325, 1.22500 },
			       { 900,   282.31,  90971, 1.12260 },
			       { 1800,  276.46,  81494, 1.02690 },
			       { 2700,  270.62,  72835, 0.93765 },
			       { 3600,  264.77,  64939, 0.85445 },
			       { 4500,  258.93,  57752, 0.77704 },
			       { 5400,  253.09,  51226, 0.70513 },
			       { 6300,  247.25,  45311, 0.63845 },
			       { 7200,  241.41,  39963, 0.57671 },
			       { 8100,  235.58,  35140, 0.51967 },
			       { 9000,  229.74,  30800, 0.46706 },
			       { 9900,  223.91,  26906, 0.41864 },
			       { 10800, 218.08,  23422, 0.37417 },
			       { 11700, 216.66,  20335, 0.32699 },
			       { 12600, 216.66,  17654, 0.28388 },
			       { 13500, 216.66,  15327, 0.24646 },
			       { 14400, 216.66,  13308, 0.21399 },
			       { 15300, 216.66,  11555, 0.18580 },
			       { 16200, 216.66,  10033, 0.16133 },
			       { 17100, 216.66,   8712, 0.14009 },
			       { 18000, 216.66,   7565, 0.12165 },
			       { 18900, 216.66,   6570, 0.10564 }};

float Atmosphere::getStdTemperature(float alt)
{
    return getRecord(alt, 1);
}

float Atmosphere::getStdPressure(float alt)
{
    return getRecord(alt, 2);
}

float Atmosphere::getStdDensity(float alt)
{
    return getRecord(alt, 3);
}

float Atmosphere::calcVEAS(float spd, float pressure, float temp)
{
    return 0; //FIXME
}

float Atmosphere::calcVCAS(float spd, float pressure, float temp)
{
    // Stolen shamelessly from JSBSim.  Constants that appear:
    //   2/5  == gamma-1
    //   5/12 == 1/(gamma+1)
    //   4/5  == 2*(gamma-1)
    //  14/5  == 2*gamma
    //  28/5  == 4*gamma
    // 144/25 == (gamma+1)^2

    float m2 = calcMach(spd, temp);
    m2 = m2*m2; // mach^2

    float cp; // pressure coefficient
    if(m2 < 1) {
        // (1+(mach^2)/5)^(gamma/(gamma-1))
        cp = Math::pow(1+0.2*m2, 3.5);
    } else {
        float tmp0 = ((144/25.) * m2) / (28/5.*m2 - 4/5.);
        float tmp1 = ((14/5.) * m2 - (2/5.)) * (5/12.);
        cp = Math::pow(tmp0, 3.5) * tmp1;
    }

    // Conditions at sea level
    float p0 = getStdPressure(0);
    float rho0 = getStdDensity(0);

    float tmp = Math::pow((pressure/p0)*(cp-1) + 1, (2/7.));
    return Math::sqrt((7*p0/rho0)*(tmp-1));
}

float Atmosphere::calcDensity(float pressure, float temp)
{
    // P = rho*R*T, R == 287 kPa*m^3 per kg*kelvin for air
    return pressure / (287 * temp);
}

float Atmosphere::calcMach(float spd, float temp)
{
    return spd / Math::sqrt(1.4 * 287 * temp);
}

float Atmosphere::getRecord(float alt, int recNum)
{
    int hi = (sizeof(data) / (4*sizeof(float))) - 1;
    int lo = 0;

    // safety valve, clamp to the edges of the table
    if(alt < data[0][0])       hi=1;
    else if(alt > data[hi][0]) lo = hi-1;

    // binary search
    while(1) {
	if(hi-lo == 1) break;
	int mid = (hi+lo)>>1;
	if(alt < data[mid][0]) hi = mid;
	else lo = mid;
    }

    // interpolate
    float frac = (alt - data[lo][0])/(data[hi][0] - data[lo][0]);
    float a = data[lo][recNum];
    float b = data[hi][recNum];
    return a + frac * (b-a);
}

}; // namespace yasim

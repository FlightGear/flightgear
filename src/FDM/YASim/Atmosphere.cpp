#include "Math.hpp"
#include "Atmosphere.hpp"
namespace yasim {

// Copied from McCormick, who got it from "The ARDC Model Atmosphere"
// Note that there's an error in the text in the first entry,
// McCormick lists 299.16/101325/1.22500, but those don't agree with
// R=287.  I chose to correct the temperature to 288.20, since 79F is
// pretty hot for a "standard" atmosphere.
// Numbers above 19000 meters calculated from src/Environment/environment.cxx
//                             meters   kelvin      Pa   kg/m^3
float Atmosphere::data[][4] = {{ -900.0f, 293.91f, 111679.0f, 1.32353f },
                               {    0.0f, 288.11f, 101325.0f, 1.22500f },
			       {   900.0f, 282.31f,  90971.0f, 1.12260f },
			       {  1800.0f, 276.46f,  81494.0f, 1.02690f },
			       {  2700.0f, 270.62f,  72835.0f, 0.93765f },
			       {  3600.0f, 264.77f,  64939.0f, 0.85445f },
			       {  4500.0f, 258.93f,  57752.0f, 0.77704f },
			       {  5400.0f, 253.09f,  51226.0f, 0.70513f },
			       {  6300.0f, 247.25f,  45311.0f, 0.63845f },
			       {  7200.0f, 241.41f,  39963.0f, 0.57671f },
			       {  8100.0f, 235.58f,  35140.0f, 0.51967f },
			       {  9000.0f, 229.74f,  30800.0f, 0.46706f },
			       {  9900.0f, 223.91f,  26906.0f, 0.41864f },
			       { 10800.0f, 218.08f,  23422.0f, 0.37417f },
			       { 11700.0f, 216.66f,  20335.0f, 0.32699f },
			       { 12600.0f, 216.66f,  17654.0f, 0.28388f },
			       { 13500.0f, 216.66f,  15327.0f, 0.24646f },
			       { 14400.0f, 216.66f,  13308.0f, 0.21399f },
			       { 15300.0f, 216.66f,  11555.0f, 0.18580f },
			       { 16200.0f, 216.66f,  10033.0f, 0.16133f },
			       { 17100.0f, 216.66f,   8712.0f, 0.14009f },
			       { 18000.0f, 216.66f,   7565.0f, 0.12165f },
                               {18900.0f, 216.66f,   6570.0f, 0.10564f },
                               {19812.0f, 216.66f,   5644.0f, 0.09073f },
                               {20726.0f, 217.23f,   4884.0f, 0.07831f },
                               {21641.0f, 218.39f,   4235.0f, 0.06755f },
                               {22555.0f, 219.25f,   3668.0f, 0.05827f },
                               {23470.0f, 220.12f,   3182.0f, 0.05035f },
                               {24384.0f, 220.98f,   2766.0f, 0.04360f },
                               {25298.0f, 221.84f,   2401.0f, 0.03770f },
                               {26213.0f, 222.71f,   2087.0f, 0.03265f },
                               {27127.0f, 223.86f,   1814.0f, 0.02822f },
                               {28042.0f, 224.73f,   1581.0f, 0.02450f },
                               {28956.0f, 225.59f,   1368.0f, 0.02112f },
                               {29870.0f, 226.45f,   1196.0f, 0.01839f },
                               {30785.0f, 227.32f,   1044.0f, 0.01599f }};

// Universal gas constant for air, in SI units.  P = R * rho * T.
// P in pascals (N/m^2), rho is kg/m^3, T in kelvin.
const float R = 287.1f;

// Specific heat ratio for air, at "low" temperatures.  
const float GAMMA = 1.4f;

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

float Atmosphere::calcVEAS(float spd,
                           float pressure, float temp, float density)
{
    static float rho0 = getStdDensity(0);
    float densityRatio = density / rho0;
    return spd * Math::sqrt(densityRatio);
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
        float tmp0 = ((144.0f/25.0f) * m2) / (28.0f/5.0f*m2 - 4.0f/5.0f);
        float tmp1 = ((14.0f/5.0f) * m2 - (2.0f/5.0f)) * (5.0f/12.0f);
        cp = Math::pow(tmp0, 3.5) * tmp1;
    }

    // Conditions at sea level
    float p0 = getStdPressure(0);
    float rho0 = getStdDensity(0);

    float tmp = Math::pow((pressure/p0)*(cp-1) + 1, (2/7.));
    return Math::sqrt((7*p0/rho0)*(tmp-1));
}

float Atmosphere::calcStdDensity(float pressure, float temp)
{
    return pressure / (R * temp);
}

float Atmosphere::calcMach(float spd, float temp)
{
    return spd / Math::sqrt(GAMMA * R * temp);
}

float Atmosphere::spdFromMach(float mach, float temp)
{
    return mach * Math::sqrt(GAMMA * R * temp);
}

float Atmosphere::spdFromVCAS(float vcas, float pressure, float temp)
{
                                // FIXME: does not account for supersonic
    float p0 = getStdPressure(0);
    float rho0 = getStdDensity(0);

    float tmp = (vcas*vcas)/(7*p0/rho0) + 1;
    float cp = ((Math::pow(tmp,(7/2.))-1)/(pressure/p0)) + 1;

    float m2 = (Math::pow(cp,(1/3.5))-1)/0.2;
    float vtas= spdFromMach(Math::sqrt(m2), temp);
    return vtas;
}

void Atmosphere::calcStaticAir(float p0, float t0, float d0, float v,
                               float* pOut, float* tOut, float* dOut)
{
    const static float C0 = ((GAMMA-1)/(2*R*GAMMA));
    const static float C1 = 1/(GAMMA-1);

    *tOut = t0 + (v*v) * C0;
    *dOut = d0 * Math::pow(*tOut / t0, C1);
    *pOut = (*dOut) * R * (*tOut);
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

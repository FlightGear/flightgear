#ifndef _ATMOSPHERE_HPP
#define _ATMOSPHERE_HPP

namespace yasim {

class Atmosphere {
public:
    static float getStdTemperature(float alt);
    static float getStdPressure(float alt);
    static float getStdDensity(float alt);

    static float calcVCAS(float spd, float pressure, float temp);
    static float calcVEAS(float spd, float pressure, float temp);
    static float calcMach(float spd, float temp);
    static float calcDensity(float pressure, float temp);

private:
    static float getRecord(float alt, int idx);
    static float data[][4];
};

}; // namespace yasim
#endif // _ATMOSPHERE_HPP

#ifndef _ATMOSPHERE_HPP
#define _ATMOSPHERE_HPP

namespace yasim {

class Atmosphere {
public:
    static float getStdTemperature(float alt);
    static float getStdPressure(float alt);
    static float getStdDensity(float alt);

    static float calcVCAS(float spd, float pressure, float temp);
    static float calcVEAS(float spd, float pressure, float temp, float density);
    static float calcMach(float spd, float temp);
    static float calcStdDensity(float pressure, float temp);

    static float spdFromMach(float mach, float temp);
    static float spdFromVCAS(float vcas, float pressure, float temp);
    
    // Given ambient ("0") pressure/density/temperature values,
    // calculate the properties of static air (air accelerated to the
    // aircraft's speed) at a given velocity.  Includes
    // compressibility, but not shock effects.
    static void calcStaticAir(float p0, float t0, float d0, float v,
                              float* pOut, float* tOut, float* dOut);

private:
    static float getRecord(float alt, int idx);
    static float data[][4];
};

}; // namespace yasim
#endif // _ATMOSPHERE_HPP

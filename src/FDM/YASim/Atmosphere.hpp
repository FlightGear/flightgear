#ifndef _ATMOSPHERE_HPP
#define _ATMOSPHERE_HPP

namespace yasim {

//constexpr int Atmosphere::numColumns {4};
  
class Atmosphere {
    enum Column {
      ALTITUDE,
      TEMPERATURE,
      PRESSURE,
      DENSITY
    };
    static const int numColumns {4};
public:
    void setTemperature(float t) { _temperature = t; }
    void setPressure(float p) { _pressure = p; }
    void setDensity(float d) { _density = d; }

    //set temperature, pressure and density to standard values for given altitude
    void setStandard(float altitude);
    
    float getTemperature() const { return _temperature; }
    float getPressure() const { return _pressure; }
    float getDensity() const { return _density; }
    
    static float getStdTemperature(float alt);
    static float getStdPressure(float alt);
    static float getStdDensity(float alt);

    static float calcVCAS(float spd, float pressure, float temp);
    static float calcVEAS(float spd, float pressure, float temp, float density);
    static float calcMach(float spd, float temp);
    static float calcStdDensity(float pressure, float temp);

    static float spdFromMach(float mach, float temp);
    static float spdFromVCAS(float vcas, float pressure, float temp);
    float spdFromMach(float mach);
    float spdFromVCAS(float vcas);
    
    // Given ambient ("0") pressure/density/temperature values,
    // calculate the properties of static air (air accelerated to the
    // aircraft's speed) at a given velocity.  Includes
    // compressibility, but not shock effects.
    static void calcStaticAir(float p0, float t0, float d0, float v,
                              float* pOut, float* tOut, float* dOut);
    void calcStaticAir(float v, float* pOut, float* tOut, float* dOut);
    static bool test();
    
private:
    static float getRecord(float alt, Atmosphere::Column recNum);
    static float data[][numColumns];
    static int maxTableIndex();
    
    float _temperature = 288.11f;
    float _pressure = 101325.0f;
    float _density = 1.22500f;
};

}; // namespace yasim
#endif // _ATMOSPHERE_HPP

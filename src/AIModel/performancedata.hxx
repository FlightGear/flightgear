#ifndef PERFORMANCEDATA_HXX
#define PERFORMANCEDATA_HXX

#include <string>
#include <map>

class FGAIAircraft;

/**
Data storage for aircraft performance data. This is used to properly simulate the flight of AIAircrafts.
 
	@author Thomas F�rster <t.foerster@biologie.hu-berlin.de>
*/
class PerformanceData
{
public:
    PerformanceData(double acceleration,
                    double deceleration,
                    double climbRate,
                    double descentRate,
                    double vRotate,
                    double vTakeOff,
                    double vClimb,
                    double vCruise,
                    double vDescent,
                    double vApproach,
                    double vTouchdown,
                    double vTaxi);
    PerformanceData(const std::string& filename);
    ~PerformanceData();

    double actualSpeed(FGAIAircraft* ac, double tgt_speed, double dt, bool needMaxBrake);
    double actualBankAngle(FGAIAircraft* ac, double tgt_roll, double dt);
    double actualPitch(FGAIAircraft* ac, double tgt_pitch, double dt);
    double actualHeading(FGAIAircraft* ac, double tgt_heading, double dt);
    double actualAltitude(FGAIAircraft* ac, double tgt_altitude, double dt);
    double actualVerticalSpeed(FGAIAircraft* ac, double tgt_vs, double dt);

    bool gearExtensible(const FGAIAircraft* ac);

    inline double climbRate        () { return _climbRate; };
    inline double descentRate      () { return _descentRate; };
    inline double vRotate          () { return _vRotate; };
    inline double maximumBankAngle () { return _maxbank; };
    inline double acceleration     () { return _acceleration; };
    inline double deceleration     () { return _deceleration; };
    inline double vTaxi            () { return _vTaxi; };
    inline double vTakeoff         () { return _vTakeOff; };
    inline double vClimb           () { return _vClimb; };
    inline double vDescent         () { return _vDescent; };
    inline double vApproach        () { return _vApproach; };
    inline double vTouchdown       () { return _vTouchdown; };
    inline double vCruise          () { return _vCruise; };
    
private:
    double _acceleration;
    double _deceleration;
    double _climbRate;
    double _descentRate;
    double _vRotate;
    double _vTakeOff;
    double _vClimb;
    double _vCruise;
    double _vDescent;
    double _vApproach;
    double _vTouchdown;
    double _vTaxi;

    double _rollrate;
    double _maxbank;
};

#endif

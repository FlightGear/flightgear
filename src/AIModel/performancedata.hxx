#ifndef PERFORMANCEDATA_HXX
#define PERFORMANCEDATA_HXX

class FGAIAircraft;
class SGPropertyNode;

/**
Data storage for aircraft performance data. This is used to properly simulate the flight of AIAircrafts.
 
	@author Thomas F�rster <t.foerster@biologie.hu-berlin.de>
*/
class PerformanceData
{
public:
    PerformanceData();
    
    PerformanceData(PerformanceData* clone);
  
    void initFromProps(SGPropertyNode* props);

    ~PerformanceData() = default;

    double actualSpeed(FGAIAircraft* ac, double tgt_speed, double dt, bool needMaxBrake);
    double actualBankAngle(FGAIAircraft* ac, double tgt_roll, double dt);
    double actualPitch(FGAIAircraft* ac, double tgt_pitch, double dt);
    double actualHeading(FGAIAircraft* ac, double tgt_heading, double dt);
    double actualAltitude(FGAIAircraft* ac, double tgt_altitude, double dt);
    double actualVerticalSpeed(FGAIAircraft* ac, double tgt_vs, double dt);

    bool gearExtensible(const FGAIAircraft* ac);

    double climbRate() const { return _climbRate; };
    double descentRate() const { return _descentRate; };
    double vRotate() const { return _vRotate; };
    double maximumBankAngle() const { return _maxbank; };
    double acceleration() const { return _acceleration; };
    double deceleration() const { return _deceleration; };
    double vTaxi() const { return _vTaxi; };
    double vTakeoff() const { return _vTakeOff; };
    double vClimb() const { return _vClimb; };
    double vDescent() const { return _vDescent; };
    double vApproach() const { return _vApproach; };
    double vTouchdown() const { return _vTouchdown; };
    double vCruise() const { return _vCruise; };
    double wingSpan() const { return _wingSpan; };
    double wingChord() const { return _wingChord; };
    double weight() const { return _weight; };

    double decelerationOnGround() const;

    /**
     @brief Last-resort fallback performance data. This is to avoid special-casing
     logic in the AIAircraft code, by ensuring we always have a valid _performance pointer.
     */
    static PerformanceData* getDefaultData();

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

    // Data for aerodynamic wake computation
    double _wingSpan;
    double _wingChord;
    double _weight;
};

#endif

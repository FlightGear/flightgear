#ifndef _FGWIND_HXX

#include <simgear/structure/SGReferenced.hxx>

////////////////////////////////////////////////////////////////////////
// A Wind Modulator interface, generates gusts and wind direction changes
////////////////////////////////////////////////////////////////////////
class FGWindModulator : public SGReferenced {
public:
	FGWindModulator();
	virtual ~FGWindModulator();
	virtual void update( double dt ) = 0;
	double get_direction_offset_norm() const { return direction_offset_norm; }
	double get_magnitude_factor_norm() const { return magnitude_factor_norm; }
protected:
	double direction_offset_norm;
	double magnitude_factor_norm;
};


////////////////////////////////////////////////////////////////////////
// A Basic Wind Modulator, implementation of FGWindModulator
// direction and magnitude variations are based on simple sin functions
////////////////////////////////////////////////////////////////////////
class FGBasicWindModulator : public FGWindModulator {
public:
	FGBasicWindModulator();
	virtual ~FGBasicWindModulator();
	virtual void update( double dt );
	void set_direction_period( double _direction_period ) { direction_period = _direction_period; }
	double get_direction_period() const { return direction_period; }
	void set_speed_period( double _speed_period ) { speed_period = _speed_period; }
	double get_speed_period() const{ return speed_period; }
private:
	double elapsed;
	double direction_period;
	double speed_period;
};

#endif

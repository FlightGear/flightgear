// terrainsampler.cxx --
//
// Written by Torsten Dreyer, started July 2010
// Based on local weather implementation in nasal from 
// Thorsten Renk
//
// Copyright (C) 2010  Curtis Olson
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <Main/fg_props.hxx>
#include <simgear/math/sg_random.h>
#include <Scenery/scenery.hxx>
#include <deque>

#include "terrainsampler.hxx"
using simgear::PropertyList;

#include "tiedpropertylist.hxx"

namespace Environment {
/**
 * @brief Class for presampling the terrain roughness
 */
class AreaSampler : public SGSubsystem {
public:
	AreaSampler( SGPropertyNode_ptr rootNode );
	virtual ~AreaSampler();
	void update( double dt );
    void bind();
    void unbind();

    double getOrientationDeg() const { return _orientation_rad * SG_RADIANS_TO_DEGREES; }
    void setOrientationDeg( double value ) { _orientation_rad = value * SG_DEGREES_TO_RADIANS; }

    int getElevationHistogramStep() const { return _elevationHistogramStep; }
    void setElevationHistograpStep( int value ) { 
        _elevationHistogramStep = value > 0 ? value : 500;
        _elevationHistogramCount = _elevationHistogramMax / _elevationHistogramStep;
    }

    int getElevationHistogramMax() const { return _elevationHistogramMax; }
    void setElevationHistograpMax( int value ) { 
        _elevationHistogramMax = value > 0 ? value : 10000;
        _elevationHistogramCount = _elevationHistogramMax / _elevationHistogramStep;
    }

    int getElevationHistogramCount() const { return _elevationHistogramCount; }

private:
    void analyse();

    SGPropertyNode_ptr _rootNode;

    bool _enabled;
    bool _useAircraftPosition;
	double _latitude_deg;
	double _longitude_deg;
	double _orientation_rad;
	int _radius;
	int _samples_per_frame;
    int _max_samples;    // keep xx samples in queue for analysis
    int _analyze_every;  // Run analysis every xx samples
    int _elevationHistogramMax;
    int _elevationHistogramStep;
    int _elevationHistogramCount;

    double _altOffset;
    double _altMedian;
    double _altMin;
    double _altLayered;
    double _altMean;

    SGPropertyNode_ptr _positionLatitudeNode;
    SGPropertyNode_ptr _positionLongitudeNode;

    deque<double> elevations;
    TiedPropertyList _tiedProperties;
};

AreaSampler::AreaSampler( SGPropertyNode_ptr rootNode ) :
    _rootNode(rootNode),
    _enabled(true),
    _useAircraftPosition(false),
    _latitude_deg(0.0),
	_longitude_deg(0.0),
	_orientation_rad(0.0),
	_radius(40000.0),
	_samples_per_frame(5),
    _max_samples(1000),
    _analyze_every(200),
    _elevationHistogramMax(10000),
    _elevationHistogramStep(500),
    _elevationHistogramCount(_elevationHistogramMax/_elevationHistogramStep),
    _altOffset(0),
    _altMedian(0),
    _altMin(0),
    _altLayered(0),
    _altMean(0)
{
    _positionLatitudeNode = fgGetNode( "/position/latitude-deg", true );
    _positionLongitudeNode = fgGetNode( "/position/longitude-deg", true );
}

AreaSampler::~AreaSampler()
{
}


void AreaSampler::bind()
{
    _tiedProperties.setRoot( _rootNode );
    _tiedProperties.Tie( "enabled", &_enabled );

    _tiedProperties.setRoot( _rootNode->getNode( "input", true ) );
    _tiedProperties.Tie( "use-aircraft-position", &_useAircraftPosition );
    _tiedProperties.Tie( "latitude-deg", &_latitude_deg );
    _tiedProperties.Tie( "latitude-deg", &_latitude_deg );
    _tiedProperties.Tie( "longitude-deg", &_longitude_deg );
    _tiedProperties.Tie( "orientation-deg", this, &AreaSampler::getOrientationDeg, &AreaSampler::setOrientationDeg );
    _tiedProperties.Tie( "radius-m", &_radius );
    _tiedProperties.Tie( "max-samples-per-frame", &_samples_per_frame );
    _tiedProperties.Tie( "max-samples", &_max_samples );
    _tiedProperties.Tie( "analyse-every", &_analyze_every );
    _tiedProperties.Tie( "elevation-histogram-max-ft", this, &AreaSampler::getElevationHistogramMax, &AreaSampler::setElevationHistograpMax );
    _tiedProperties.Tie( "elevation-histogram-step-ft", this, &AreaSampler::getElevationHistogramStep, &AreaSampler::setElevationHistograpStep );
    _tiedProperties.Tie( "elevation-histogram-count", this, &AreaSampler::getElevationHistogramCount );

    _tiedProperties.setRoot( _rootNode->getNode( "output", true ) );
    _tiedProperties.Tie( "alt-offset-ft", &_altOffset );
    _tiedProperties.Tie( "alt-median-ft", &_altMedian );
    _tiedProperties.Tie( "alt-min-ft", &_altMin );
    _tiedProperties.Tie( "alt-layered-ft", &_altLayered );
    _tiedProperties.Tie( "alt-mean-ft", &_altMean );
}

void AreaSampler::unbind()
{
    _tiedProperties.Untie();
}

void AreaSampler::update( double dt )
{
	if( !(_enabled && dt > SGLimitsd::min()) )
        return;

    if( _useAircraftPosition ) {
        _longitude_deg = _positionLongitudeNode->getDoubleValue();
        _latitude_deg = _positionLatitudeNode->getDoubleValue();
    }

    SGGeoc center = SGGeoc::fromGeod( SGGeod::fromDegM( _longitude_deg, _latitude_deg, SG_MAX_ELEVATION_M ) );

    FGScenery * scenery = globals->get_scenery();
	for( int i = 0; 
		i < _samples_per_frame; 
		i++ ) {

		double distance = sg_random() * _radius;
		double course = sg_random() * 2.0 * SG_PI;
		SGGeod probe = SGGeod::fromGeoc(center.advanceRadM( course, distance ));
		double elevation_m = 0.0;
	    if (scenery->get_elevation_m( probe, elevation_m, NULL ))
            elevations.push_front(elevation_m *= SG_METER_TO_FEET);
        
        if( elevations.size() >= (deque<unsigned>::size_type)_max_samples ) {
            analyse();
            elevations.resize( _max_samples - _analyze_every );
        }
	}
}

void AreaSampler::analyse()
{
    double sum;

	vector<int> histogram(_elevationHistogramCount,0);
    
    for( deque<double>::size_type i = 0; i < elevations.size(); i++ ) {
        int idx = SGMisc<int>::clip( (int)(elevations[i]/_elevationHistogramStep), 0, histogram.size()-1 );
        histogram[idx]++;
    }

    _altMedian = 0.0;
    sum = 0.0;
    for( vector<int>::size_type i = 0; i < histogram.size(); i++ ) {
        sum += histogram[i];
        if( sum > 0.5 * elevations.size() ) {
            _altMedian = i * _elevationHistogramStep;
            break;
        }
    }

    _altOffset = 0.0;
    sum = 0.0;
    for(  vector<int>::size_type i = 0; i < histogram.size(); i++ ) {
        sum += histogram[i];
        if( sum > 0.3 * elevations.size() ) {
            _altOffset = i * _elevationHistogramStep;
            break;
        }
    }

   _altMean = 0.0;
    for(  vector<int>::size_type i = 0; i < histogram.size(); i++ ) {
        _altMean += histogram[i] * i;
    }
    _altMean *= _elevationHistogramStep;
    if( elevations.size() != 0.0 ) _altMean /= elevations.size();

    _altMin = 0.0;
    for(  vector<int>::size_type i = 0; i < histogram.size(); i++ ) {
        if( histogram[i] > 0 ) {
            _altMin = i * _elevationHistogramStep;
            break;
        }
    }

    double alt_low_min = 0.0;
    double n_max = 0.0;
    sum = 0.0;
    for(  vector<int>::size_type i = 0; i < histogram.size()-1; i++ ) {
        sum += histogram[i];
        if( histogram[i] > n_max ) n_max = histogram[i];
        if( n_max > histogram[i+1] && sum > 0.3*elevations.size()) {
            alt_low_min = i * _elevationHistogramStep;
            break;
        }
    }

    _altLayered = 0.5 * (_altMin + _altOffset);

#if 0
    SG_LOG( SG_ALL, SG_ALERT, "TerrainPresampler - alalysis results:" <<
        " total:" << elevations.size() <<
        " mean:" << _altMean <<
        " median:" << _altMedian <<
        " min:" << _altMin <<
        " alt_20:" << _altOffset );
#endif
#if 0
append(alt_50_array, alt_med);
#endif
}

/* --------------------- End of AreaSampler implementation ------------- */

/* --------------------- TerrainSamplerImplementation -------------------------- */

class TerrainSamplerImplementation : public TerrainSampler
{
public:
	TerrainSamplerImplementation ( SGPropertyNode_ptr rootNode );
	virtual ~TerrainSamplerImplementation ();
	
	virtual void init ();
	virtual void reinit ();
    virtual void bind();
    virtual void unbind();
	virtual void update (double delta_time_sec);
private:
    inline string areaSubsystemName( unsigned i ) {
      ostringstream name;
      name <<  "area" << i;
      return name.str();
    }

	SGPropertyNode_ptr _rootNode;
    bool _enabled;
    TiedPropertyList _tiedProperties;
};

TerrainSamplerImplementation::TerrainSamplerImplementation( SGPropertyNode_ptr rootNode ) :
    _rootNode( rootNode ),
    _enabled(true)
{
}

TerrainSamplerImplementation::~TerrainSamplerImplementation()
{
}

void TerrainSamplerImplementation::init()
{
    PropertyList areaNodes = _rootNode->getChildren( "area" );
    
    for( PropertyList::size_type i = 0; i < areaNodes.size(); i++ )
        set_subsystem( areaSubsystemName(i), new AreaSampler( areaNodes[i] ) );

    SGSubsystemGroup::bind();// bind the subsystems before the get init()ed
    SGSubsystemGroup::init();
}

void TerrainSamplerImplementation::reinit()
{
    for( unsigned i = 0;; i++ ) {
        string subsystemName = areaSubsystemName(i);
        SGSubsystem * subsys = get_subsystem( subsystemName );
        if( subsys == NULL )
            break;
        remove_subsystem( subsystemName );
    }
    
    init();
}

void TerrainSamplerImplementation::bind()
{
    SGSubsystemGroup::bind();
    _tiedProperties.Tie( _rootNode->getNode("enabled",true), &_enabled );
}

void TerrainSamplerImplementation::unbind()
{
    _tiedProperties.Untie();
    SGSubsystemGroup::unbind();
}

void TerrainSamplerImplementation::update( double dt )
{
	if( !(_enabled && dt > SGLimitsd::min()) )
		return;
    SGSubsystemGroup::update(dt);
}

/* ----------------------------------------------------------------------- */

/* implementation of the TerrainSampler factory to hide the implementation
   details */
TerrainSampler::~TerrainSampler ()
{
}

TerrainSampler * TerrainSampler::createInstance( SGPropertyNode_ptr rootNode )
{
	return new TerrainSamplerImplementation( rootNode );
}

} // namespace


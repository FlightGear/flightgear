// MRG.cxx - an electrically powered master reference gyro.
// Written by Vivian Meazza based on work by David Megginson, started 2006.
//
// This file is in the Public Domain and comes with no warranty.

// TODO:
// - better spin-up

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <simgear/compiler.h>
#include <simgear/sg_inlines.h>
#include <simgear/math/SGMath.hxx>

#include <iostream>
#include <string>
#include <sstream>
#include <math.h>	// fabs()

#include <Main/fg_props.hxx>
#include <Main/util.hxx>

#include "mrg.hxx"

const double MasterReferenceGyro::gravity = -32.1740485564;

using std::string;

MasterReferenceGyro::MasterReferenceGyro ( SGPropertyNode *node ) :
_name(node->getStringValue("name", "master-reference-gyro")),
_num(node->getIntValue("number", 0))
{
}

MasterReferenceGyro::~MasterReferenceGyro ()
{
}

void
MasterReferenceGyro::init ()
{
    _last_hdg = 0;
    _last_roll = 0;
    _last_pitch = 0;
    _indicated_hdg = 0;
    _indicated_roll = 0;
    _indicated_pitch = 0;
    _indicated_hdg_rate = 0;
    _indicated_roll_rate = 0;
    _indicated_pitch_rate = 0;
    _erect_time = 180;
    _last_g = 1;
    _g_error = 10;

    string branch;
    branch = "/instrumentation/" + _name;

    _pitch_in_node = fgGetNode("/orientation/pitch-deg", true);
    _roll_in_node = fgGetNode("/orientation/roll-deg", true);
    _hdg_in_node = fgGetNode("/orientation/heading-deg", true);
    _hdg_mag_in_node = fgGetNode("/orientation/heading-magnetic-deg", true);
    _pitch_rate_node = fgGetNode("/orientation/pitch-rate-degps", true);
    _roll_rate_node = fgGetNode("/orientation/roll-rate-degps", true);
    _yaw_rate_node = fgGetNode("/orientation/yaw-rate-degps", true);
    //_g_in_node =   fgGetNode("/accelerations/pilot/z-accel-fps_sec", true);
    _g_in_node =   fgGetNode("/accelerations/pilot-g", true);
    _electrical_node = fgGetNode("/systems/electrical/outputs/MRG", true);
    _hdg_mag_in_node = fgGetNode("/orientation/heading-magnetic-deg", true);

    SGPropertyNode *node = fgGetNode(branch.c_str(), _num, true );
    _off_node = node->getChild("off-flag", 0, true);
    _pitch_out_node = node->getChild("indicated-pitch-deg", 0, true);
    _roll_out_node = node->getChild("indicated-roll-deg", 0, true);
    _hdg_out_node = node->getChild("indicated-hdg-deg", 0, true);
    _hdg_mag_out_node = node->getChild("indicated-mag-hdg-deg", 0, true);
    _pitch_rate_out_node = node->getChild("indicated-pitch-rate-degps", 0, true);
    _roll_rate_out_node = node->getChild("indicated-roll-rate-degps", 0, true);
    _hdg_rate_out_node = node->getChild("indicated-hdg-rate-degps", 0, true);
    _responsiveness_node = node->getChild("responsiveness", 0, true);
    _error_out_node = node->getChild("heading-bug-error-deg", 0, true);
    _hdg_input_source_node = node->getChild("heading-source", 0, true);
    _fast_erect_node = node->getChild("fast-erect", 0, true);

    _electrical_node->setDoubleValue(0);
    _responsiveness_node->setDoubleValue(0.75);
    _off_node->setBoolValue(false);
    _hdg_input_source_node->setBoolValue(false);
    _fast_erect_node->setBoolValue(false);
    _g_in_node->setDoubleValue(1);
}

void
MasterReferenceGyro::bind ()
{
    std::ostringstream temp;
    string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";

    fgTie((branch + "/serviceable").c_str(),
        &_gyro, &Gyro::is_serviceable, &Gyro::set_serviceable);
    fgTie((branch + "/spin").c_str(),
        &_gyro, &Gyro::get_spin_norm, &Gyro::set_spin_norm);
}

void
MasterReferenceGyro::unbind ()
{
    std::ostringstream temp;
    string branch;
    temp << _num;
    branch = "/instrumentation/" + _name + "[" + temp.str() + "]";

    fgUntie((branch + "/serviceable").c_str());
    fgUntie((branch + "/spin").c_str());
}

void
MasterReferenceGyro::update (double dt)
{
    //sanity check
    if (!fgGetBool("/sim/fdm-initialized", false)) {
        return;
    }

    double indicated_roll = 0;
    double indicated_pitch = 0;
    double indicated_hdg = 0;
    double indicated_roll_rate = 0;
    double indicated_pitch_rate = 0;
    double indicated_hdg_rate = 0;
    double hdg = 0;
    double erect_time_factor = 1;

    const double erect_time = 180;
    const double max_g_error = 10.0;

    //Get the spin from the gyro
    _gyro.set_power_norm( _electrical_node->getDoubleValue()/24 );
    _gyro.update(dt);
    double spin = _gyro.get_spin_norm();

    // set the "off-flag"
    if ( _electrical_node->getDoubleValue() > 0 && spin >= 0.25) {
        _off_node->setBoolValue(false);
    } else {
        _off_node->setBoolValue(true);
        return;
    }

    // Get the input values
    if(_hdg_input_source_node->getBoolValue()){
        hdg = _hdg_in_node->getDoubleValue();
    } else {
        hdg = _hdg_mag_in_node->getDoubleValue();
    }

    double roll = _roll_in_node->getDoubleValue();
    double pitch = _pitch_in_node->getDoubleValue();
    double g = _g_in_node->getDoubleValue()/* / gravity*/;

    double roll_rate = _yaw_rate_node->getDoubleValue();
    double pitch_rate = _pitch_rate_node->getDoubleValue();
    double yaw_rate = _yaw_rate_node->getDoubleValue();

    //modulate the input by the spin rate
    double responsiveness = spin * spin * spin * spin * spin * spin;
    roll = fgGetLowPass( _last_roll, roll, responsiveness );
    pitch = fgGetLowPass( _last_pitch , pitch, responsiveness );

    if ((hdg - _last_hdg) > 180)
        _last_hdg += 360;
    if ((hdg - _last_hdg) < -180)
        _last_hdg -= 360;

    hdg = fgGetLowPass( _last_hdg , hdg, responsiveness );

    //but we need to filter the hdg and yaw_rate as well - yuk!
    responsiveness = 0.1 / (spin * spin * spin * spin * spin * spin);
    yaw_rate = fgGetLowPass( _last_yaw_rate , yaw_rate, responsiveness );
    g = fgGetLowPass( _last_g , g, 1.5);


    // store the new values
    _last_roll = roll;
    _last_pitch = pitch;
    _last_hdg = hdg;
    _last_roll_rate = roll_rate;
    _last_pitch_rate = pitch_rate;
    _last_yaw_rate = yaw_rate;
    _last_g = g;

    //the gyro only erects inside limits
    if ( fabs ( yaw_rate ) <= 5
        && g <= 1.5 && g >= -0.5){

            if ( !_fast_erect_node->getBoolValue() ){
                erect_time_factor = 1;
            } else {
                erect_time_factor = 2;
            }

        _g_error -= (max_g_error/(erect_time * 0.33)) * dt * erect_time_factor;
    } else {
        _g_error += (max_g_error /(erect_time * 0.33)) * dt * 2; 

        //SG_LOG(SG_INSTR, SG_ALERT,_num <<
        //    " g input " << _g_in_node->getDoubleValue() * gravity
        //    <<" _erect_time " << _erect_time 
        //    << " yaw " <<   yaw_rate
        //    << " pitch " << _pitch_rate_node->getDoubleValue()
        //    << " roll " << _roll_rate_node->getDoubleValue());

    }

    //cout << "_g_error "<< _g_error << endl;
    _g_error = SGMiscd::clip(_g_error, 0, 10);
//     cout << "_g_error clip "<< _g_error << endl;
    indicated_roll = _last_roll + _g_error;
    indicated_pitch = _last_pitch + _g_error;
    indicated_hdg = _last_hdg + _g_error;
    indicated_roll_rate = _last_roll_rate;
    indicated_pitch_rate = _last_pitch_rate;
    indicated_hdg_rate = _last_yaw_rate;
    // calculate the difference between the indicated heading
    // and the selected heading for use with an autopilot
    static SGPropertyNode *bnode
        = fgGetNode( "/autopilot/settings/heading-bug-deg", false );

    if ( bnode ) {
        double diff = bnode->getDoubleValue() - indicated_hdg;
        if ( diff < -180.0 ) { diff += 360.0; }
        if ( diff > 180.0 ) { diff -= 360.0; }
        _error_out_node->setDoubleValue( diff );
        //SG_LOG(SG_INSTR, SG_ALERT,
        //"autopilot input " << bnode->getDoubleValue() 
        //<< " output " << _error_out_node->getDoubleValue()<<);
    }

    //smooth the output
    double factor = _responsiveness_node->getDoubleValue() * dt;

    indicated_roll = fgGetLowPass( _indicated_roll, indicated_roll, factor );
    indicated_pitch = fgGetLowPass( _indicated_pitch , indicated_pitch, factor );
    //indicated_hdg = fgGetLowPass( _indicated_hdg , indicated_hdg, factor );

    indicated_roll_rate = fgGetLowPass( _indicated_roll_rate, indicated_roll_rate, factor );
    indicated_pitch_rate = fgGetLowPass( _indicated_pitch_rate , indicated_pitch_rate, factor );
    indicated_hdg_rate = fgGetLowPass( _indicated_hdg_rate , indicated_hdg_rate, factor );

    // store the new values
    _indicated_roll = indicated_roll;
    _indicated_pitch = indicated_pitch;
    _indicated_hdg = indicated_hdg;

    _indicated_roll_rate = indicated_roll_rate;
    _indicated_pitch_rate = indicated_pitch_rate;
    _indicated_hdg_rate = indicated_hdg_rate;

    // add in a gyro underspin "error" if gyro is spinning too slowly
    const double spin_thresh = 0.8;
    const double max_roll_error = 40.0;
    const double max_pitch_error = 12.0;
    const double max_hdg_error = 140.0;
    double roll_error;
    double pitch_error;
    double hdg_error;

    if ( spin <= spin_thresh ) {
        double roll_error_factor	= ( spin_thresh - spin ) / spin_thresh;
        double pitch_error_factor	= ( spin_thresh - spin ) / spin_thresh;
        double hdg_error_factor		= ( spin_thresh - spin ) / spin_thresh;
        roll_error	= roll_error_factor * roll_error_factor * max_roll_error;
        pitch_error = pitch_error_factor * pitch_error_factor * max_pitch_error;
        hdg_error	= hdg_error_factor * hdg_error_factor * max_hdg_error;
    } else {
        roll_error = 0.0;
        pitch_error = 0.0;
        hdg_error = 0.0;
    }

    _roll_out_node->setDoubleValue( _indicated_roll + roll_error );
    _pitch_out_node->setDoubleValue( _indicated_pitch + pitch_error );
    _hdg_out_node->setDoubleValue( _indicated_hdg + hdg_error );
    _pitch_rate_out_node ->setDoubleValue( _indicated_pitch_rate );
    _roll_rate_out_node ->setDoubleValue( _indicated_roll_rate );
    _hdg_rate_out_node ->setDoubleValue( _indicated_hdg_rate );
}

// end of mrg.cxx

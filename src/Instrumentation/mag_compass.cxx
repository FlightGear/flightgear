// mag_compass.cxx - a magnetic compass.
// Written by David Megginson, started 2003.
//
// This file is in the Public Domain and comes with no warranty.

// This implementation is derived from an earlier one by Alex Perry,
// which appeared in src/Cockpit/steam.cxx

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <plib/sg.h>

#include "mag_compass.hxx"
#include <Main/fg_props.hxx>
#include <Main/util.hxx>


MagCompass::MagCompass ( SGPropertyNode *node )
    : _error_deg(0.0),
      _rate_degps(0.0),
      name("magnetic-compass"),
      num(0)
{
    int i;
    for ( i = 0; i < node->nChildren(); ++i ) {
        SGPropertyNode *child = node->getChild(i);
        string cname = child->getName();
        string cval = child->getStringValue();
        if ( cname == "name" ) {
            name = cval;
        } else if ( cname == "number" ) {
            num = child->getIntValue();
        } else {
            SG_LOG( SG_INSTR, SG_WARN, "Error in magnetic-compass config logic" );
            if ( name.length() ) {
                SG_LOG( SG_INSTR, SG_WARN, "Section = " << name );
            }
        }
    }
}

MagCompass::MagCompass ()
    : _error_deg(0.0),
      _rate_degps(0.0)
{
}

MagCompass::~MagCompass ()
{
}

void
MagCompass::init ()
{
    string branch;
    branch = "/instrumentation/" + name;

    SGPropertyNode *node = fgGetNode(branch.c_str(), num, true );
    _serviceable_node = node->getChild("serviceable", 0, true);
    _roll_node =
        fgGetNode("/orientation/roll-deg", true);
    _pitch_node =
        fgGetNode("/orientation/pitch-deg", true);
    _heading_node =
        fgGetNode("/orientation/heading-magnetic-deg", true);
    _beta_node =
        fgGetNode("/orientation/side-slip-deg", true);
    _dip_node =
        fgGetNode("/environment/magnetic-dip-deg", true);
    _x_accel_node =
        fgGetNode("/accelerations/pilot/x-accel-fps_sec", true);
    _y_accel_node =
        fgGetNode("/accelerations/pilot/y-accel-fps_sec", true);
    _z_accel_node =
        fgGetNode("/accelerations/pilot/z-accel-fps_sec", true);
    _out_node = node->getChild("indicated-heading-deg", 0, true);

    _serviceable_node->setBoolValue(true);
}


void
MagCompass::update (double delta_time_sec)
{
                                // This is the real magnetic
                                // heading, which will almost
                                // never appear.
    double heading_mag_deg = _heading_node->getDoubleValue();


                                // don't update if it's broken
    if (!_serviceable_node->getBoolValue())
        return;

    /*
      Jam on an excessive sideslip.
    */
    if (fabs(_beta_node->getDoubleValue()) > 12.0) {
        _rate_degps = 0.0;
        return;
    }


    /*
      Formula for northernly turning error from
      http://williams.best.vwh.net/compass/node4.html:

      Hc: compass heading
      psi: magnetic heading
      theta: bank angle (right positive; should be phi here)
      mu: dip angle (down positive)

      Hc = atan2(sin(Hm)cos(theta)-tan(mu)sin(theta), cos(Hm))

      This function changes the variable names to the more common
      psi for the heading, theta for the pitch, and phi for the
      roll.  It also modifies the equation to incorporate pitch
      as well as roll, as suggested by Chris Metzler.
    */

                                // bank angle (radians)
    double phi = _roll_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;

                                // pitch angle (radians)
    double theta = _pitch_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;

                                // magnetic heading (radians)
    double psi = _heading_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;

                                // magnetic dip (radians)
    double mu = _dip_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;


    /*
      Tilt adjustments for accelerations.
      
      The magnitudes of these are totally made up, but in real life,
      they would depend on the fluid level, the amount of friction,
      etc. anyway.  Basically, the compass float tilts forward for
      acceleration and backward for deceleration.  Tilt about 4
      degrees (0.07 radians) for every G (32 fps/sec) of
      acceleration.

      TODO: do something with the vertical acceleration.
    */
    double x_accel_g = _x_accel_node->getDoubleValue() / 32;
    double y_accel_g = _y_accel_node->getDoubleValue() / 32;
    double z_accel_g = _z_accel_node->getDoubleValue() / 32;

    theta -= 0.07 * x_accel_g;
    phi -= 0.07 * y_accel_g;
    
    ////////////////////////////////////////////////////////////////////
    // calculate target compass heading Hc in degrees
    ////////////////////////////////////////////////////////////////////

                                // these are expensive: don't repeat
    double sin_phi = sin(phi);
    double sin_theta = sin(theta);
    double sin_mu = sin(mu);
    double cos_theta = cos(theta);
    double cos_psi = cos(psi);
    double cos_mu = cos(mu);

    double a = cos(phi) * sin(psi) * cos_mu
        - sin_phi * cos_theta * sin_mu
        - sin_phi* sin_theta * cos_mu * cos_psi;

    double b = cos_theta * cos_psi * cos(mu)
        - sin_theta * sin_mu;

                                // This is the value that the compass
                                // is *trying* to display, but it
                                // takes time to move there, and because
                                // of momentum, the compass will often
                                // overshoot.
    double Hc = atan2(a, b) * SGD_RADIANS_TO_DEGREES;

    _out_node->setDoubleValue(Hc);
}

// end of altimeter.cxx

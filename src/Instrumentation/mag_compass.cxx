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
    _north_accel_node =
        fgGetNode("/accelerations/ned/north-accel-fps_sec", true);
    _east_accel_node =
        fgGetNode("/accelerations/ned/east-accel-fps_sec", true);
    _down_accel_node =
        fgGetNode("/accelerations/ned/down-accel-fps_sec", true);
    _out_node = node->getChild("indicated-heading-deg", 0, true);

    _serviceable_node->setBoolValue(true);
}


void
MagCompass::update (double delta_time_sec)
{
                                // don't update if it's broken
    if (!_serviceable_node->getBoolValue())
        return;

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

    double Hc = atan2(a, b) * SGD_RADIANS_TO_DEGREES;

    while (Hc < 0)
        Hc += 360;
    while (Hc >= 360)
        Hc =- 360;

    // TODO add acceleration error
    // TODO allow binding with excessive dip/sideslip

    _out_node->setDoubleValue(Hc);


                                // algorithm from Alex Perry
                                // possibly broken by David Megginson

//                                 // jam on a sideslip of 12 degrees or more
//     if (fabs(_beta_node->getDoubleValue()) > 12.0) {
//         _rate_degps = 0.0;
//         _error_deg = _heading_node->getDoubleValue() -
//             _out_node->getDoubleValue();
//         return;
//     }

//     double accelN = _north_accel_node->getDoubleValue();
//     double accelE = _east_accel_node->getDoubleValue();
//     double accelU = _down_accel_node->getDoubleValue() - 32.0; // why?

//                                 // force vector towards magnetic north pole
//     double var = _variation_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
//     double dip = _dip_node->getDoubleValue() * SGD_DEGREES_TO_RADIANS;
//     double cosdip = cos(dip);
//     double forceN = cosdip * cos(var);
//     double forceE = cosdip * sin(var);
//     double forceU = sin(dip);

//                                 // rotation is around acceleration axis
//                                 // (magnitude doesn't matter)
//     double accel = accelN * accelN + accelE * accelE + accelU * accelU;
//     if (accel > 1.0)
//         accel = sqrt(accel);
//     else
//             accel = 1.0;

//                                 // North marking on compass card
//     double edgeN = cos(_error_deg * SGD_DEGREES_TO_RADIANS);
//     double edgeE = sin(_error_deg * SGD_DEGREES_TO_RADIANS);
//     double edgeU = 0.0;

//                                 // apply the force to that edge to get torques
//     double torqueN = edgeE * forceU - edgeU * forceE;
//     double torqueE = edgeU * forceN - edgeN * forceU;
//     double torqueU = edgeN * forceE - edgeE * forceN;

//                                 // get the component parallel to the axis
//     double torque = (torqueN * accelN +
//                      torqueE * accelE +
//                      torqueU * accelU) * 5.0 / accel;

//                                 // the compass has angular momentum,
//                                 // so apply a torque and wait
//     if (delta_time_sec < 1.0) {
//         _rate_degps = _rate_degps * (1.0 - delta_time_sec) - torque;
//         _error_deg += delta_time_sec * _rate_degps;
//     }
//     if (_error_deg > 180.0)
//         _error_deg -= 360.0;
//     else if (_error_deg < -180.0)
//         _error_deg += 360.0;

//                                 // Set the indicated heading
//     _out_node->setDoubleValue(_heading_node->getDoubleValue() - _error_deg);
}

// end of altimeter.cxx

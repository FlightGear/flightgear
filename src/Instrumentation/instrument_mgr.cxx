// instrument_mgr.cxx - manage aircraft instruments.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#include "instrument_mgr.hxx"
#include "airspeed_indicator.hxx"
#include "annunciator.hxx"
#include "attitude_indicator.hxx"
#include "altimeter.hxx"
#include "turn_indicator.hxx"
#include "slip_skid_ball.hxx"
#include "heading_indicator.hxx"
#include "vertical_speed_indicator.hxx"
#include "mag_compass.hxx"

#include "dme.hxx"
#include "gps.hxx"


FGInstrumentMgr::FGInstrumentMgr ()
{
    set_subsystem("asi", new AirspeedIndicator);
    set_subsystem("annunciator", new Annunciator);
    set_subsystem("ai", new AttitudeIndicator);
    set_subsystem("alt", new Altimeter);
    set_subsystem("ti", new TurnIndicator);
    set_subsystem("ball", new SlipSkidBall);
    set_subsystem("hi", new HeadingIndicator);
    set_subsystem("vsi", new VerticalSpeedIndicator);
    set_subsystem("compass", new MagCompass);
    set_subsystem("dme", new DME, 1.0);
    set_subsystem("gps", new GPS, 0.45);
}

FGInstrumentMgr::~FGInstrumentMgr ()
{
}

// end of instrument_manager.cxx

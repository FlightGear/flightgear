// instrument_mgr.cxx - manage aircraft instruments.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain and comes with no warranty.


#include "instrument_mgr.hxx"
#include "altimeter.hxx"
#include "attitude_indicator.hxx"
#include "heading_indicator.hxx"
#include "vertical_speed_indicator.hxx"


FGInstrumentMgr::FGInstrumentMgr ()
{
    // NO-OP
}

FGInstrumentMgr::~FGInstrumentMgr ()
{
    for (unsigned int i = 0; i < _instruments.size(); i++) {
        delete _instruments[i];
        _instruments[i] = 0;
    }
}

void
FGInstrumentMgr::init ()
{
                                // TODO: replace with XML configuration
    _instruments.push_back(new Altimeter);
    _instruments.push_back(new AttitudeIndicator);
    _instruments.push_back(new HeadingIndicator);
    _instruments.push_back(new VerticalSpeedIndicator);

                                // Initialize the individual instruments
    for (unsigned int i = 0; i < _instruments.size(); i++)
        _instruments[i]->init();
}

void
FGInstrumentMgr::bind ()
{
    // NO-OP
}

void
FGInstrumentMgr::unbind ()
{
    // NO-OP
}

void
FGInstrumentMgr::update (double dt)
{
    for (unsigned int i = 0; i < _instruments.size(); i++)
        _instruments[i]->update(dt);
}

// end of instrument_manager.cxx

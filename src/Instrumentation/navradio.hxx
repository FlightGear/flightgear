// navradio.hxx -- class to manage a nav radio instance
//
// Written by Curtis Olson, started April 2000.
//
// Copyright (C) 2000 - 2002  Curtis L. Olson - http://www.flightgear.org/~curt
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
// $Id$


#ifndef _FG_NAVRADIO_HXX
#define _FG_NAVRADIO_HXX


#include <Main/fg_props.hxx>

#include <simgear/compiler.h>
#include <simgear/structure/subsystem_mgr.hxx>
#include <simgear/timing/timestamp.hxx>

// forward decls
class SGInterpTable;

class SGSampleGroup;
class FGNavRecord;
typedef SGSharedPtr<FGNavRecord> FGNavRecordPtr;

class FGNavRadio : public SGSubsystem, public SGPropertyChangeListener
{
    SGInterpTable *term_tbl;
    SGInterpTable *low_tbl;
    SGInterpTable *high_tbl;

    SGPropertyNode_ptr _radio_node;
    SGPropertyNode_ptr bus_power_node;

    // property inputs
    SGPropertyNode_ptr is_valid_node;   // is station data valid (may be way out
                                        // of range.)
    SGPropertyNode_ptr power_btn_node;
    SGPropertyNode_ptr freq_node;       // primary freq
    SGPropertyNode_ptr alt_freq_node;   // standby freq
    SGPropertyNode_ptr is_loc_freq_node;// is the primary freq a loc/gs (paired) freq?
    SGPropertyNode_ptr sel_radial_node; // selected radial
    SGPropertyNode_ptr vol_btn_node;
    SGPropertyNode_ptr ident_btn_node;
    SGPropertyNode_ptr audio_btn_node;
    SGPropertyNode_ptr backcourse_node;
    SGPropertyNode_ptr nav_serviceable_node;
    SGPropertyNode_ptr cdi_serviceable_node;
    SGPropertyNode_ptr gs_serviceable_node;
    SGPropertyNode_ptr tofrom_serviceable_node;
    
    // property outputs
    SGPropertyNode_ptr fmt_freq_node;     // formated frequency
    SGPropertyNode_ptr fmt_alt_freq_node; // formated alternate frequency
    SGPropertyNode_ptr heading_node;      // true heading to nav station
    SGPropertyNode_ptr radial_node;       // current radial we are on (taking
                                       // into consideration the vor station
                                       // alignment which likely doesn't
                                       // match the magnetic alignment
                                       // exactly.)
    SGPropertyNode_ptr recip_radial_node; // radial_node(val) + 180 (for
                                       // convenience)
    SGPropertyNode_ptr target_radial_true_node;
                                       // true heading of selected radial
    SGPropertyNode_ptr target_auto_hdg_node;
                                       // suggested autopilot heading
                                       // to intercept selected radial
    SGPropertyNode_ptr time_to_intercept; // estimated time to intecept selected
                                       // radial at current speed and heading
    SGPropertyNode_ptr to_flag_node;
    SGPropertyNode_ptr from_flag_node;
    SGPropertyNode_ptr inrange_node;
    SGPropertyNode_ptr signal_quality_norm_node;
    SGPropertyNode_ptr cdi_deflection_node;
    SGPropertyNode_ptr cdi_deflection_norm_node;
    SGPropertyNode_ptr cdi_xtrack_error_node;
    SGPropertyNode_ptr cdi_xtrack_hdg_err_node;
    SGPropertyNode_ptr has_gs_node;
    SGPropertyNode_ptr loc_node;
    SGPropertyNode_ptr loc_dist_node;
    SGPropertyNode_ptr gs_deflection_node;
    SGPropertyNode_ptr gs_deflection_deg_node;
    SGPropertyNode_ptr gs_deflection_norm_node;
    SGPropertyNode_ptr gs_direct_node;
    SGPropertyNode_ptr gs_rate_of_climb_node;
    SGPropertyNode_ptr gs_rate_of_climb_fpm_node;
    SGPropertyNode_ptr gs_dist_node;
    SGPropertyNode_ptr gs_inrange_node;
    SGPropertyNode_ptr nav_id_node;
    SGPropertyNode_ptr id_c1_node;
    SGPropertyNode_ptr id_c2_node;
    SGPropertyNode_ptr id_c3_node;
    SGPropertyNode_ptr id_c4_node;

    // gps slaving support
    SGPropertyNode_ptr nav_slaved_to_gps_node;
    SGPropertyNode_ptr gps_cdi_deflection_node;
    SGPropertyNode_ptr gps_to_flag_node;
    SGPropertyNode_ptr gps_from_flag_node;
    SGPropertyNode_ptr gps_has_gs_node;
    SGPropertyNode_ptr gps_course_node;
    SGPropertyNode_ptr gps_xtrack_error_nm_node;
    SGPropertyNode_ptr _magvarNode;
    
    // realism setting, are false courses and GS lobes enabled?
    SGPropertyNode_ptr falseCoursesEnabledNode;

    // internal (private) values

    bool _operable; ///< is the unit serviceable, on, powered, etc
    int play_count;
    bool _nav_search;
    double _last_freq;
    FGNavRecordPtr _navaid;
    FGNavRecordPtr _gs;
    
    double target_radial;
    double effective_range;
    double target_gs;
    double twist;
    double horiz_vel;
    double last_x;
    double last_xtrack_error;
    double xrate_ms;
    double _localizerWidth; // cached localizer width in degrees
    
    std::string _name;
    int _num;

    // internal periodic station search timer
    double _time_before_search_sec;

    SGVec3d _gsCart, _gsAxis, _gsVertical, _gsBaseline;

    // CDI properties
    bool _toFlag, _fromFlag;
    double _cdiDeflection;
    double _cdiCrossTrackErrorM;
    double _gsNeedleDeflection;
    double _gsNeedleDeflectionNorm;
    double _gsDirect;

    class AudioIdent * _audioIdent;
    
    bool updateWithPower(double aDt);

    // model standard VOR/DME/TACAN service volumes as per AIM 1-1-8
    double adjustNavRange( double stationElev, double aircraftElev,
			   double nominalRange );

    // model standard ILS service volumes as per AIM 1-1-9
    double adjustILSRange( double stationElev, double aircraftElev,
			   double offsetDegrees, double distance );

    void updateAudio( double dt );

    void updateReceiver(double dt);
    void updateGlideSlope(double dt, const SGVec3d& aircraft, double signal_quality_norm);
    void updateGPSSlaved();
    void updateCDI(double dt);
    
    void clearOutputs();

    FGNavRecord* findPrimaryNavaid(const SGGeod& aPos, double aFreqMHz);
    
    /// accessor for tied, read-only 'operable' property
    bool isOperable() const
      { return _operable; }
      
  // implement SGPropertyChangeListener
    virtual void valueChanged (SGPropertyNode * prop);
public:

    FGNavRadio(SGPropertyNode *node);
    ~FGNavRadio();

    void init ();
    void bind ();
    void unbind ();
    void update (double dt);

    // Update nav/adf radios based on current postition
    void search ();
    void updateNav();
};


#endif // _FG_NAVRADIO_HXX

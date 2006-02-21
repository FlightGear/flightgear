/**********************************************************************

 FILENAME:     uiuc_aerodeflections.cpp

----------------------------------------------------------------------

 DESCRIPTION:  determine the aero control surface deflections
               elevator [rad]
               aileron [rad]
               rudder [rad]
               
----------------------------------------------------------------------

 STATUS:       alpha version

----------------------------------------------------------------------

 REFERENCES:   based on deflection portions of c172_aero.c and 
               uiuc_aero.c

----------------------------------------------------------------------

 HISTORY:      01/30/2000   initial release
               04/05/2000   (JS) added zero_Long_trim command
	       07/05/2001   (RD) removed elevator_tab addition to
	                    elevator calculation
	       11/12/2001   (RD) added new flap routine.  Needed for
	                    Twin Otter non-linear model
               12/28/2002   (MSS) added simple SAS capability
               03/03/2003   (RD) changed flap code to call
                            uiuc_find_position to determine
                            flap position
               08/20/2003   (RD) changed spoiler variables and code
                            to match flap conventions.  Changed
                            flap_pos_pct to flap_pos_norm

----------------------------------------------------------------------

 AUTHOR(S):    Jeff Scott         http://www.jeffscott.net/
               Robert Deters      <rdeters@uiuc.edu>
               Michael Selig      <m-selig@uiuc.edu>

----------------------------------------------------------------------

 VARIABLES:

----------------------------------------------------------------------

 INPUTS:       *

----------------------------------------------------------------------

 OUTPUTS:      *

----------------------------------------------------------------------

 CALLED BY:    *

----------------------------------------------------------------------

 CALLS TO:     *

----------------------------------------------------------------------

 COPYRIGHT:    (C) 2000 by Michael Selig

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**********************************************************************/

#include <math.h>

#include "uiuc_aerodeflections.h"

void uiuc_aerodeflections( double dt )
{
  double demax_remain;
  double demin_remain;
  static double elev_trim;

  if (use_uiuc_network) {
      if (pitch_trim_up)
	elev_trim += 0.001;
      if (pitch_trim_down)
	elev_trim -= 0.001;
      if (elev_trim > 1.0)
	elev_trim = 1;
      if (elev_trim < -1.0)
	elev_trim = -1;
      Flap_handle = flap_percent * flap_max;
      if (outside_control) {
	pilot_elev_no = true;
	pilot_ail_no = true;
	pilot_rud_no = true;
	pilot_throttle_no = true;
	Long_trim = elev_trim;
      }
  }
  
  if (zero_Long_trim) {
    Long_trim = 0;
    //elevator_tab = 0;
  }
  
  if (Lat_control <= 0)
    aileron = - Lat_control * damin * DEG_TO_RAD;
  else
    aileron = - Lat_control * damax * DEG_TO_RAD;
  
  if (trim_case_2) {
    if (Long_trim <= 0) {
      elevator = Long_trim * demax * DEG_TO_RAD;
      demax_remain = demax + Long_trim * demax;
      demin_remain = -Long_trim * demax + demin;
      if (Long_control <= 0)
	elevator += Long_control * demax_remain * DEG_TO_RAD;
      else
	elevator += Long_control * demin_remain * DEG_TO_RAD;
    } else {
      elevator = Long_trim * demin * DEG_TO_RAD;
      demin_remain = demin - Long_trim * demin;
      demax_remain = Long_trim * demin + demax;
      if (Long_control >=0)
	elevator += Long_control * demin_remain * DEG_TO_RAD;
      else
	elevator += Long_control * demax_remain * DEG_TO_RAD;
    }
  } else {
    if ((Long_control+Long_trim) <= 0)
      elevator = (Long_control + Long_trim) * demax * DEG_TO_RAD;
    else
      elevator = (Long_control + Long_trim) * demin * DEG_TO_RAD;
  }
  
  if (Rudder_pedal <= 0)
    rudder = -Rudder_pedal * drmin * DEG_TO_RAD;
  else
    rudder = -Rudder_pedal * drmax * DEG_TO_RAD;

  /* at this point aileron, elevator, and rudder commands are known (in radians)
   * add in SAS and any other mixing control laws
   */
  
  // SAS for pitch, positive elevator is TED
  if (use_elevator_stick_gain) 
    elevator *= elevator_stick_gain;
  if (use_elevator_sas_type1) {
    elevator_sas = elevator_sas_KQ * Q_body;
    if (use_elevator_sas_max && (elevator_sas > (elevator_sas_max * DEG_TO_RAD))) {
      elevator += elevator_sas_max * DEG_TO_RAD;
      //elevator_sas = elevator_sas_max;
    } else if (use_elevator_sas_min && (elevator_sas < (elevator_sas_min * DEG_TO_RAD))) {
      elevator += elevator_sas_min * DEG_TO_RAD;
      //elevator_sas = elevator_sas_min;
    }
    else
      elevator += elevator_sas;
    // don't exceed the elevator deflection limits
    if (elevator > demax * DEG_TO_RAD)
      elevator = demax * DEG_TO_RAD;
    else if (elevator < (-demin * DEG_TO_RAD))
      elevator = -demin * DEG_TO_RAD;
  }  
  
  // SAS for roll, positive aileron is right aileron TED
  if (use_aileron_stick_gain) 
    aileron *= aileron_stick_gain;
  if (use_aileron_sas_type1) {
    aileron_sas = aileron_sas_KP * P_body;
    if (use_aileron_sas_max && (fabs(aileron_sas) > (aileron_sas_max * DEG_TO_RAD)))
      if (aileron_sas >= 0) {
	aileron +=  aileron_sas_max * DEG_TO_RAD;
	//aileron_sas = aileron_sas_max;
      } else {
	aileron += -aileron_sas_max * DEG_TO_RAD;
	//aileron_sas = -aileron_sas;
      }
    else
      aileron += aileron_sas;
    // don't exceed aileron deflection limits
    if (fabs(aileron) > (damax * DEG_TO_RAD))
      if (aileron > 0)
	aileron =  damax * DEG_TO_RAD;
      else
	aileron = -damax * DEG_TO_RAD;
  }
  
  // SAS for yaw, positive rudder deflection is TEL
  if (use_rudder_stick_gain)
    rudder *= rudder_stick_gain;
  if (use_rudder_sas_type1) {
    rudder_sas = rudder_sas_KR * R_body;
    if (use_rudder_sas_max && (fabs(rudder_sas) > (rudder_sas_max*DEG_TO_RAD)))
      if (rudder_sas >= 0) {
	rudder +=  rudder_sas_max * DEG_TO_RAD;
	//rudder_sas = rudder_sas_max;
      } else {
	rudder += -rudder_sas_max * DEG_TO_RAD;
	//rudder_sas = rudder_sas_max;
      }
    else
      rudder += rudder_sas;
    // don't exceed rudder deflection limits, assumes drmax = drmin, 
    // i.e. equal rudder throws left and right
    if (fabs(rudder) > drmax)
      if (rudder > 0)
	rudder =  drmax * DEG_TO_RAD;
      else
	rudder = -drmax * DEG_TO_RAD;
  }
  
  /* This old code in the first part of the if-block needs to get removed from FGFS. 030222 MSS
     The second part of the if-block is rewritten below.
  double prevFlapHandle = 0.0f;
  double flap_transit_rate;
  bool flaps_in_transit = false;

  if (old_flap_routine) {
    // old flap routine
    // check for lowest flap setting
    if (Flap_handle < dfArray[1]) {
      Flap_handle    = dfArray[1];
      prevFlapHandle = Flap_handle;
      flap           = Flap_handle;
    } else if (Flap_handle > dfArray[ndf]) {
      // check for highest flap setting
      Flap_handle      = dfArray[ndf];
      prevFlapHandle   = Flap_handle;
      flap             = Flap_handle;
    } else {
      // otherwise in between
      if(Flap_handle != prevFlapHandle) 
	flaps_in_transit = true;
      if(flaps_in_transit) {
	int iflap = 0;
	while (dfArray[iflap] < Flap_handle)
	  iflap++;
	if (flap < Flap_handle) {
	  if (TimeArray[iflap] > 0)
	    flap_transit_rate = (dfArray[iflap] - dfArray[iflap-1]) / TimeArray[iflap+1];
	  else
	    flap_transit_rate = (dfArray[iflap] - dfArray[iflap-1]) / 5;
	} else {
	  if (TimeArray[iflap+1] > 0)
	    flap_transit_rate = (dfArray[iflap] - dfArray[iflap+1]) / TimeArray[iflap+1];
	  else
	    flap_transit_rate = (dfArray[iflap] - dfArray[iflap+1]) / 5;
	}
	if(fabs (flap - Flap_handle) > dt * flap_transit_rate)
	  flap += flap_transit_rate * dt;
	else {
	  flaps_in_transit = false;
	  flap = Flap_handle;
	}
      }
    }
    prevFlapHandle = Flap_handle;
  } else {
    // new flap routine
    // designed for the twin otter non-linear model
    if (!outside_control) {
      flap_percent     = Flap_handle / 30.0;    // percent of flaps desired
      if (flap_percent>=0.31 && flap_percent<=0.35)
	flap_percent = 1.0 / 3.0;
      if (flap_percent>=0.65 && flap_percent<=0.69)
	flap_percent = 2.0 / 3.0;
    }
    flap_cmd_deg        = flap_percent * flap_max;  // angle of flaps desired
    flap_moving_rate = flap_rate * dt;           // amount flaps move per time step
    
    // determine flap position with respect to the flap goal
    if (flap_pos < flap_cmd_deg) {
      flap_pos += flap_moving_rate;
      if (flap_pos > flap_cmd_deg)
	flap_pos = flap_cmd_deg;
    } else if (flap_pos > flap_cmd_deg) {
      flap_pos -= flap_moving_rate;
      if (flap_pos < flap_cmd_deg)
	flap_pos = flap_cmd_deg;
    } 
  }

*/

  if (!outside_control && use_flaps) {
    // angle of flaps desired [rad]
    flap_cmd = Flap_handle * DEG_TO_RAD;
    // amount flaps move per time step [rad/sec]
    flap_increment_per_timestep = flap_rate * dt * DEG_TO_RAD;
    // determine flap position [rad] with respect to flap command
    flap_pos = uiuc_find_position(flap_cmd,flap_increment_per_timestep,flap_pos);
    // get the normalized position
    flap_pos_norm = flap_pos/(flap_max * DEG_TO_RAD);
  }
  
    
  if(use_spoilers) {
    // angle of spoilers desired [rad]
    spoiler_cmd = Spoiler_handle * DEG_TO_RAD;
    // amount spoilers move per time step [rad/sec]
    spoiler_increment_per_timestep = spoiler_rate * dt * DEG_TO_RAD; 
    // determine spoiler position [rad] with respect to spoiler command
    spoiler_pos = uiuc_find_position(spoiler_cmd,spoiler_increment_per_timestep,spoiler_pos);
    // get the normailized position
    spoiler_pos_norm = spoiler_pos/(spoiler_max * DEG_TO_RAD);
  }


  return;
}

// end uiuc_aerodeflections.cpp

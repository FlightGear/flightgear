#include "uiuc_get_flapper.h"

void uiuc_get_flapper(double dt)
{
  double flapper_alpha;
  double flapper_V;
  //double cycle_incr;
  flapStruct flapper_struct;
  //FlapStruct flapper_struct;

  flapper_alpha = Std_Alpha*180/LS_PI;
  flapper_V = V_rel_wind;

  flapper_freq = 0.8 + 0.45*Throttle_pct;

  //if (Simtime == 0)
  //  flapper_cycle_pct = flapper_cycle_pct_init;
  //else
  //  {
  //    cycle_incr = flapper_freq*dt - static_cast<int>(flapper_freq*dt);
  //    if (cycle_incr < 1)
  //      flapper_cycle_pct += cycle_incr;
  //    else  //need because problem when flapper_freq*dt is same as int
  //	flapper_cycle_pct += cycle_incr - 1;
  //  }
  //if (flapper_cycle_pct >= 1)
  //  flapper_cycle_pct -= 1;

  //if (flapper_cycle_pct >= 0 && flapper_cycle_pct < 0.25)
  //  flapper_phi = LS_PI/2 * (sin(2*LS_PI*flapper_cycle_pct+3*LS_PI/2)+1);
  //if (flapper_cycle_pct >= 0.25 && flapper_cycle_pct < 0.5)
  //  flapper_phi = LS_PI/2 * sin(2*LS_PI*(flapper_cycle_pct-0.25))+LS_PI/2;
  //if (flapper_cycle_pct >= 0.5 && flapper_cycle_pct < 0.75)
  //  flapper_phi = LS_PI/2 * (sin(2*LS_PI*(flapper_cycle_pct-0.5)+3*LS_PI/2)+1)+LS_PI;
  //if (flapper_cycle_pct >= 0.75 && flapper_cycle_pct < 1)
  //  flapper_phi = LS_PI/2 * sin(2*LS_PI*(flapper_cycle_pct-0.75))+3*LS_PI/2;

  if (Simtime == 0)
    flapper_phi = flapper_phi_init;
  else
    flapper_phi += 2* LS_PI * flapper_freq * dt;

  if (flapper_phi >= 2*LS_PI)
    flapper_phi -= 2*LS_PI;

  flapper_struct=flapper_data->flapper(flapper_alpha,flapper_V,flapper_freq,flapper_phi);
  flapper_Lift=flapper_struct.getLift();
  flapper_Thrust=flapper_struct.getThrust();
  flapper_Inertia=flapper_struct.getInertia();
  flapper_Moment=flapper_struct.getMoment();

  F_Z_aero_flapper = -1*flapper_Lift*cos(Std_Alpha) - flapper_Thrust*sin(Std_Alpha);
  F_Z_aero_flapper -= flapper_Inertia;
  F_X_aero_flapper = -1*flapper_Lift*sin(Std_Alpha) + flapper_Thrust*cos(Std_Alpha);

}

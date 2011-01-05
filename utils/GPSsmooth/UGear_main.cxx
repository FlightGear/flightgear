#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#else
#  include <netinet/in.h>       // htonl() ntohl()
#endif

#ifndef _WIN32
#  include <strings.h>		// for bzero()
#else
#  define bzero(a,b) memset(a,0,b)
#endif
#include <iostream>
#include <string>

#include <plib/sg.h>

#include <simgear/constants.h>
#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/io/sg_file.hxx>
#include <simgear/io/raw_socket.hxx>
#include <simgear/serial/serial.hxx>
#include <simgear/math/sg_geodesy.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>

#include "UGear.hxx"
#include "UGear_command.hxx"
#include "UGear_opengc.hxx"
#include "UGear_telnet.hxx"


using std::cout;
using std::endl;
using std::string;


// Network channels
static simgear::Socket fdm_sock, ctrls_sock, opengc_sock;

// ugear data
UGTrack track;

// Default ports
static int fdm_port = 5505;
static int ctrls_port = 5506;
static int opengc_port = 6000;

// Default path
static string infile = "";
static string flight_dir = "";
static string serialdev = "";
static string outfile = "";

// Master time counter
float sim_time = 0.0f;
double frame_us = 0.0f;

// sim control
SGTimeStamp last_time_stamp;
SGTimeStamp current_time_stamp;

// altitude offset
double alt_offset = 0.0;

// skip initial seconds
double skip = 0.0;

// for speed estimate
double last_lat = 0.0, last_lon = 0.0;
double kts_filter = 0.0;

bool inited = false;

bool run_real_time = true;

bool ignore_checksum = false;

bool sg_swap = false;

bool use_ground_track_hdg = false;
bool use_ground_speed = false;

bool est_controls = false;

float gps_status = -1.0;


// The function htond is defined this way due to the way some
// processors and OSes treat floating point values.  Some will raise
// an exception whenever a "bad" floating point value is loaded into a
// floating point register.  Solaris is notorious for this, but then
// so is LynxOS on the PowerPC.  By translating the data in place,
// there is no need to load a FP register with the "corruped" floating
// point value.  By doing the BIG_ENDIAN test, I can optimize the
// routine for big-endian processors so it can be as efficient as
// possible
static void htond (double &x)	
{
    if ( sgIsLittleEndian() ) {
        int    *Double_Overlay;
        int     Holding_Buffer;
    
        Double_Overlay = (int *) &x;
        Holding_Buffer = Double_Overlay [0];
    
        Double_Overlay [0] = htonl (Double_Overlay [1]);
        Double_Overlay [1] = htonl (Holding_Buffer);
    } else {
        return;
    }
}

// Float version
static void htonf (float &x)	
{
    if ( sgIsLittleEndian() ) {
        int    *Float_Overlay;
        int     Holding_Buffer;
    
        Float_Overlay = (int *) &x;
        Holding_Buffer = Float_Overlay [0];
    
        Float_Overlay [0] = htonl (Holding_Buffer);
    } else {
        return;
    }
}


static void ugear2fg( gps *gpspacket, imu *imupacket, nav *navpacket,
		      servo *servopacket, health *healthpacket,
		      FGNetFDM *fdm, FGNetCtrls *ctrls )
{
    unsigned int i;

    // Version sanity checking
    fdm->version = FG_NET_FDM_VERSION;

    // Aero parameters
    fdm->longitude = navpacket->lon * SG_DEGREES_TO_RADIANS;
    fdm->latitude = navpacket->lat * SG_DEGREES_TO_RADIANS;
    fdm->altitude = navpacket->alt + alt_offset;
    fdm->agl = -9999.0;
    fdm->psi = imupacket->psi; // heading
    fdm->phi = imupacket->phi; // roll
    fdm->theta = imupacket->the; // pitch;

    fdm->phidot = 0.0;
    fdm->thetadot = 0.0;
    fdm->psidot = 0.0;

    // estimate speed
    // double az1, az2, dist;
    // geo_inverse_wgs_84( fdm->altitude, last_lat, last_lon,
    //                     fdm->latitude, fdm->longitude, &az1, &az2, &dist );
    // last_lat = fdm->latitude;
    // last_lon = fdm->longitude;
    // double v_ms = dist / (frame_us / 1000000);
    // double v_kts = v_ms * SG_METER_TO_NM * 3600;
    // kts_filter = (0.9 * kts_filter) + (0.1 * v_kts);
    // printf("dist = %.5f  kts est = %.2f\n", dist, kts_filter);

    double vn = navpacket->vn;
    double ve = navpacket->ve;
    double vd = navpacket->vd;

    if ( use_ground_track_hdg ) {
        fdm->psi = SGD_PI * 0.5 - atan2(vn, ve); // heading
    }

    double mps = 0.0;
    if ( use_ground_speed ) {
        mps = sqrt( vn*vn + ve*ve + vd*vd );
    } else {
        mps = imupacket->Pt;
    }
    // double mph = mps * 3600 / 1609.3440;
    double kts = mps * SG_MPS_TO_KT;
    fdm->vcas = kts;
    // printf("speed = %.2f mph  %.2f kts\n", mph, kts );
    static double Ps = 0.0, Ps_last = 0.0, t_last = 0.0;
    Ps_last = Ps;
    Ps = 0.92 * Ps + 0.08 * imupacket->Ps;
    double climb = (Ps - Ps_last) / (imupacket->time - t_last);
    t_last = imupacket->time;
    static double climbf = 0.0;
    climbf = 0.994 * climbf + 0.006 * climb;
    fdm->climb_rate = climbf; // fps

    static double Ps_error = 0.0;
    static double Ps_count = 0;
    const double span = 10000.0;
    Ps_count += 1.0; if (Ps_count > (span-1.0)) { Ps_count = (span-1.0); }
    double error = navpacket->alt - Ps;
    Ps_error = (Ps_count/span) * Ps_error + ((span-Ps_count)/span) * error;
    fdm->altitude = Ps +  Ps_error;

    /* printf("%.3f, %.3f, %.3f, %.3f, %.8f, %.8f, %.3f, %.3f, %.3f, %.3f, %.3f\n",
           imupacket->time, imupacket->the, -navpacket->vd, climbf,
           navpacket->lat, navpacket->lon, gpspacket->alt, navpacket->alt,
           imupacket->Ps, Ps, Ps + Ps_error); */

    // cout << "climb rate = " << aero->hdota << endl;
    fdm->v_north = 0.0;
    fdm->v_east = 0.0;
    fdm->v_down = 0.0;
    fdm->v_wind_body_north = 0.0;
    fdm->v_wind_body_east = 0.0;
    fdm->v_wind_body_down = 0.0;
    fdm->stall_warning = 0.0;

    fdm->A_X_pilot = 0.0;
    fdm->A_Y_pilot = 0.0;
    fdm->A_Z_pilot = 0.0 /* (should be -G) */;

    // Engine parameters
    fdm->num_engines = 1;
    fdm->eng_state[0] = 2;
    // cout << "state = " << fdm->eng_state[0] << endl;
    double rpm = 5000.0 - ((double)servopacket->chn[2] / 65536.0)*3500.0;
    if ( rpm < 0.0 ) { rpm = 0.0; }
    if ( rpm > 5000.0 ) { rpm = 5000.0; }
    fdm->rpm[0] = rpm;

    fdm->fuel_flow[0] = 0.0;
    fdm->egt[0] = 0.0;
    // cout << "egt = " << aero->EGT << endl;
    fdm->oil_temp[0] = 0.0;
    fdm->oil_px[0] = 0.0;

    // Consumables
    fdm->num_tanks = 2;
    fdm->fuel_quantity[0] = 0.0;
    fdm->fuel_quantity[1] = 0.0;

    // Gear and flaps
    fdm->num_wheels = 3;
    fdm->wow[0] = 0;
    fdm->wow[1] = 0;
    fdm->wow[2] = 0;

    // the following really aren't used in this context
    fdm->cur_time = 0;
    fdm->warp = 0;
    fdm->visibility = 0;

    // cout << "Flap deflection = " << aero->dflap << endl;
    fdm->left_flap = 0.0;
    fdm->right_flap = 0.0;

    if ( est_controls ) {
        static float est_elev = 0.0;
        static float est_aileron = 0.0;
        static float est_rudder = 0.0;
        est_elev = 0.99 * est_elev + 0.01 * (imupacket->q * 4);
        est_aileron = 0.95 * est_aileron + 0.05 * (imupacket->p * 5);
        est_rudder = 0.95 * est_rudder + 0.05 * (imupacket->r * 2);
        ctrls->elevator = fdm->elevator = -est_elev;
        ctrls->aileron = fdm->left_aileron = est_aileron;
        fdm->right_aileron = -est_aileron;
        ctrls->rudder = fdm->rudder = est_rudder;
    } else {
        ctrls->elevator = fdm->elevator = 1.0 - ((double)servopacket->chn[1] / 32768.0);
        ctrls->aileron = fdm->left_aileron = 1.0 - ((double)servopacket->chn[0] / 32768.0);
        fdm->right_aileron = ((double)servopacket->chn[0] / 32768.0) - 1.0;
        ctrls->rudder = fdm->rudder = 1.0 - ((double)servopacket->chn[3] / 32768.0);
	ctrls->elevator *= 3.0;
	ctrls->aileron *= 3.0;
    }
    fdm->elevator_trim_tab = 0.0;
    fdm->left_flap = 0.0;
    fdm->right_flap = 0.0;
    fdm->nose_wheel = 0.0;
    fdm->speedbrake = 0.0;
    fdm->spoilers = 0.0;

    // Convert the net buffer to network format
    fdm->version = htonl(fdm->version);

    htond(fdm->longitude);
    htond(fdm->latitude);
    htond(fdm->altitude);
    htonf(fdm->agl);
    htonf(fdm->phi);
    htonf(fdm->theta);
    htonf(fdm->psi);
    htonf(fdm->alpha);
    htonf(fdm->beta);

    htonf(fdm->phidot);
    htonf(fdm->thetadot);
    htonf(fdm->psidot);
    htonf(fdm->vcas);
    htonf(fdm->climb_rate);
    htonf(fdm->v_north);
    htonf(fdm->v_east);
    htonf(fdm->v_down);
    htonf(fdm->v_wind_body_north);
    htonf(fdm->v_wind_body_east);
    htonf(fdm->v_wind_body_down);

    htonf(fdm->A_X_pilot);
    htonf(fdm->A_Y_pilot);
    htonf(fdm->A_Z_pilot);

    htonf(fdm->stall_warning);
    htonf(fdm->slip_deg);

    for ( i = 0; i < fdm->num_engines; ++i ) {
        fdm->eng_state[i] = htonl(fdm->eng_state[i]);
        htonf(fdm->rpm[i]);
        htonf(fdm->fuel_flow[i]);
        htonf(fdm->egt[i]);
        htonf(fdm->cht[i]);
        htonf(fdm->mp_osi[i]);
        htonf(fdm->tit[i]);
        htonf(fdm->oil_temp[i]);
        htonf(fdm->oil_px[i]);
    }
    fdm->num_engines = htonl(fdm->num_engines);

    for ( i = 0; i < fdm->num_tanks; ++i ) {
        htonf(fdm->fuel_quantity[i]);
    }
    fdm->num_tanks = htonl(fdm->num_tanks);

    for ( i = 0; i < fdm->num_wheels; ++i ) {
        fdm->wow[i] = htonl(fdm->wow[i]);
        htonf(fdm->gear_pos[i]);
        htonf(fdm->gear_steer[i]);
        htonf(fdm->gear_compression[i]);
    }
    fdm->num_wheels = htonl(fdm->num_wheels);

    fdm->cur_time = htonl( fdm->cur_time );
    fdm->warp = htonl( fdm->warp );
    htonf(fdm->visibility);

    htonf(fdm->elevator);
    htonf(fdm->elevator_trim_tab);
    htonf(fdm->left_flap);
    htonf(fdm->right_flap);
    htonf(fdm->left_aileron);
    htonf(fdm->right_aileron);
    htonf(fdm->rudder);
    htonf(fdm->nose_wheel);
    htonf(fdm->speedbrake);
    htonf(fdm->spoilers);

#if 0    
    ctrls->version = FG_NET_CTRLS_VERSION;
    ctrls->elevator_trim = 0.0;
    ctrls->flaps = 0.0;

    htonl(ctrls->version);
    htond(ctrls->aileron);
    htond(ctrls->rudder);
    htond(ctrls->elevator);
    htond(ctrls->elevator_trim);
    htond(ctrls->flaps);
#endif
}


static void ugear2opengc( gps *gpspacket, imu *imupacket, nav *navpacket,
                          servo *servopacket, health *healthpacket,
                          ogcFGData *ogc )
{
    // Version sanity checking
    ogc->version_id = OGC_VERSION;

    // Aero parameters
    ogc->longitude = navpacket->lon;
    ogc->latitude = navpacket->lat;
    ogc->heading = imupacket->psi * SG_RADIANS_TO_DEGREES; // heading
    ogc->bank = imupacket->phi * SG_RADIANS_TO_DEGREES; // roll
    ogc->pitch = imupacket->the * SG_RADIANS_TO_DEGREES; // pitch;

    ogc->phi_dot = 0.0;
    ogc->theta_dot = 0.0;
    ogc->psi_dot = 0.0;

    ogc->alpha = 0.0;
    ogc->beta = 0.0;
    ogc->alpha_dot = 0.0;
    ogc->beta_dot = 0.0;

    ogc->left_aileron = 1.0 - ((double)servopacket->chn[0] / 32768.0);
    ogc->right_aileron = ((double)servopacket->chn[0] / 32768.0) - 1.0;
    ogc->elevator = 1.0 - ((double)servopacket->chn[1] / 32768.0);
    ogc->elevator_trim = 0.0;
    ogc->rudder = 1.0 - ((double)servopacket->chn[3] / 32768.0);
    ogc->flaps = 0.0;
    ogc->flaps_cmd = 0.0;

    ogc->wind = 0.0;
    ogc->wind_dir = 0.0;

    // estimate speed
    // double az1, az2, dist;
    // geo_inverse_wgs_84( fdm->altitude, last_lat, last_lon,
    //                     fdm->latitude, fdm->longitude, &az1, &az2, &dist );
    // last_lat = fdm->latitude;
    // last_lon = fdm->longitude;
    // double v_ms = dist / (frame_us / 1000000);
    // double v_kts = v_ms * SG_METER_TO_NM * 3600;
    // kts_filter = (0.9 * kts_filter) + (0.1 * v_kts);
    // printf("dist = %.5f  kts est = %.2f\n", dist, kts_filter);

    double vn = navpacket->vn;
    double ve = navpacket->ve;
    double vd = navpacket->vd;

    if ( use_ground_track_hdg ) {
        ogc->heading = (SGD_PI * 0.5 - atan2(vn, ve)) * SG_RADIANS_TO_DEGREES;
    }

    if ( ogc->heading < 0 ) { ogc->heading += 360.0; }

    double mps = 0.0;
    if ( use_ground_speed ) {
        mps = sqrt( vn*vn + ve*ve + vd*vd );
    } else {
        mps = imupacket->Pt;
    }
    // double mph = mps * 3600 / 1609.3440;
    double kts = mps * SG_MPS_TO_KT;
    ogc->v_kcas = kts;
    // printf("speed = %.2f mph  %.2f kts\n", mph, kts );
    static double Ps = 0.0, Ps_last = 0.0, t_last = 0.0;
    Ps_last = Ps;
    Ps = 0.92 * Ps + 0.08 * imupacket->Ps;
    double climb = (Ps - Ps_last) / (imupacket->time - t_last);
    t_last = imupacket->time;
    static double climbf = 0.0;
    climbf = 0.994 * climbf + 0.006 * climb;
    ogc->vvi = climbf; // fps

    // uncomment one of the following schemes for setting elevation:

    // use the navigation (inertially augmented gps estimate)
    // ogc->altitude = ogc->elevation
    //     = (navpacket->alt + alt_offset * SG_METER_TO_FEET);

    // use estimate error between pressure sensor and gps altitude over time
    // use pressure sensor + error average for altitude estimate.
    static double Ps_error = 0.0;
    static double Ps_count = 0;
    const double span = 10000.0;
    Ps_count += 1.0; if (Ps_count > (span-1.0)) { Ps_count = (span-1.0); }
    double error = navpacket->alt - Ps;
    Ps_error = (Ps_count/span) * Ps_error + ((span-Ps_count)/span) * error;
    ogc->elevation = (Ps +  Ps_error) * SG_METER_TO_FEET;

    /* printf("%.3f, %.3f, %.3f, %.3f, %.8f, %.8f, %.3f, %.3f, %.3f, %.3f, %.3f\n",
           imupacket->time, imupacket->the, -navpacket->vd, climbf,
           navpacket->lat, navpacket->lon, gpspacket->alt, navpacket->alt,
           imupacket->Ps, Ps, Ps + Ps_error); */

    if ( est_controls ) {
        static float est_elev = 0.0;
        static float est_aileron = 0.0;
        static float est_rudder = 0.0;
        est_elev = 0.99 * est_elev + 0.01 * (imupacket->q * 4);
        est_aileron = 0.95 * est_aileron + 0.05 * (imupacket->p * 5);
        est_rudder = 0.95 * est_rudder + 0.05 * (imupacket->r * 2);
        ogc->elevator = -est_elev;
        ogc->left_aileron = est_aileron;
        ogc->right_aileron = -est_aileron;
        ogc->rudder = est_rudder;
    } else {
        ogc->elevator = 1.0 - ((double)servopacket->chn[1] / 32768.0);
        ogc->left_aileron = 1.0 - ((double)servopacket->chn[0] / 32768.0);
        ogc->right_aileron = ((double)servopacket->chn[0] / 32768.0) - 1.0;
        ogc->rudder = 1.0 - ((double)servopacket->chn[3] / 32768.0);
    }
    ogc->elevator *= 4.0;
    ogc->left_aileron *= 4.0;
    ogc->right_aileron *= 4.0;
    ogc->rudder *= 4.0;

    // additional "abused" data fields
    ogc->egt[0] = ogc->bank - healthpacket->target_roll_deg; // flight director target roll
    ogc->egt[1] = -ogc->pitch + healthpacket->target_pitch_deg; // flight director target pitch
    ogc->egt[2] = healthpacket->target_heading_deg; // target heading bug
    ogc->egt[3] = healthpacket->target_climb_fps;   // target VVI bug
    ogc->epr[0] = healthpacket->target_altitude_ft; // target altitude bug
    ogc->epr[1] = 30.0;                              // target speed bug
    ogc->epr[2] = gps_status;   // gps status box
}


static void send_data_udp( gps *gpspacket, imu *imupacket, nav *navpacket,
                           servo *servopacket, health *healthpacket )
{
    int len;
    int ogcsize = sizeof( ogcFGData );
    int fdmsize = sizeof( FGNetFDM );
    // int ctrlsize = sizeof( FGNetCtrls );

    // cout << "Running main loop" << endl;

    ogcFGData fgogc;
    FGNetFDM fgfdm;
    FGNetCtrls fgctrls;

    ugear2fg( gpspacket, imupacket, navpacket, servopacket, healthpacket,
	      &fgfdm, &fgctrls );
    ugear2opengc( gpspacket, imupacket, navpacket, servopacket, healthpacket,
                  &fgogc );
    len = opengc_sock.send(&fgogc, ogcsize, 0);
    len = fdm_sock.send(&fgfdm, fdmsize, 0);
    // len = ctrls_sock.send(&fgctrls, ctrlsize, 0);
}


void usage( const string &argv0 ) {
    cout << "Usage: " << argv0 << endl;
    cout << "\t[ --help ]" << endl;
    cout << "\t[ --infile <infile_name>" << endl;
    cout << "\t[ --flight <flight_dir>" << endl;
    cout << "\t[ --serial <dev_name>" << endl;
    cout << "\t[ --outfile <outfile_name> (capture the data to a file)" << endl;
    cout << "\t[ --hertz <hertz> ]" << endl;
    cout << "\t[ --host <hostname> ]" << endl;
    cout << "\t[ --broadcast ]" << endl;
    cout << "\t[ --opengc-port <opengc output port #> ]" << endl;
    cout << "\t[ --fdm-port <fdm output port #> ]" << endl;
    cout << "\t[ --ctrls-port <ctrls output port #> ]" << endl;
    cout << "\t[ --groundtrack-heading ]" << endl;
    cout << "\t[ --ground-speed ]" << endl;
    cout << "\t[ --estimate-control-deflections ]" << endl;
    cout << "\t[ --altitude-offset <meters> ]" << endl;
    cout << "\t[ --skip-seconds <seconds> ]" << endl;
    cout << "\t[ --no-real-time ]" << endl;
    cout << "\t[ --ignore-checksum ]" << endl;
    cout << "\t[ --sg-swap ]" << endl;
}


int main( int argc, char **argv ) {
    double hertz = 60.0;
    string out_host = "localhost";
    bool do_broadcast = false;

    // process command line arguments
    for ( int i = 1; i < argc; ++i ) {
        if ( strcmp( argv[i], "--help" ) == 0 ) {
            usage( argv[0] );
            exit( 0 );
        } else if ( strcmp( argv[i], "--hertz" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                hertz = atof( argv[i] );
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--infile" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                infile = argv[i];
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--flight" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                flight_dir = argv[i];
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--outfile" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                outfile = argv[i];
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--serial" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                serialdev = argv[i];
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--host" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                out_host = argv[i];
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--broadcast" ) == 0 ) {
	  do_broadcast = true;
        } else if ( strcmp( argv[i], "--opengc-port" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                opengc_port = atoi( argv[i] );
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--fdm-port" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                fdm_port = atoi( argv[i] );
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--ctrls-port" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                ctrls_port = atoi( argv[i] );
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp (argv[i], "--groundtrack-heading" ) == 0 ) {
            use_ground_track_hdg = true;
        } else if ( strcmp (argv[i], "--ground-speed" ) == 0 ) {
            use_ground_speed = true;
        } else if (strcmp (argv[i], "--estimate-control-deflections" ) == 0) {
            est_controls = true;
        } else if ( strcmp( argv[i], "--altitude-offset" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                alt_offset = atof( argv[i] );
            } else {
                usage( argv[0] );
                exit( -1 );
            }
        } else if ( strcmp( argv[i], "--skip-seconds" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                skip = atof( argv[i] );
            } else {
                usage( argv[0] );
                exit( -1 );
            }
	} else if ( strcmp( argv[i], "--no-real-time" ) == 0 ) {
            run_real_time = false;
	} else if ( strcmp( argv[i], "--ignore-checksum" ) == 0 ) {
            ignore_checksum = true;
	} else if ( strcmp( argv[i], "--sg-swap" ) == 0 ) {
            sg_swap = true;
        } else {
            usage( argv[0] );
            exit( -1 );
        }
    }

    // Setup up outgoing network connections

    simgear::Socket::initSockets(); // We must call this before any other net stuff

    if ( ! opengc_sock.open( false ) ) {  // open a UDP socket
        cout << "error opening opengc output socket" << endl;
        return -1;
    }
    if ( ! fdm_sock.open( false ) ) {  // open a UDP socket
        cout << "error opening fdm output socket" << endl;
        return -1;
    }
    if ( ! ctrls_sock.open( false ) ) {  // open a UDP socket
        cout << "error opening ctrls output socket" << endl;
        return -1;
    }
    cout << "open net channels" << endl;

    opengc_sock.setBlocking( false );
    fdm_sock.setBlocking( false );
    ctrls_sock.setBlocking( false );
    cout << "blocking false" << endl;

    if ( do_broadcast ) {
        opengc_sock.setBroadcast( true );
        fdm_sock.setBroadcast( true );
        ctrls_sock.setBroadcast( true );
    }

    if ( opengc_sock.connect( out_host.c_str(), opengc_port ) == -1 ) {
        perror("connect");
        cout << "error connecting to outgoing opengc port: " << out_host
	     << ":" << opengc_port << endl;
        return -1;
    }
    cout << "connected outgoing opengc socket" << endl;

    if ( fdm_sock.connect( out_host.c_str(), fdm_port ) == -1 ) {
        perror("connect");
        cout << "error connecting to outgoing fdm port: " << out_host
	     << ":" << fdm_port << endl;
        return -1;
    }
    cout << "connected outgoing fdm socket" << endl;

    if ( ctrls_sock.connect( out_host.c_str(), ctrls_port ) == -1 ) {
        perror("connect");
        cout << "error connecting to outgoing ctrls port: " << out_host
	     << ":" << ctrls_port << endl;
        return -1;
    }
    cout << "connected outgoing ctrls socket" << endl;

    if ( sg_swap ) {
        track.set_stargate_swap_mode();
    }

    UGTelnet telnet( 5402 );
    telnet.open();

    if ( infile.length() || flight_dir.length() ) {
        if ( infile.length() ) {
            // Load data from a stream log data file
            track.load_stream( infile, ignore_checksum );
        } else if ( flight_dir.length() ) {
            // Load data from a flight directory
            track.load_flight( flight_dir );
        }
        cout << "Loaded " << track.gps_size() << " gps records." << endl;
        cout << "Loaded " << track.imu_size() << " imu records." << endl;
        cout << "Loaded " << track.nav_size() << " nav records." << endl;
        cout << "Loaded " << track.servo_size() << " servo records." << endl;
        cout << "Loaded " << track.health_size() << " health records." << endl;

        int size = track.imu_size();

        double current_time = track.get_imupt(0).time;
        cout << "Track begin time is " << current_time << endl;
        double end_time = track.get_imupt(size-1).time;
        cout << "Track end time is " << end_time << endl;
        cout << "Duration = " << end_time - current_time << endl;

        if ( track.gps_size() > 0 ) {
            double tmp = track.get_gpspt(track.gps_size()-1).ITOW;
            int days = (int)(tmp / (24 * 60 * 60));
            tmp -= days * 24 * 60 * 60;
            int hours = (int)(tmp / (60 * 60));
            tmp -= hours * 60 * 60;
            int min = (int)(tmp / 60);
            tmp -= min * 60;
            double sec = tmp;
            printf("[GPS  ]:ITOW= %.3f[sec]  %dd %02d:%02d:%06.3f\n",
                   tmp, days, hours, min, sec);
        }


        // advance skip seconds forward
        current_time += skip;

        frame_us = 1000000.0 / hertz;
        if ( frame_us < 0.0 ) {
            frame_us = 0.0;
        }

        SGTimeStamp start_time;
        start_time.stamp();
        int gps_count = 0;
        int imu_count = 0;
        int nav_count = 0;
        int servo_count = 0;
        int health_count = 0;

        gps gps0, gps1;
        gps0 = gps1 = track.get_gpspt( 0 );
    
        imu imu0, imu1;
        imu0 = imu1 = track.get_imupt( 0 );
    
        nav nav0, nav1;
        nav0 = nav1 = track.get_navpt( 0 );
    
        servo servo0, servo1;
        servo0 = servo1 = track.get_servopt( 0 );
    
        health health0, health1;
        health0 = health1 = track.get_healthpt( 0 );

        double last_lat = -999.9, last_lon = -999.9;

        printf("<gpx>\n");
        printf(" <trk>\n");
        printf("  <trkseg>\n");
        while ( current_time < end_time ) {
            // cout << "current_time = " << current_time << " end_time = "
            //      << end_time << endl;

            // Advance gps pointer
            while ( current_time > gps1.time
                    && gps_count < track.gps_size() - 1 )
            {
                gps0 = gps1;
                ++gps_count;
                // cout << "count = " << count << endl;
                gps1 = track.get_gpspt( gps_count );
            }
            // cout << "p0 = " << p0.get_time() << " p1 = " << p1.get_time()
            //      << endl;

            // Advance imu pointer
            while ( current_time > imu1.time
                    && imu_count < track.imu_size() - 1 )
            {
                imu0 = imu1;
                ++imu_count;
                // cout << "count = " << count << endl;
                imu1 = track.get_imupt( imu_count );
            }
            //  cout << "pos0 = " << pos0.get_seconds()
            // << " pos1 = " << pos1.get_seconds() << endl;

            // Advance nav pointer
            while ( current_time > nav1.time
                    && nav_count < track.nav_size() - 1 )
            {
                nav0 = nav1;
                ++nav_count;
                // cout << "nav count = " << nav_count << endl;
                nav1 = track.get_navpt( nav_count );
            }
            //  cout << "pos0 = " << pos0.get_seconds()
            // << " pos1 = " << pos1.get_seconds() << endl;

            // Advance servo pointer
            while ( current_time > servo1.time
                    && servo_count < track.servo_size() - 1 )
            {
                servo0 = servo1;
                ++servo_count;
                // cout << "count = " << count << endl;
                servo1 = track.get_servopt( servo_count );
            }
            //  cout << "pos0 = " << pos0.get_seconds()
            // << " pos1 = " << pos1.get_seconds() << endl;

            // Advance health pointer
            while ( current_time > health1.time
                    && health_count < track.health_size() - 1 )
            {
                health0 = health1;
                ++health_count;
                // cout << "count = " << count << endl;
                health1 = track.get_healthpt( health_count );
            }
            //  cout << "pos0 = " << pos0.get_seconds()
            // << " pos1 = " << pos1.get_seconds() << endl;

            double gps_percent;
            if ( fabs(gps1.time - gps0.time) < 0.00001 ) {
                gps_percent = 0.0;
            } else {
                gps_percent =
                    (current_time - gps0.time) /
                    (gps1.time - gps0.time);
            }
            // cout << "Percent = " << percent << endl;

            double imu_percent;
            if ( fabs(imu1.time - imu0.time) < 0.00001 ) {
                imu_percent = 0.0;
            } else {
                imu_percent =
                    (current_time - imu0.time) /
                    (imu1.time - imu0.time);
            }
            // cout << "Percent = " << percent << endl;

            double nav_percent;
            if ( fabs(nav1.time - nav0.time) < 0.00001 ) {
                nav_percent = 0.0;
            } else {
                nav_percent =
                    (current_time - nav0.time) /
                    (nav1.time - nav0.time);
            }
            // cout << "Percent = " << percent << endl;

            double servo_percent;
            if ( fabs(servo1.time - servo0.time) < 0.00001 ) {
                servo_percent = 0.0;
            } else {
                servo_percent =
                    (current_time - servo0.time) /
                    (servo1.time - servo0.time);
            }
            // cout << "Percent = " << percent << endl;

            double health_percent;
            if ( fabs(health1.time - health0.time) < 0.00001 ) {
                health_percent = 0.0;
            } else {
                health_percent =
                    (current_time - health0.time) /
                    (health1.time - health0.time);
            }
            // cout << "Percent = " << percent << endl;

            gps gpspacket = UGEARInterpGPS( gps0, gps1, gps_percent );
            imu imupacket = UGEARInterpIMU( imu0, imu1, imu_percent );
            nav navpacket = UGEARInterpNAV( nav0, nav1, nav_percent );
            servo servopacket = UGEARInterpSERVO( servo0, servo1,
						  servo_percent );
            health healthpacket = UGEARInterpHEALTH( health0, health1,
						  health_percent );

            // cout << current_time << " " << p0.lat_deg << ", " << p0.lon_deg
            //      << endl;
            // cout << current_time << " " << p1.lat_deg << ", " << p1.lon_deg
            //      << endl;
            // cout << (double)current_time << " " << pos.lat_deg << ", "
            //      << pos.lon_deg << " " << att.yaw_deg << endl;
            if ( gpspacket.lat > -500 ) {
                // printf( "%.3f  %.4f %.4f %.1f  %.2f %.2f %.2f\n",
                //         current_time,
                //         navpacket.lat, navpacket.lon, navpacket.alt,
                //         imupacket.psi, imupacket.the, imupacket.phi );
                double dlat = last_lat - navpacket.lat;
                double dlon = last_lon - navpacket.lon;
                double dist = sqrt( dlat*dlat + dlon*dlon );
                if ( dist > 0.01 ) {
                    printf("   <trkpt lat=\"%.8f\" lon=\"%.8f\"></trkpt>\n",
                           navpacket.lat, navpacket.lon );
                    // printf(" </wpt>\n");
                    last_lat = navpacket.lat;
                    last_lon = navpacket.lon;
                }
	    }

            if ( (fabs(gpspacket.lat) < 0.0001 &&
                  fabs(gpspacket.lon) < 0.0001 &&
                  fabs(gpspacket.alt) < 0.0001) )
            {
                printf("WARNING: LOST GPS!!!\n");
                gps_status = -1.0;
            } else {
                gps_status = 1.0;
            }

            send_data_udp( &gpspacket, &imupacket, &navpacket, &servopacket,
                           &healthpacket );

            if ( run_real_time ) {
                // Update the elapsed time.
                static bool first_time = true;
                if ( first_time ) {
                    last_time_stamp.stamp();
                    first_time = false;
                }

                current_time_stamp.stamp();
                /* Convert to ms */
                double elapsed_us = (current_time_stamp - last_time_stamp).toUSecs();
                if ( elapsed_us < (frame_us - 2000) ) {
                    double requested_us = (frame_us - elapsed_us) - 2000 ;
                    ulMilliSecondSleep ( (int)(requested_us / 1000.0) ) ;
                }
                current_time_stamp.stamp();
                while ( (current_time_stamp - last_time_stamp).toUSecs() < frame_us ) {
                    current_time_stamp.stamp();
                }
            }

            current_time += (frame_us / 1000000.0);
            last_time_stamp = current_time_stamp;
        }

        printf("   <trkpt lat=\"%.8f\" lon=\"%.8f\"></trkpt>\n",
               nav1.lat, nav1.lon );

        printf("  </trkseg>\n");
        printf(" </trk>\n");
        nav0 = track.get_navpt( 0 );
        nav1 = track.get_navpt( track.nav_size() - 1 );
        printf(" <wpt lat=\"%.8f\" lon=\"%.8f\"></wpt>\n",
               nav0.lat, nav0.lon );
        printf(" <wpt lat=\"%.8f\" lon=\"%.8f\"></wpt>\n",
               nav1.lat, nav1.lon );
        printf("<gpx>\n");

        cout << "Processed " << imu_count << " entries in "
             << current_time_stamp - start_time << " seconds."
             << endl;
    } else if ( serialdev.length() ) {
        // process incoming data from the serial port

        int count = 0;
        double current_time = 0.0;
        double last_time = 0.0;

        gps gpspacket; bzero( &gpspacket, sizeof(gpspacket) );
	imu imupacket; bzero( &imupacket, sizeof(imupacket) );
	nav navpacket; bzero( &navpacket, sizeof(navpacket) );
	servo servopacket; bzero( &servopacket, sizeof(servopacket) );
	health healthpacket; bzero( &healthpacket, sizeof(healthpacket) );

        double gps_time = 0.0;
        double imu_time = 0.0;
        double nav_time = 0.0;
        double servo_time = 0.0;
        double health_time = 0.0;
        double command_time = 0.0;
        double command_heartbeat = 0.0;

        // open the serial port device
        SGSerialPort uavcom( serialdev, 115200 );
        if ( !uavcom.is_enabled() ) {
            cout << "Cannot open: " << serialdev << endl;
            return false;
        }

        // open up the data log file if requested
        if ( !outfile.length() ) {
            cout << "no --outfile <name> specified, cannot capture data!"
                 << endl;
            return false;
        }
        SGFile log( outfile );
        if ( !log.open( SG_IO_OUT ) ) {
            cout << "Cannot open: " << outfile << endl;
            return false;
        }

        // add some test commands
        //command_mgr.add("ap,alt,1000");
        //command_mgr.add("home,158.0,32.5");
        //command_mgr.add("go,home");
        //command_mgr.add("go,route");

        while ( uavcom.is_enabled() ) {
	    // cout << "looking for next message ..." << endl;
            int id = track.next_message( &uavcom, &log, &gpspacket,
					 &imupacket, &navpacket, &servopacket,
					 &healthpacket, ignore_checksum );
            // cout << "message id = " << id << endl;
            count++;

            telnet.process();

            if ( id == GPS_PACKET ) {
                if ( gpspacket.time > gps_time ) {
                    gps_time = gpspacket.time;
                    current_time = gps_time;
                } else {
		  cout << "oops gps back in time: " << gpspacket.time << " " << gps_time << endl;
                }
	    } else if ( id == IMU_PACKET ) {
                if ( imupacket.time > imu_time ) {
                    imu_time = imupacket.time;
                    current_time = imu_time;
                } else {
                    cout << "oops imu back in time: " << imupacket.time << " " << imu_time << endl;
                }
	    } else if ( id == NAV_PACKET ) {
                if ( navpacket.time > nav_time ) {
                    nav_time = navpacket.time;
                    current_time = nav_time;
                } else {
                    cout << "oops nav back in time: " << navpacket.time << " " << nav_time << endl;
                }
	    } else if ( id == SERVO_PACKET ) {
                if ( servopacket.time > servo_time ) {
                    servo_time = servopacket.time;
                    current_time = servo_time;
                } else {
                    cout << "oops servo back in time: " << servopacket.time << " " << servo_time << endl;
                }
	    } else if ( id == HEALTH_PACKET ) {
                if ( healthpacket.time > health_time ) {
                    health_time = healthpacket.time;
                    current_time = health_time;
                    printf("Received a health packet, sequence: %d\n",
                           (int)healthpacket.command_sequence);
                    command_mgr.update_cmd_sequence(healthpacket.command_sequence);
                } else {
                    cout << "oops health back in time: " << healthpacket.time << " " << health_time << endl;
                }
                
            }

            if ( (current_time > gps_time + 2) ||
                 (fabs(gpspacket.lat) < 0.0001 &&
                  fabs(gpspacket.lon) < 0.0001 &&
                  fabs(gpspacket.alt) < 0.0001) )
            {
                printf("WARNING: LOST GPS!!!\n");
                gps_status = -1.0;
            } else {
                gps_status = 1.0;
            }

            // Generate a ground station heart beat every 4 seconds
            if ( current_time >= command_heartbeat + 4 ) {
                command_mgr.add("hb");
                command_heartbeat = current_time;
            }
                
            // Command update @ 1hz
            if ( current_time >= command_time + 1 ) {
                command_mgr.update(&uavcom);
                command_time = current_time;
            }

            // Relay data on to FlightGear and LFSTech Glass
            if ( current_time >= last_time + (1/hertz) ) {
                // if ( gpspacket.lat > -500 ) {
                int londeg = (int)navpacket.lon;
                // double lonmin = fabs(navpacket.lon - londeg);
                int latdeg = (int)navpacket.lat;
                // double latmin = fabs(navpacket.lat - latdeg);
                char londir = 'E'; if ( londeg < 0 ) londir = 'W';
                char latdir = 'N'; if ( latdeg < 0 ) latdir = 'S';
                londeg = abs(londeg);
                latdeg = abs(latdeg);
                /*printf( "%.2f  %c%02d:%.4f %c%03d:%.4f %.1f  %.2f %.2f %.2f\n",
                        current_time,
                        latdir, latdeg, latmin, londir, londeg, lonmin,
                        navpacket.alt,
                        imupacket.phi*SGD_RADIANS_TO_DEGREES,
                        imupacket.the*SGD_RADIANS_TO_DEGREES,
                        imupacket.psi*SGD_RADIANS_TO_DEGREES ); */
                // }

                last_time = current_time;
                send_data_udp( &gpspacket, &imupacket, &navpacket,
                               &servopacket, &healthpacket );
            }
        }
    }

    return 0;
}

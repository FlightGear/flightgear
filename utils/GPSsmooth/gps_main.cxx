#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef HAVE_WINDOWS_H
#  include <windows.h>
#else
#  include <netinet/in.h>       // htonl() ntohl()
#endif

#include <iostream>
#include <string>

#include <plib/sg.h>

#include <simgear/io/lowlevel.hxx> // endian tests
#include <simgear/io/raw_socket.hxx>
#include <simgear/timing/timestamp.hxx>

#include <Network/net_ctrls.hxx>
#include <Network/net_fdm.hxx>

#include "gps.hxx"


using std::cout;
using std::endl;
using std::string;


// Network channels
static simgear::Socket fdm_sock, ctrls_sock;

// gps data
GPSTrack track;

// Default ports
static int fdm_port = 5505;
static int ctrls_port = 5506;

// Default path
static string file = "";

// Master time counter
float sim_time = 0.0f;

// sim control
SGTimeStamp last_time_stamp;
SGTimeStamp current_time_stamp;

bool inited = false;


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


static void gps2fg( const GPSPoint p, FGNetFDM *fdm, FGNetCtrls *ctrls )
{
    unsigned int i;

    static double last_psi;
    static double last_alt;
    static double phi_filter = 0.0;
    static double theta_filter = 0.0;

    // Nan-be-gone
    if ( phi_filter != phi_filter ) {
        phi_filter = 0.0;
    }
    if ( theta_filter != theta_filter ) {
        theta_filter = 0.0;
    }

    // Version sanity checking
    fdm->version = FG_NET_FDM_VERSION;

    // Aero parameters
    fdm->longitude = p.lon_deg * SGD_DEGREES_TO_RADIANS;
    fdm->latitude = p.lat_deg * SGD_DEGREES_TO_RADIANS;
    fdm->altitude = p.altitude_msl;
    fdm->agl = -9999.0;
    fdm->psi = p.course_true; // heading

    double diff = p.course_true - last_psi;
    if ( diff < -SGD_PI ) { diff += 2.0*SGD_PI; }
    if ( diff > SGD_PI ) { diff -= 2.0*SGD_PI; }
    double phi = diff * 100.0;
    if ( phi > 0.5*SGD_PI ) { phi = 0.5*SGD_PI; }
    if ( phi < -0.5*SGD_PI ) { phi = -0.5*SGD_PI; }
    phi_filter = 0.99*phi_filter + 0.01*phi;
    fdm->phi = phi_filter;
    last_psi = p.course_true;
    // cout << p.course_true << endl;

    diff = p.altitude_msl - last_alt;
    if ( diff < -SGD_PI ) { diff += 2.0*SGD_PI; }
    if ( diff > SGD_PI ) { diff -= 2.0*SGD_PI; }
    double theta = diff * 2.0;
    if ( theta > 0.5*SGD_PI ) { theta = 0.5*SGD_PI; }
    if ( theta < -0.5*SGD_PI ) { theta = -0.5*SGD_PI; }
    theta_filter = 0.99*theta_filter + 0.01*theta;
    fdm->theta = theta_filter;
    last_alt = p.altitude_msl;

    fdm->phidot = 0.0;
    fdm->thetadot = 0.0;
    fdm->psidot = 0.0;
    fdm->vcas = p.speed_kts;
    fdm->climb_rate = 0; // fps
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
    double rpm = ((p.speed_kts - 15.0) / 65.0) * 2000.0 + 500.0;
    if ( rpm < 0.0 ) { rpm = 0.0; }
    if ( rpm > 3000.0 ) { rpm = 3000.0; }
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

    fdm->elevator = -theta_filter * 5.0;
    fdm->elevator_trim_tab = 0.0;
    fdm->left_flap = 0.0;
    fdm->right_flap = 0.0;
    fdm->left_aileron = phi_filter * 1.5;
    fdm->right_aileron = phi_filter * 1.5;
    fdm->rudder = 0.0;
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
}


static void send_data( const GPSPoint p ) {
    int len;
//    int ctrlsize = sizeof( FGNetCtrls );
    int fdmsize = sizeof( FGNetFDM );

    // cout << "Running main loop" << endl;

    FGNetFDM fgfdm;
    FGNetCtrls fgctrls;

    gps2fg( p, &fgfdm, &fgctrls );
    len = fdm_sock.send(&fgfdm, fdmsize, 0);
}


void usage( const string &argv0 ) {
    cout << "Usage: " << argv0 << endl;
    cout << "\t[ --help ]" << endl;
    cout << "\t[ --file <file_name>" << endl;
    cout << "\t[ --hertz <hertz> ]" << endl;
    cout << "\t[ --host <hostname> ]" << endl;
    cout << "\t[ --broadcast ]" << endl;
    cout << "\t[ --fdm-port <fdm output port #> ]" << endl;
    cout << "\t[ --ctrls-port <ctrls output port #> ]" << endl;
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
        } else if ( strcmp( argv[i], "--file" ) == 0 ) {
            ++i;
            if ( i < argc ) {
                file = argv[i];
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
        } else {
            usage( argv[0] );
            exit( -1 );
        }
    }

    // Load the track data
    if ( file == "" ) {
        cout << "No track file specified" << endl;
        exit(-1);
    }
    track.load( file );
    cout << "Loaded " << track.size() << " records." << endl;

    // Setup up outgoing network connections

    simgear::Socket::initSockets(); // We must call this before any other net stuff

    if ( ! fdm_sock.open( false ) ) {  // open a UDP socket
        cout << "error opening fdm output socket" << endl;
        return -1;
    }
    if ( ! ctrls_sock.open( false ) ) {  // open a UDP socket
        cout << "error opening ctrls output socket" << endl;
        return -1;
    }
    cout << "open net channels" << endl;

    fdm_sock.setBlocking( false );
    ctrls_sock.setBlocking( false );
    cout << "blocking false" << endl;

    if ( do_broadcast ) {
        fdm_sock.setBroadcast( true );
        ctrls_sock.setBroadcast( true );
    }

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

    int size = track.size();

    double current_time = track.get_point(0).get_time();
    cout << "Track begin time is " << current_time << endl;
    double end_time = track.get_point(size-1).get_time();
    cout << "Track end time is " << end_time << endl;
    cout << "Duration = " << end_time - current_time << endl;

    double frame_us = 1000000.0 / hertz;
    if ( frame_us < 0.0 ) {
        frame_us = 0.0;
    }

    SGTimeStamp start_time;
    start_time.stamp();
    int count = 0;

    GPSPoint p, p0, p1;
    p0 = p1 = track.get_point( 0 );
    
    while ( current_time < end_time ) {
        // cout << "current_time = " << current_time << " end_time = "
        //      << end_time << endl;

        if ( current_time > p1.get_time() ) {
            p0 = p1;
            ++count;
            // cout << "count = " << count << endl;
            p1 = track.get_point( count );
        }
        // cout << "p0 = " << p0.get_time() << " p1 = " << p1.get_time()
        //      << endl;

        double percent;
        if ( fabs(p1.get_time() - p0.get_time()) < 0.0001 ) {
            percent = 0.0;
        } else {
            percent =
                (current_time - p0.get_time()) /
                (p1.get_time() - p0.get_time());
        }
        // cout << "Percent = " << percent << endl;

        GPSPoint p = GPSInterpolate( p0, p1, percent );
        // cout << current_time << " " << p0.lat_deg << ", " << p0.lon_deg << endl;
        // cout << current_time << " " << p1.lat_deg << ", " << p1.lon_deg << endl;
        cout << current_time << " " << p.lat_deg << ", " << p.lon_deg << endl;

        send_data( p );

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

        current_time += (frame_us / 1000000.0);
        last_time_stamp = current_time_stamp;
    }

    cout << "Processed " << count << " entries in "
         << current_time_stamp - start_time << " seconds." << endl;

    return 0;
}

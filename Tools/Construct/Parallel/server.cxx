// remote_server.c -- Written by Curtis Olson
//                 -- for CSci 5502


#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>  // for stat()
#include <unistd.h>

#include <sys/socket.h>  // bind
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <string>

#include <Bucket/newbucket.hxx>


#define MAXBUF 1024


static double lat = 0.0;
static double lon = 0.0;
static double dy = 0.0;
static int pass = 0;


int make_socket (unsigned short int* port) {
    int sock;
    struct sockaddr_in name;
    socklen_t length;
     
    // Create the socket.
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	perror ("socket");
	exit (EXIT_FAILURE);
    }
     
    // Give the socket a name.
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = 0 /* htons (port) */;
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
	perror ("bind");
	exit (EXIT_FAILURE);
    }
     
    // Find the assigned port number
    length = sizeof(struct sockaddr_in);
    if ( getsockname(sock, (struct sockaddr *) &name, &length) ) {
	perror("Cannot get socket's port number");
    }
    *port = ntohs(name.sin_port);

    return sock;
}


// return true if file exists
static bool file_exists( const string& file ) {
    struct stat buf;

    if ( stat( file.c_str(), &buf ) == 0 ) {
	return true;
    } else {
	return false;
    }
}


// check if the specified tile has data defined for it
static bool has_data( const string& path, const FGBucket& b ) {
    string dem_file = path + ".dem" + "/Scenery/" + b.gen_base_path()
	+ "/" + b.gen_index_str() + ".dem";
    if ( file_exists( dem_file ) ) {
	return true;
    }

    dem_file += ".gz";
    if ( file_exists( dem_file ) ) {
	return true;
    }

    return false;
}


// initialize the tile counting system
void init_tile_count() {
    // pre-pass
    pass = 0;

    // initial bogus value
    lat = 100;

    // determine tile height
    FGBucket tmp1( 0.0, 0.0 );
    dy = tmp1.get_height();
}


// return the next tile
long int get_next_tile( const string& work_base )
{
    FGBucket b;
    static double shift_over = 0.0;
    static double shift_up = 0.0;

    // cout << "lon = " << lon << " lat = " << lat << endl;

    if ( lat > 90.0 ) {
	++pass;
	if ( pass == 1 ) {
	    shift_over = 0.0;
	    shift_up = 0.0;
	} else if ( pass == 2 ) {
	    shift_over = 1.0;
	    shift_up = 0.0;
	} else if ( pass == 3 ) {
	    shift_over = 0.0;
	    shift_up = 1.0;
	} else if ( pass == 4 ) {
	    shift_over = 1.0;
	    shift_up = 1.0;
	} else {
	    return -1;
	}

	// reset lat
	// lat = -89.0 + (shift_up*dy) - (dy*0.5);
	lat = 15.0 + (shift_up*dy) + (dy*0.5);

	// reset lon
	FGBucket tmp( 0.0, lat );
	double dx = tmp.get_width();
	lon = -180 + (shift_over*dx) + (dx*0.5);

	cout << "starting pass = " << pass 
	     << " with lat = " << lat << " lon = " << lon << endl;
    }

    // if ( ! start_lon ) {
    // lon = -180 + dx * 0.5;
    // } else {
    //    start_lon = false;
    // }

    if ( lon > 180.0 ) {
	// increment to next row
	// skip every other row (to avoid two clients working on
	// adjacent tiles)
	lat += 2.0 * dy;

	FGBucket tmp( 0.0, lat );
	double dx = tmp.get_width();
	lon = -180 + (shift_over*dx) + (dx*0.5);
    }

    b = FGBucket( lon, lat );
    cout << "Bucket = " << b << " (" << pass << ")" << endl;

    // increment to next tile
    FGBucket tmp( 0.0, lat );
    double dx = tmp.get_width();

    // skip every other column (to avoid two clients working on
    // adjacent tiles)
    lon += 2.0 * dx;

    return b.gen_index();
}


// log a pending tile (has been given out as a taks for some client)
void log_pending_tile( const string& path, long int tile ) {
    FGBucket b(tile);

    string pending_file = path + "/" + b.gen_index_str() + ".pending";

    string command = "touch " + pending_file;
    system( command.c_str() );
}


// a tile is finished (removed the .pending file)
void log_finished_tile( const string& path, long int tile ) {
    FGBucket b(tile);

    string finished_file = path + "/" + b.gen_index_str() + ".pending";
    // cout << "unlinking " << finished_file << endl;
    unlink( finished_file.c_str() );
}


// make note of a failed tile
void log_failed_tile( const string& path, long int tile ) {
    FGBucket b(tile);

    string failed_file = path + "/" + b.gen_index_str() + ".failed";

    string command = "touch " + failed_file;
    system( command.c_str() );

    cout << "logged bad tile = " << tile << endl;
}


// display usage and exit
void usage( const string name ) {
    cout << "Usage: " << name << " <work_base> <output_base>" << endl;
    exit(-1);
}


int main( int argc, char **argv ) {
    long int next_tile;
    int sock, msgsock, length, pid;
    fd_set ready;
    short unsigned int port;

    // quick argument sanity check
    if ( argc < 3 ) {
	usage( argv[0] );
    }

    string work_base = argv[1];
    string output_base = argv[2];

    // initialize tile counter / incrementer
    init_tile_count();

    // create the status directory
    string status_dir = work_base + ".status";
    string command = "mkdir -p " + status_dir;
    system( command.c_str() );

    // setup socket to listen on
    sock = make_socket( &port );
    cout << "socket is connected to port = " << port << endl;

    // Specify the maximum length of the connection queue
    listen(sock, 10);

    for ( ;; ) {
	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	
	// block until we get some input on sock
	select(32, &ready, 0, 0, NULL);

	if ( FD_ISSET(sock, &ready) ) {
	    // printf("%d %d Incomming message --> ", getpid(), pid);

	    // get the next tile to work on
	    next_tile = get_next_tile( work_base );
	    while ( ! has_data( work_base, FGBucket(next_tile) ) ) {
		next_tile = get_next_tile( work_base );
	    }

	    log_pending_tile( status_dir, next_tile );
	    // cout << "next tile = " << next_tile << endl;;

	    msgsock = accept(sock, 0, 0);
	    // cout << "msgsock = " << msgsock << endl;

	    // spawn a child
	    pid = fork();

	    if ( pid < 0 ) {
		// error
		perror("Cannot fork child process");
		exit(-1);
	    } else if ( pid > 0 ) {
		// This is the parent
		close(msgsock);

		// clean up all of our zombie children
		int status;
		while ( (pid = waitpid( WAIT_ANY, &status, WNOHANG )) > 0 ) {
		    // cout << "waitpid(): pid = " << pid 
		    //      << " status = " << status << endl;
		}
	    } else {
		// This is the child

		// cout << "new process started to handle new connection for "
		//      << next_tile << endl;

                // Read client's message (which is the status of the
                // last scenery creation task.)
		char buf[MAXBUF];
                if ( (length = read(msgsock, buf, MAXBUF)) < 0) {
                    perror("Cannot read command");
                    exit(-1);
                }
		buf[length] = '\0';
		long int returned_tile = atoi(buf);
		cout << "client returned = " << returned_tile << endl;

		// record status
		if ( returned_tile < 0 ) {
		    // failure
		    log_failed_tile( status_dir, -returned_tile );
		    log_finished_tile( status_dir, -returned_tile );
		} else {
		    // success
		    log_finished_tile( status_dir, returned_tile );
		}

		// reply to the client
		char message[MAXBUF];
		sprintf(message, "%ld", next_tile);
		length = strlen(message);
		if ( write(msgsock, message, length) < 0 ) {
		    perror("Cannot write to stream socket");
		}
		close(msgsock);
		// cout << "process for " << next_tile << " ended" << endl;

		exit(0);
	    }
	}
    }
}

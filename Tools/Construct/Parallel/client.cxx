/* remote_exec.c -- Written by Curtis Olson */
/*               -- for CSci 5502 */


#ifdef HAVE_CONFIG_H
#  include <Include/config.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>	// BSD macro definitions
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <utmp.h>

#include <stdio.h>
#include <stdlib.h>  // atoi()
#include <string.h>  // bcopy()

#include <iostream>
#include <string>

#include <Bucket/newbucket.hxx>


#define MAXBUF 1024
#define BUSY_WAIT_TIME 30


string work_base = "";
string output_base = "";


// check if it is ok to run
void check_master_switch() {
    string file = work_base + ".status/MASTER_ON";

    FILE *fp = fopen( file.c_str(), "r" );
    if ( fp == NULL ) {
	cout << "MASTER_ON file not found ... exiting." << endl;
	exit(0);
    }

    fclose( fp );
}


// check if the host system is free of interactive users
int system_free() {
#ifndef BSD
    struct utmp *uptr;

    setutent();

    while ( (uptr = getutent()) != NULL ) {
	// cout << "NULL = " << NULL << "  uptr = " << uptr << endl;
	// cout << "user =  ut_user = " << uptr->ut_user << endl;
	// cout << "user =  ut_type = " << uptr->ut_type << endl;
	if (uptr->ut_type == USER_PROCESS) {
	    // found someone
	    endutent();
	    return 0;
	}
    }

    endutent();
#else
#  warning Port me
#endif

    return 1;
}


int make_socket (char *host, unsigned short int port) {
    int sock;
    struct sockaddr_in name;
    struct hostent *hp;
     
    // Create the socket.
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	perror ("socket");
	exit (EXIT_FAILURE);
    }
     
    // specify address family
    name.sin_family = AF_INET;

    // get the hosts official name/info
    hp = gethostbyname(host);

    // Connect this socket to the host and the port specified on the
    // command line
    bcopy(hp->h_addr, &(name.sin_addr.s_addr), hp->h_length);
    name.sin_port = htons(port);

    if ( connect(sock, (struct sockaddr *) &name, 
		 sizeof(struct sockaddr_in)) < 0 )
    {
	close(sock);
	perror("Cannot connect to stream socket");
	return -1;
    }

    return sock;
}


// connect to the server and get the next task
long int get_next_task( const string& host, int port, long int last_tile ) {
    long int tile;
    int sock, len;
    fd_set ready;
    char message[256];

    // loop till we get a socket connection
    while ( (sock = make_socket( host.c_str(), port )) < 0 ) {
	// check if the master switch is on
	check_master_switch();

	sleep(1);
    }

    // build a command string from the argv[]'s
    sprintf(message, "%ld", last_tile);

    // send command and arguments to remote server
    if ( write(sock, message, sizeof(message)) < 0 ) {
        perror("Cannot write to stream socket");
    }

    // loop until remote program finishes
    cout << "querying server for next task ..." << endl;

    FD_ZERO(&ready);
    FD_SET(sock, &ready);

    // block until input from sock
    select(32, &ready, 0, 0, NULL);
    cout << " received reply" << endl;

    if ( FD_ISSET(sock, &ready) ) {
	/* input coming from socket */
	if ( (len = read(sock, message, 1024)) > 0 ) {
	    message[len] = '\0';
	    tile = atoi(message);
	    cout << "  tile to construct = " << tile << endl;
	    close(sock);
	    return tile;
	} else {
	    close(sock);
	    return -1;
	}
    }

    close(sock);
    return -1;
}


// build the specified tile, return true if contruction completed
// successfully
bool construct_tile( const FGBucket& b, const string& result_file ) {
    double angle = 10.0;
    bool still_trying = true;

    while ( still_trying ) {
	still_trying = false; 
	char angle_str[256];
	sprintf(angle_str, "%.0f", angle);
	string command = "fgfs-construct ";
	command += angle_str;
	command += " " + work_base + " " + output_base + " "
	    + b.gen_index_str() + " > " + result_file + " 2>&1";
	cout << command << endl;
	
	system( command.c_str() );
	
	FILE *fp = fopen( result_file.c_str(), "r" );
	char line[256];
	while ( fgets( line, 256, fp ) != NULL ) {
	    string line_str = line;
	    line_str = line_str.substr(0, line_str.length() - 1);
	    // cout << line_str << endl;
	    if ( line_str == "[Finished successfully]" ) {
		fclose(fp);
		return true;
	    } else if 
		( (line_str.substr(0, 31) == "Error:  Ran out of precision at")
		  || (line_str.substr(0, 22) == "Error:  Out of memory.")
		  || (line_str.substr(0, 23) == "Error:  Too many nodes.") ) {
		if ( angle > 9.0 ) {
		    angle = 5.0;
		    still_trying = true;
		} else if ( angle > 4.0 ) {
		    angle = 0.0;
		    still_trying = true;
		}
	    }

	}
	fclose(fp);

	if ( !still_trying && ( angle > 0.0 ) ) {
	    // build died for some reason ... lets try one last time
	    // with an interior angle restriction of 0
	    angle = 0.0;
	    still_trying = true;
	}
    }
    return false;
}


main(int argc, char *argv[]) {
    long int tile, last_tile;
    bool rude = false;
    bool result;

    // Check usage
    if ( argc < 5 ) {
	printf("Usage: %s remote_machine port work_base output_base [ -r ]\n",
	       argv[0]);
	exit(1);
    }

    string host = argv[1];
    int port = atoi( argv[2] );
    work_base = argv[3];
    output_base = argv[4];

    if ( argc == 6 ) {
	string option = argv[5];
	if ( option == "-r" ) {
	    cout << "Running in RUDE mode!" << endl;
	    rude = true;
	}
    }
    // get hostname and pid
    char hostname[MAXBUF];
    gethostname( hostname, MAXBUF );
    pid_t pid = getpid();

    char tmp[MAXBUF];
    sprintf(tmp, "result.%s.%d", hostname, pid);
    string result_file = work_base + ".status/" + tmp;

    last_tile = 0;

    // check if the master switch is on
    check_master_switch();

    while ( (tile = get_next_task( host, port, last_tile )) >= 0 ) {
	result = construct_tile( FGBucket(tile), result_file );
	if ( result ) {
	    last_tile = tile;
	} else {
	    last_tile = -tile;
	}

	// check if the master switch is on
	check_master_switch();

	// niceness policy: This whole process should run niced.  But
	// additionally, if there is interactive use, we will sleep
	// for 60 seconds between each tile to stagger out the load
	// and impose less of an impact on the machine.
	if ( !system_free() && !rude) {
	    cout << "System has interactive use, sleeping for " 
		 << BUSY_WAIT_TIME << " seconds..." << endl;
	    sleep( BUSY_WAIT_TIME );
	}
    }
}

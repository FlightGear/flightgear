/* remote_exec.c -- Written by Curtis Olson */
/*               -- for CSci 5502 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>  // atoi()
#include <string.h>  // bcopy()

#include <iostream>
#include <string>


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

    while ( (sock = make_socket( host.c_str(), port )) < 0 ) {
	// loop till we get a socket connection
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
bool construct_tile( long int tile ) {
    return true;
}


main(int argc, char *argv[]) {
    long int tile, last_tile;
    bool result;

    // Check usage
    if ( argc < 3 ) {
	printf("Usage: %s remote_machine port\n", argv[0]);
	exit(1);
    }

    string host = argv[1];
    int port = atoi( argv[2] );

    last_tile = 0;

    while ( (tile = get_next_task( host, port, last_tile )) >= 0 ) {
	result = construct_tile( tile );

	if ( result ) {
	    last_tile = tile;
	} else {
	    last_tile = -tile;
	}
    }
}

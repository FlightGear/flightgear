// remote_server.c -- Written by Curtis Olson
//                 -- for CSci 5502


#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>  // bind
#include <netinet/in.h>

#include <unistd.h>

#include <iostream>

// #include <netdb.h>
// #include <fcntl.h>
// #include <stdio.h>


#define MAXBUF 1024

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


main() {
    int sock, msgsock, length, pid;
    fd_set ready;
    short unsigned int port;
    char buf[MAXBUF];

    sock = make_socket( &port );

    // Save the port number
    // set_port( port );
    cout << "socket is connected to port = " << port << endl;

    /* Specify the maximum length of the connection queue */
    listen(sock, 3);

    for ( ;; ) {
	FD_ZERO(&ready);
	FD_SET(sock, &ready);
	
	/* block until we get some input on sock */
	select(32, &ready, 0, 0, NULL);

	if ( FD_ISSET(sock, &ready) ) {
	    /* printf("%d %d Incomming message --> ", getpid(), pid); */

	    msgsock = accept(sock, 0, 0);

	    /* spawn a child */
	    pid = fork();

	    if ( pid < 0 ) {
		/* error */
		perror("Cannot fork child process");
		exit(-1);
	    } else if ( pid > 0 ) {
		/* This is the parent */
		close(msgsock);
	    } else {
		/* This is the child */

		cout << "new process started to handle new connection" << endl;
		// Read client's message
		while ( (length = read(msgsock, buf, MAXBUF)) > 0) {
		    cout << "buffer length = " << length << endl;
		    buf[length] = '\0';
		    cout << "Incoming command -> " << buf;

		    // reply to the client
		    if ( write(sock, message, sizeof(message)) < 0 ) {
			perror("Cannot write to stream socket");
		    }

		    
		}
		cout << "process ended" << endl;
		exit(0);
	    }
	}
    }
}

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

#include "remote_exec.h"

char *determine_port(char *host);


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
	exit(-1);
    }

    return sock;
}


main(int argc, char *argv[]) {
    int sock, len;
    fd_set ready;
    int port;
    char message[256];

    /* Check usage */
    if ( argc < 3 ) {
	printf("Usage: %s remote_machine port\n", argv[0]);
	exit(1);
    }

    port = atoi(argv[2]);
    sock = make_socket(argv[1], port);

    /* build a command string from the argv[]'s */
    strcpy(message, "hello world!\n");

    /* send command and arguments to remote server */
    if ( write(sock, message, sizeof(message)) < 0 ) {
	perror("Cannot write to stream socket");
    }

    for ( ;; ) {
	/* loop until remote program finishes */

        FD_ZERO(&ready);
        FD_SET(sock, &ready);

	/* block until input from sock or stdin */
	select(32, &ready, 0, 0, NULL);

	if ( FD_ISSET(sock, &ready) ) {
	    /* input coming from socket */
	    if ( (len = read(sock, message, 1024)) > 0 ) {
		write(1, message, len);
	    } else {
		exit(0);
	    }
	}
    }
    close(sock);
}


#include <simgear/compiler.h>

#include <stdio.h>
#include <malloc.h>
#include <sys/types.h>
#include <signal.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <setjmp.h>
#include <string.h>
#include <strings.h>

#include STL_IOSTREAM
SG_USING_STD(cout);
SG_USING_STD(endl);

#define ATC_SERVER_ADDRESS "192.168.2.15" // adddress of machine running festival server

#include "voice.hxx"

int			atc_sockfd = 0;
int			result, servlen;
struct sockaddr_in	atc_serv_addr, atc_cli_add;

//FGVoice *p_voice;

bool FGVoice::send_transcript( string trans, string refname, short repeat )
{
	string msg;

	switch ( repeat ) {
		case 0: msg = "S " + refname + ">" + trans;
			break;
		case 1: msg = "R " + refname + ">" + trans;
			break;
		case 2: msg = "X " + refname;
			break;
		}
	// strip out some characters	
	for(unsigned int i = 0; i < msg.length(); ++i) {
		if((msg.substr(i,1) == "_") || (msg.substr(i,1) == "/")) {
			msg[i] = ' ';
		}
	}
		
	int len = msg.length();	
	if (sendto(atc_sockfd, (char *) msg.c_str(), len, 0, (struct sockaddr *) &atc_serv_addr, servlen ) != len) {
			cout << "network write failed for " << len << " chars" << endl;
			return false;
		}
	printf("Transmit: %s of %d chars\n", msg.c_str(), len );
	return true;
}

FGVoice::FGVoice()
{
	string mesg = "Welcome to the FlightGear voice synthesizer";
	string welcome = "welcome ";
// Init the network stuff here
	printf( "FGVoice created. Connecting to atc sim\n");
  servlen = sizeof( atc_serv_addr );	
  bzero((char *) &atc_serv_addr, servlen);
  atc_serv_addr.sin_family = AF_INET;
  atc_serv_addr.sin_addr.s_addr = inet_addr(ATC_SERVER_ADDRESS);
  atc_serv_addr.sin_port = htons(7100);

  if ((atc_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
		printf("Failed to obtain a socket\n");
//		return( 0 );
		}
		else send_transcript( mesg, welcome, 0 );
}

FGVoice::~FGVoice()
{
	close( atc_sockfd );
}

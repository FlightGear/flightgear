/*************************************************************/
/* FGD.C by Oliver Delise                                    */
/* Contact info:                                             */
/* e-mail: delise@mail.isis.de                               */
/* www: http://www.isis.de/members/~odelise/progs/flightgear  */
/*                                                           */
/* Version 0.1-beta                                          */
/* The author of this program offers no waranty at all       */
/* about the correct execution of this software material.    */
/* Furthermore, the author can NOT be held responsible for   */
/* any physical or moral damage caused by the use of this    */
/* software.                                                 */
/*                                                           */
/* This is a standalone Tool to communicate with any         */
/* FlightGear System and FGFS-Deamon.                        */
/* This is Open Source Software with some parts              */
/* shamelessly stolen from others...                         */
/*                                                           */
/* -> This program will use a TCP port listening on a        */
/*    remote or local host inside the range you give to it.  */
/*    I offer no warranty over the accuracy though :)        */
/*    There are 3 verbose modes: No info, service info, and  */
/*    full info. No info is good of you only want the list   */
/*    of the ports, no more info. The best mode is Full      */
/*    info, as you get error information,etc. The main       */
/*    output is STDOUT, and ALL the errors go to STDERR.     */
/*                                                           */
/*    History: v0.1pre-alpha: May 25 1999 -> First release   */
/*             v0.1-alpha   : Nov 08 1999                    */
/*             v0.1-beta    : Jan 16 2000                    */
/*                            libc5, glibc-2.0, 2.1 cleanups */
/*                            June 8 2000    socket cleanup  */
/*************************************************************/


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/utsname.h>

//#define printf //

/* Net-stuff */
fd_set rset, allset;
int  maxfd, nready, retval;
struct timeval tv;
struct utsname myname;
char *fgd_host, *src_host;

/* Program-stuff */
int verbose, fgd_len_msg;

/* List-stuff (doubly-Linked-list) */
#include <string.h>
#include <stdlib.h>

int i, j, fgd_cnt, fgd_curpos;
char *vb, *fgd_txt;
const int True  = 0;
const int False= -1;

float sgFGD_COORD[4][4];

struct list_ele {
   unsigned char ipadr[16], callsign[16];
   unsigned char lat[8], lon[8], alt[8], speed[8], roll[8], pitch[8], yaw[8];
   float latf, lonf, altf, speedf, rollf, pitchf, yawf;
   float sgFGD_COORD[4][4];
   struct list_ele *next, *prev;
};

struct list_ele *head, *tail, *act, *test, *incoming;  /*  fgd_msg; */

struct fgd_sock_list {
   char adr[16];
   int  prev_sock;
};

struct fgd_sock_list fgd_cli_list[255];
int fgd_known_cli = -1;   /* False */
int fgd_cli = 0;

/*...Create head and tail of list */
void list_init( void) {
   incoming = (struct list_ele*) malloc(sizeof(struct list_ele));
   head = (struct list_ele*) malloc(sizeof(struct list_ele));
   tail = (struct list_ele*) malloc(sizeof(struct list_ele));
   if (head == NULL || tail == NULL) { printf("Out of memory\n"); exit(1); }
/* fixme :Where is the "with foobar do command "
   head->ipadr = "127.0.0.0";
   strcpy(head->callsign, "None");
   head->lon = 0;
   head->lat = 0;   
   head->alt = 0;   
   head->pitch = 0;
   head->roll = 0;
   head->yaw = 0;
*/   
   /* yaw!. Who the f$%& invented this ;-) */
   head->ipadr[0] = 0;
   tail->ipadr[0] = 255;
   tail->ipadr[1] = 0;
   head->prev = tail->prev = head;
   head->next = tail->next = tail;
   act = head;		/* put listpointer to beginning of list */
}

void list_output( void) {
}

void list_search( char name[16]) {

   if (strcmp(name, head->next->ipadr) <= 0)     act = head;
   else if (strcmp(name, tail->prev->ipadr) > 0) act = tail->prev;
   else {
      int vergleich = strcmp(name, act->ipadr);
      if (vergleich > 0)
         while (strcmp(name, act->next->ipadr) > 0) {
            act = act->next;
         }
      else if (vergleich < 0)
         while (strcmp(name, act->ipadr) < 0) {
            act = act->prev;
         }
      else
         while (strcmp(name, act->ipadr) == 0) {
            act = act->prev;
         }
   }
}

void list_insert( char newip[16]) {
struct list_ele *new_ele;

   new_ele = (struct list_ele*) malloc(sizeof(struct list_ele));
   if (new_ele == NULL) { printf("Out of memory\n"); exit(1); }
   strcpy(new_ele->ipadr, newip);
/* setting default */   
   strcpy(new_ele->callsign, "not assigned");
   list_search( newip);
   new_ele->prev = act;
   new_ele->next = act->next;
   act->next->prev = act->next = new_ele;
}

void list_setval( char newip[16]) {

   list_search( newip);
   strcpy( act->next->callsign, incoming->callsign);
   act->next->latf = incoming->latf;
   act->next->lonf = incoming->lonf;
   act->next->altf = incoming->altf;
   act->next->speedf = incoming->speedf;
   act->next->rollf = incoming->rollf;
   act->next->pitchf = incoming->pitchf;
   act->next->yawf = incoming->yawf;
   printf("Callsign %s ", act->next->callsign);
   printf(" lat: %7.3f lon: %7.3f alt: %7.3f speed: %7.3f roll: %7.3f pitch: %7.3f yaw: %7.3f",
            act->next->latf, act->next->lonf, act->next->altf, act->next->speedf,
            act->next->rollf, act->next->pitchf, act->next->yawf);
}

void list_setval_Mat4( char newip[16]) {

   list_search( newip);
   strcpy( act->next->callsign, incoming->callsign);
   for (i=0;i<4;i++)
     for (j=0;j<4;j++) 
       act->next->sgFGD_COORD[i][j] = incoming->sgFGD_COORD[i][j];
   printf("Callsign %s ", act->next->callsign);
}


void list_clear( char clrip[16]) {
struct list_ele *clr_ele;

   list_search( clrip);
   if ( strcmp( clrip, act->next->ipadr))
      printf("....Name %s nicht vorhanden", clrip);
   else {
     clr_ele         = act->next;
     act->next       = act->next->next;
     act->next->prev = act;
     free( clr_ele);
   }
}


int list_not_in( char name[16]) {
   
   i = True;
   test = head->next;
   while ((test != tail) && (i==True)) {
     i = (strcmp(test->ipadr, name) ? True : False);
     test = test->next;
     if (verbose != 0) printf("list_not_in : %d\n",i);
   }
   return(i);
}




int sock = -1;
int my_sock;
int fgd_com;
int *ip;
size_t anz;
char buff[1024];
char *fgd_name;
struct { char *adr, *lon, *lat, *alt;} fgd_client;
int fgd_ele_len;

struct sockaddr_in address;
struct sockaddr_in my_address;
int result;
socklen_t size = sizeof(address);


extern int errno;
int current_port = 0; 
u_short base_port = 0;
u_short end_port = 1024;
struct hostent *host_info;
struct servent *service_info;

void fgd_init(void);

int main(int argc, char **argv)
{ 
   if (argc < 4) {
    fprintf(stderr,"Usage: fgd [start port] [end port] [name] <-v or -vv>\n");
    exit(1);
   }
   
   switch (argc) {
     case 5: if (!strcmp(argv[4],"-v")) 
               verbose = 1;
             else if (!strcmp(argv[4],"-vv"))
                    verbose = 2;
               else { fprintf(stderr,"Usage: fgd [start port] [end port] [name] <-v or -vv>\n");
                      exit(1); }

     case 4: base_port = (u_short)atoi(argv[1]);
             end_port = (u_short)atoi(argv[2]);
             fgd_name = argv[3];
             break;
     default: fprintf(stderr,"Usage: fgd [start port] [end port] [name] <-v or -vv>\n");
              exit(1);
              break;
   }
   
   bzero((char *)&address, sizeof(address));
   address.sin_family = AF_INET;
   if (uname(&myname) == 0) fgd_host = myname.nodename;
   printf(" I am running as %s on HOST %s\n", fgd_name, fgd_host);

   if (verbose == 2) printf(" Resolving: %s ->",fgd_host);
   if (host_info = gethostbyname(fgd_host)) {
     bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
     printf(" fgd : ip = %s\n", inet_ntoa( address.sin_addr));
     
     if (verbose == 2) printf(" resolved\n");
   } else if ((address.sin_addr.s_addr = inet_addr(fgd_host)) == INADDR_NONE) {
            fprintf(stderr,"Could not get %s host entry !\n",argv[1]);
            printf(" NOT resolved !!!\n");
            exit(1);
          } else if (verbose == 2) printf(" address valid\n");
   
   if ((base_port > end_port) || ((short)base_port < 0)) { 
     fprintf(stderr,"Bad port range : start=%d end=%d !\n");
     exit(1);
   } else if (verbose == 2) {
            printf(" Port range: %d to %d\n",base_port,end_port);
     }
   printf(" verbose: %d\n",verbose);
   /* some init stuff */
   fgd_txt = (char *) malloc(1024);
   list_init();
   fgd_init();
   exit(0);
}

void fgd_init(void) {

struct { char *ip, *lon, *lat, *alt;} fg_id;


   current_port = base_port;
   while (current_port <= end_port) {
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
     {
	fprintf(stderr, "Error assigning master socket: %s\n",sys_errlist[errno]);
	exit(-1);
     } 

    address.sin_port = htons(current_port);
    if (verbose == 2) printf(" address.sin_port : %d\n",htons(address.sin_port));
    if (1 == 1) {
    if ( bind(sock, (struct sockaddr *)&address, sizeof(address)) == -1) {
         printf(" Aiiiieeehh...ADRESS ALSO IN USE...\7hmmm...please check another port\n");
         printf(" Just wait a few seconds or do a netstat to see the port status.\n");
         exit(-1);
    }
    listen(sock, 5);

/* Hier neu */
    maxfd = sock;
    FD_ZERO( &allset);
    FD_SET( sock, &allset);
for (;;){
    rset = allset;
    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    nready = select( sock + 1, &rset, NULL, NULL, &tv);
    if (FD_ISSET( sock, &rset)) {
      my_sock = accept( sock, (struct sockaddr *)&address, &size);
      anz = 2;
/* reading length of senders' ip */
      fgd_ele_len =  0;
      read( my_sock, buff, 1);
      fgd_ele_len =  buff[0];      
      if (verbose == 2) printf("ele_len %d\n", fgd_ele_len);  
/* reading senders' ip */
      read( my_sock, &buff, fgd_ele_len);
      ip = (int *) buff;
//      printf("%d %d %d %d   %x   %x  %x\n", buff[0], buff[1], buff[2], buff[3], &ip, ip, *ip);
      fgd_client.adr = (char*) inet_ntoa( *ip);
      src_host = fgd_client.adr;
/* reading commando */
      read( my_sock, &buff, 1);
/* closing previous file descriptor of same client, at least we know now 
   that the previous command is finished since it's the same client who 
   again requests something. Maybe I'm to quick and diiirty ;-) */
      printf("FGD : Using socket #%d\n", my_sock);
      fgd_known_cli = False;
      for ( fgd_cnt = 1; fgd_cnt < fgd_cli+1; fgd_cnt++) {
        printf("FGD : fgd_cnt:%d  fgd_cli:%d  fgd_client.adr: %s  prev.sock:%d  fgd_cli_list[fgd_cnt].adr: %s\n",
                      fgd_cnt, fgd_cli, fgd_client.adr, 
                      fgd_cli_list[fgd_cnt].prev_sock, fgd_cli_list[fgd_cnt].adr);
        if ( strcmp( fgd_cli_list[fgd_cnt].adr, fgd_client.adr) == 0) {
             printf("FGD : In Vergleichsloop. Closing socket: %d\n",
                      fgd_cli_list[fgd_cnt].prev_sock);
             close( fgd_cli_list[fgd_cnt].prev_sock);
             fgd_cli_list[fgd_cnt].prev_sock = my_sock;
             fgd_known_cli = True;
        }
      }
      if ( fgd_known_cli == False) {
           fgd_cli++;
           fgd_cli_list[fgd_cli].prev_sock = my_sock;
           strcpy(fgd_cli_list[fgd_cli].adr, fgd_client.adr);
      }
      printf(" Commando received : %s from Host : %s\n", &buff, src_host);
      fgd_com = ( (char) buff[0]) - 0x30;
      printf("%d \n", fgd_com);
      switch (fgd_com) {
      case 0:	printf(" fgd   : Identify\n");
                sprintf( (char*) buff, "FGDLH%s", fgd_name);
                buff[3] = strlen(fgd_name) + 1;     /* Lo, incl.zero      */
                buff[4] = 0;                        /* Hi, names < 0xff ! */
                buff[buff[3] + 4] = 0;              /* Term.zero */
                printf(" I am  : %s\n", fgd_name);
                write( my_sock, &buff, buff[3]+5);  /* fgd housekeeping ;-) */
     //           close(my_sock);
                break;      
      case 1:	printf(" fgd   : Register\n");
      /* reading length of FGFS_host ip */
                fgd_ele_len =  0;
                read( my_sock, &buff, 1);
                fgd_ele_len =  (int) &buff;
      /* reading FGFS_host ip */
                read( my_sock, &buff, fgd_ele_len);
                ip = (int *) buff;
                fgd_client.adr = (char*) inet_ntoa( *ip);
		if (list_not_in(fgd_client.adr) == True) {
		    list_insert(fgd_client.adr);
		    list_search(fgd_client.adr);
//                    strcpy(act->callsign, "None");
                    printf(" Setting default values\n");
                    printf("    IP : %s\n", act->next->ipadr);
                    printf(" PILOT : %s\n", act->next->callsign);                    
                    }
      /* writing answer back to client */
                sprintf( (char*) buff, "FGDLH%s", fgd_client.adr);
                buff[3] = strlen(fgd_client.adr) + 1;     /* Lo, incl.zero      */
                buff[4] = 0;                        /* Hi, names < 0xff ! */
                buff[buff[3] + 4] = 0;              /* Term.zero */
                write( my_sock, &buff, buff[3]+5);  /* fgd housekeeping ;-) */
 //               close(my_sock);
      		break;
      case 2:	printf(" fgd   : Show Registered\n");
                sprintf( (char*) buff, "FGD");
                // buff[3] = buff[4] = 0;
                fgd_cnt = 0;
                fgd_curpos = 6;
      		test = head->next;
      		while (test != tail) { 
      		   printf("    IP : %s\n", test->ipadr);
      		   fgd_cnt++;
      		   fgd_ele_len = strlen(test->ipadr) + 1;
//      		   printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
      		   buff[fgd_curpos] = fgd_ele_len;
      		   fgd_curpos++;
      		   bcopy(test->ipadr, &buff[fgd_curpos], fgd_ele_len);
      		   fgd_curpos += fgd_ele_len;
      		   //buff[fgd_curpos] = 0;
      		   test = test->next;
      		}
      		if (fgd_cnt == 0) fgd_curpos--;
      		buff[3] = fgd_curpos & 0xff;
      		buff[4] = fgd_curpos >> 8;
      		buff[5] = fgd_cnt;
      		write( my_sock, &buff, fgd_curpos);
//      		close(my_sock);
      		break;
      case 3:	printf(" fgd   : Send MSG\n");
//                close(my_sock);
      		break;
      case 4:	printf(" fgd   : Send MSG to all\n");
//      		close(my_sock);
      		break;      		
      case 5:	printf(" fgd   : Get DATA from client\n");
		read( my_sock, &buff, 1);
		fgd_ele_len = buff[0];
		read( my_sock, &buff, fgd_ele_len);
		ip = (int *) buff;
		fgd_client.adr = (char*) inet_ntoa( *ip);
		printf("    IP : %s\n", fgd_client.adr);
		if (verbose != 0) printf("not_in (CASE) : %d\n", list_not_in(fgd_client.adr));
		if (list_not_in(fgd_client.adr) == False) {
		    printf(" Checkpoint\n");
                    read( my_sock, &buff, 1);
                    printf(" Checkpoint 1\n");
		    fgd_ele_len = buff[0];
		    read( my_sock, &buff, fgd_ele_len);
		    incoming->callsign[fgd_ele_len] = 0;
                    bcopy( &buff, incoming->callsign, fgd_ele_len);
                /* lat, lon */
                    read( my_sock, &buff, 56);
                    sscanf( buff," %7f %7f %7f %7f %7f %7f %7f", &incoming->latf, &incoming->lonf,
                           &incoming->altf, &incoming->speedf, &incoming->rollf,
                           &incoming->pitchf, &incoming->yawf);
                    printf(" lat   :%7.3f\n lon   :%7.3f\n alt   :%7.3f\n speed :%7.3f\n roll  :%7.3f\n pitch :%7.3f\n yaw   :%7.3f\n",
                            incoming->latf, incoming->lonf, incoming->altf, incoming->speedf,
                            incoming->rollf, incoming->pitchf, incoming->yawf);
		    list_setval(fgd_client.adr);
                    }
                    else strcpy( fgd_client.adr, "UNKNOWN");
      /* writing answer back to client */
                sprintf( (char*) buff, "%sLH%s ", "FGD", fgd_client.adr);
                buff[3] = strlen(fgd_client.adr) + 1;
                buff[4] = 0;
                buff[ buff[3] + 4] = 0;
                printf("    IP : %s\n", fgd_client.adr);
                write( my_sock, &buff, buff[3]+5);
//      		close(my_sock);
      		break;
      case 17:	printf(" fgd   : Get Mat4 DATA from client\n");
		read( my_sock, &buff, 1);
		fgd_ele_len = buff[0];
		read( my_sock, &buff, fgd_ele_len);
		ip = (int *) buff;
		fgd_client.adr = (char*) inet_ntoa( *ip);
		printf("    IP : %s\n", fgd_client.adr);
		if (verbose != 0) printf("not_in (CASE) : %d\n", list_not_in(fgd_client.adr));
		if (list_not_in(fgd_client.adr) == False) {
		    printf(" Checkpoint\n");
                    read( my_sock, &buff, 1);
                    printf(" Checkpoint 1\n");
		    fgd_ele_len = buff[0];
		    read( my_sock, &buff, fgd_ele_len);
		    incoming->callsign[fgd_ele_len] = 0;
                    bcopy( &buff, incoming->callsign, fgd_ele_len);
                /* lat, lon */
                    read( my_sock, &buff, 158);
                    i = sscanf( buff," %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                            &incoming->sgFGD_COORD[0][0], &incoming->sgFGD_COORD[0][1], &incoming->sgFGD_COORD[0][2], &incoming->sgFGD_COORD[0][3],
                            &incoming->sgFGD_COORD[1][0], &incoming->sgFGD_COORD[1][1], &incoming->sgFGD_COORD[1][2], &incoming->sgFGD_COORD[1][3],
                            &incoming->sgFGD_COORD[2][0], &incoming->sgFGD_COORD[2][1], &incoming->sgFGD_COORD[2][2], &incoming->sgFGD_COORD[2][3],
                            &incoming->sgFGD_COORD[3][0], &incoming->sgFGD_COORD[3][1], &incoming->sgFGD_COORD[3][2], &incoming->sgFGD_COORD[3][3]);

//                    printf(" sscanf input: %d\n",i);
                    printf(" sgMat4: \n%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n",
                                           incoming->sgFGD_COORD[0][0], incoming->sgFGD_COORD[0][1], incoming->sgFGD_COORD[0][2], incoming->sgFGD_COORD[0][3],
                                           incoming->sgFGD_COORD[1][0], incoming->sgFGD_COORD[1][1], incoming->sgFGD_COORD[1][2], incoming->sgFGD_COORD[1][3],
                                           incoming->sgFGD_COORD[2][0], incoming->sgFGD_COORD[2][1], incoming->sgFGD_COORD[2][2], incoming->sgFGD_COORD[2][3],
                                           incoming->sgFGD_COORD[3][0], incoming->sgFGD_COORD[3][1], incoming->sgFGD_COORD[3][2], incoming->sgFGD_COORD[3][3]);
		    list_setval_Mat4(fgd_client.adr);
                    }
                    else strcpy( fgd_client.adr, "UNKNOWN");
      /* writing answer back to client */
                sprintf( (char*) buff, "FGDLH%s", fgd_client.adr);
                buff[3] = strlen(fgd_client.adr) + 1;
                buff[4] = buff[buff[3]+5] = 0;
                printf("    IP : %s\n", fgd_client.adr);
                write( my_sock, &buff, buff[3]+5);
//      		close(my_sock);
      		break;
      case 6:	printf(" fgd   : Send all DATA to client\n");
                sprintf( (char*) buff, "%s %s", "FGD", fgd_client.adr);
                buff[3] = 0;
                fgd_cnt = 0;
                fgd_curpos = 6;
      		test = head->next;
      		while (test != tail) { 
      		   printf("    IP : %-16s  Callsign : %-16s\n", test->ipadr, test->callsign);
      		   fgd_cnt++;
     		/* IP */
      		   fgd_ele_len = strlen(test->ipadr);
                   printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
      		   buff[fgd_curpos] = fgd_ele_len;
      		   fgd_curpos++;
      		   bcopy(test->ipadr, &buff[fgd_curpos], fgd_ele_len);
      		   fgd_curpos = fgd_curpos + fgd_ele_len;
      		/* Callsign */
      		   fgd_ele_len = strlen(test->callsign);
                   printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
      		   buff[fgd_curpos] = fgd_ele_len;
      		   fgd_curpos++;
      		   bcopy(test->callsign, &buff[fgd_curpos], fgd_ele_len);
      		   fgd_curpos = fgd_curpos + fgd_ele_len;
                /* Lat, Lon, Alt, Speed, Roll, Pitch, Yaw
                   hope this sprintf call is not too expensive */
                   sprintf( fgd_txt, " %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f",
                            test->latf, test->lonf, test->altf, test->speedf,
                            test->rollf, test->pitchf, test->yawf);
                   printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
                   printf(" Data : %s\n", fgd_txt);
                   bcopy((char *) fgd_txt, &buff[fgd_curpos], 56);
                   fgd_curpos += 56;
     		   test = test->next;
      		}
      		if (fgd_cnt == 0) fgd_curpos --;
      		printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
      		buff[3] = fgd_curpos;
      		buff[4] = fgd_curpos / 256;
      		buff[5] = fgd_cnt;
      		write( my_sock, &buff, fgd_curpos);
//      		close(my_sock);
      		break;
      case 18:	printf(" fgd   : Send all Mat4 DATA to client\n");
                sprintf( (char*) buff, "FGDLH");
                buff[3] = buff[4] = 0;
                fgd_cnt = 0;
                fgd_curpos = 6;
      		test = head->next;
      		while (test != tail) { 
      		   printf("    IP : %-16s  Callsign : %-16s\n", test->ipadr, test->callsign);
      		   fgd_cnt++;
     		/* IP */
      		   fgd_ele_len = strlen(test->ipadr);
                   printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
      		   buff[fgd_curpos] = fgd_ele_len;
      		   fgd_curpos++;
      		   bcopy(test->ipadr, &buff[fgd_curpos], fgd_ele_len);
      		   fgd_curpos = fgd_curpos + fgd_ele_len;
      		/* Callsign */
      		   fgd_ele_len = strlen(test->callsign);
                   printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
      		   buff[fgd_curpos] = fgd_ele_len;
      		   fgd_curpos++;
      		   bcopy(test->callsign, &buff[fgd_curpos], fgd_ele_len);
      		   fgd_curpos = fgd_curpos + fgd_ele_len;
                /* Lat, Lon, Alt, Speed, Roll, Pitch, Yaw
                   hope this sprintf call is not too expensive */
                   fgd_len_msg = sprintf( fgd_txt, " %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                          test->sgFGD_COORD[0][0], test->sgFGD_COORD[0][1], test->sgFGD_COORD[0][2], test->sgFGD_COORD[0][3],
                          test->sgFGD_COORD[1][0], test->sgFGD_COORD[1][1], test->sgFGD_COORD[1][2], test->sgFGD_COORD[1][3],
                          test->sgFGD_COORD[2][0], test->sgFGD_COORD[2][1], test->sgFGD_COORD[2][2], test->sgFGD_COORD[2][3],
                          test->sgFGD_COORD[3][0], test->sgFGD_COORD[3][1], test->sgFGD_COORD[3][2], test->sgFGD_COORD[3][3]);
                 fgd_txt[fgd_len_msg] = 0;
                   printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
                   printf(" Data : %s\n", fgd_txt);
                   bcopy((char *) fgd_txt, &buff[fgd_curpos], fgd_len_msg+1);
                   fgd_curpos += fgd_len_msg+1;
     		   test = test->next;
      		}
      		if (fgd_cnt == 0) fgd_curpos -= 1;
      		printf(" ele_len %d  curpos %d\n", fgd_ele_len, fgd_curpos);
      		buff[3] = fgd_curpos & 0xff;
      		buff[4] = fgd_curpos / 256;
      		buff[5] = fgd_cnt;
      		printf("ANZ: %d  CURPOS: %d\n", (unsigned char) buff[3] + (unsigned char) buff[4] * 256, fgd_curpos);
      		write( my_sock, &buff, fgd_curpos);
//      		close(my_sock);
      		break;
      case 8:	printf(" fgd   : Unregister\n");
		read( my_sock, &buff, 1);
		fgd_ele_len = (int) &buff;
		read( my_sock, &buff, fgd_ele_len);
		ip = (int *) buff;
		fgd_client.adr = (char*) inet_ntoa( *ip);
		printf("    IP : %s\n", fgd_client.adr);
		if (verbose != 0) printf("not_in (CASE) : %d\n", list_not_in(fgd_client.adr));
		if (list_not_in(fgd_client.adr) == -1) {
		    list_clear(fgd_client.adr);
                    }
                    else strcpy( fgd_client.adr, "UNKNOWN");
      /* writing answer back to client */
                sprintf( (char*) buff, "FGDLH%s", fgd_client.adr);
                buff[3] = strlen(fgd_client.adr) + 1;
                buff[4] = buff[buff[3]+5] = 0;
                printf("    IP : %s\n", fgd_client.adr);
                write( my_sock, &buff, buff[3]+5);                    
//  Just leaving the old stuff in, to correct it for FGFS later...
//  I'm sick of this f$%&ing libc5/glibc2.0/2.1 quirks
//  Oliver...very angry...
//      /* writing answer back to client */
//                sprintf( (char*) buff, "%s %s", "FGD", fgd_client.adr);
//                buff[3] = strlen(fgd_client.adr);
//                printf("    IP : %s\n", fgd_client.adr);
//                write( my_sock, &buff, buff[3]+4);
//                close(my_sock);
      		break;  
      case 9:	printf(" fgd   : Shutdown\n");
                close(my_sock);
                close(sock);
                exit(0);      		    		
      default:	printf(" fgd   : Huh?...Unknown Command\n");
                break;
      }
    }
    }
/*
     switch (verbose) {
      case 0: printf("%d\n",base_port+current_port);
              break;
      case 1: service_info = getservbyport(htons(base_port+current_port),"tcp");
              if (!service_info) {
              printf("%d -> service name unknown\n",base_port+current_port);
              } else {
              printf("%d -> %s\n",base_port+current_port,service_info->s_name);
              }
              break; 
      case 2: service_info = getservbyport(htons(base_port+current_port),"tcp");
              if (!service_info) {
              printf("Port %d found. Service name unknown\n",base_port+current_port);
              } else {
              printf("Port %d found. Service name: %s\n",base_port+current_port,service_info->s_name);
              }
              break; 
     } 
*/
    }  else if (errno == 113) {
         fprintf(stderr,"No route to host !\n");
         exit(1);
       } 
/*     current_port++; */
   }

  if (verbose == 2) printf("Port scan finished !\n");
}

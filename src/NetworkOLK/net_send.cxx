/*************************************************************/
/* NET_SEND.CXX by Oliver Delise                             */
/* Contact info:                                             */
/* e-mail: delise@mail.isis.de                               */
/* www: http://www.isis.de/members/odelise/progs/flightgear  */
/*                                                           */
/* Version 0.1-beta                                          */
/* The author of this program offers no waranty at all       */
/* about the correct execution of this software material.    */
/* Furthermore, the author can NOT be held responsible for   */
/* any physical or moral damage caused by the use of this    */
/* software.                                                 */
/*                                                           */
/* This is an interactive standalone Tool to communicate     */ 
/* with any FlightGear-Deamon.                               */
/* This is Open Source Software with many parts              */
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
/*             v0.1-alpha     Nov 11 1999                    */
/*             v0.1-beta      Jan 16 2000 -> libc5, glibc2.0 */
/*                            glibc-2.1 issues fixed         */
/*************************************************************/

#include <stdio.h>
#include <strings.h>
#include "fgd.h"

/* I prefer NHV's decl. */
#include <simgear/constants.h>

#include <Cockpit/hud.hxx>
#include <plib/ssg.h>
#include <Main/views.hxx>

//#define printf //

/* Netstuff */
#include <arpa/inet.h>
int sock = -1;
int my_sock;
struct sockaddr_in address;
struct sockaddr_in my_address;
int result;

#if defined( __CYGWIN__ )
#include <errno.h>
#else
extern int errno;
#endif

extern const char *const sys_errlist[];

int current_port  = 10000; 
u_short base_port = 10000;
u_short end_port  = 10010;
int verbose = 0;
struct hostent *host_info, *f_host_info;
struct servent *service_info;
struct utsname myname;

/* Program-stuff */
int i, j;
int fgd_len_msg = 1, fgd_reply_len, fgd_status, fgd_ele_len, fgd_curpos, fgd_cnt, fgd_ppl,
    fgd_ppl_old, fgd_loss, net_r;
size_t anz;
char *fgd_job, *fgd_callsign, *fgd_name, *fgd_ip, *fgd_mcp_ip;
char *buffp, *src_host, *fgd_host, *fgfs_host, *fgfs_pilot, *fgd_txt;
sgMat4 sgFGD_COORD;
extern sgMat4 sgFGD_VIEW;
extern ssgRoot *fgd_scene;
extern char *FGFS_host, *net_callsign;

/* List-stuff */

const int True  = 0;
const int False= -1;

struct list_ele {
   /* unsigned */ char ipadr[16], callsign[16];
   /* unsigned */ char lon[8], lat[8], alt[8], roll[8], pitch[8], yaw[8];
   float lonf, latf, altf, speedf, rollf, pitchf, yawf;
   sgMat4 sgFGD_COORD;
   ssgSelector *fgd_sel;
   ssgTransform *fgd_pos;
   struct list_ele *next, *prev;
};

struct list_ele *head, *tail, *act, *test, *incoming, *boss, *other;  /* fgd_msg; */

/*...Create head and tail of list */
void list_init( void) {

   incoming = (struct list_ele*) malloc(sizeof(struct list_ele));
   boss = (struct list_ele*) malloc(sizeof(struct list_ele));
   other = (struct list_ele*) malloc(sizeof(struct list_ele));
   head = (struct list_ele*) malloc(sizeof(struct list_ele));
   tail = (struct list_ele*) malloc(sizeof(struct list_ele));
   if (head == NULL || tail == NULL) { printf("Out of memory\n"); exit(1); }
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
/* generating ssg stuff */
   new_ele->fgd_sel = new ssgSelector;
   new_ele->fgd_pos = new ssgTransform;

   ssgEntity *fgd_obj = ssgLoadAC( "tuxcopter.ac" );
   fgd_obj->clrTraversalMaskBits( SSGTRAV_HOT );
   new_ele->fgd_pos->addKid( fgd_obj );
   new_ele->fgd_sel->addKid( new_ele->fgd_pos );
   ssgFlatten( fgd_obj );
   ssgStripify( new_ele->fgd_sel );

   fgd_scene->addKid( new_ele->fgd_sel );
   fgd_scene->addKid( fgd_obj );
/* ssgKid "born" and inserted into scene */
   list_search( newip);
   new_ele->prev = act;
   new_ele->next = act->next;
   act->next->prev = act->next = new_ele;
}

void list_setval( char newip[16]) {

   list_search( newip);
   strcpy( act->next->callsign, incoming->callsign);
   printf("Callsign %s\n", act->next->callsign);
   
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
   return i;
}

void fgd_print_Mat4( sgMat4 m ) {
    printf("0.0 %f 0.1 %f 0.2 %f 0.3 %f\n", 
    m[0][0], m[0][1], m[0][2], m[0][3] );
    printf("1.0 %f 1.1 %f 1.2 %f 1.3 %f\n", 
    m[1][0], m[1][1], m[1][2], m[1][3] );
    printf("2.0 %f 2.1 %f 2.2 %f 2.3 %f\n", 
    m[2][0], m[2][1], m[2][2], m[2][3] );
    printf("3.0 %f 3.1 %f 3.2 %f 3.3 %f\n", 
    m[3][0], m[3][1], m[3][2], m[3][3] );
}

void fgd_init(void){

/* Let's init a few things */
   printf("MCP: Allocating memory...");
   buffp = (char *) malloc(1024); /* No I don't check if there are another KB */
   fgd_job = (char *) malloc(8);     
   fgd_host = (char *) malloc(64);
   fgd_callsign = (char *) malloc(64);
   fgd_name = (char*) malloc(64);
   fgd_ip = (char *) malloc(16);
   fgd_mcp_ip = (char *) malloc(16);
   fgfs_host = (char *) malloc(64);
   fgfs_pilot = (char *) malloc(64);
   src_host = (char *) malloc(64);
   fgd_txt = (char *) malloc(1024);
   printf("ok\nMCP: Initializing values...");   
   strcpy( fgd_job, "xxx");
   strcpy( fgd_host, "Olk");
   strcpy( fgd_callsign, "Unknown");
   strcpy( fgd_name, "Unknown");
   strcpy( fgd_ip, (char *) inet_ntoa(address.sin_addr));
   strcpy( fgd_txt, "");
   printf("ok\n");
   boss->latf = 112.3;
   boss->lonf = 4.5;
   boss->altf = 0.67;
   boss->speedf = 100.95;
   boss->rollf = 89.0;
   boss->pitchf = 1.23;
   boss->yawf = 456.789;
   fgd_ppl = 0;
   bzero((char *)&address, sizeof(address));
   address.sin_family = AF_INET;
/* determinating the source/sending host */
   if (uname(&myname) == 0) strcpy(src_host , myname.nodename);   
   printf("MCP: I'm running on HOST : %s  ", src_host);
   if (host_info = gethostbyname( src_host)) {
     bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
     strcpy((char *) fgd_mcp_ip, (char *) inet_ntoa(address.sin_addr));
     }
   printf("IP : %s\n", fgd_mcp_ip);
   FGFS_host = src_host;
/* resolving the destination host, here fgd's host */   
   if (verbose == 2) printf("     Resolving default DEAMON: %s ->", fgd_host);
   if (host_info = gethostbyname( fgd_host)) {
     bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
     strcpy((char *) fgd_ip, (char *) inet_ntoa(address.sin_addr));
     if (verbose == 2) {
            printf(" resolved\n     FGD running on HOST : %s", fgd_host);
            printf("   IP : %s\n", fgd_ip);
            }
   } else if ((address.sin_addr.s_addr = inet_addr( fgd_host)) == INADDR_NONE) {
            fprintf(stderr,"   Could not get %s host entry !\n", fgd_host);
            printf(" NOT resolved !!!\n");
            // exit(1);
          } else if (verbose == 2) printf(" address valid\n");
   
   if ((base_port > end_port) || ((short)base_port < 0)) { 
     fprintf(stderr,"Bad port range : start=%d end=%d !\n",base_port,end_port);
   // exit(1);
   } else if (verbose == 2) {
            printf("     Port range: %d to %d\n",base_port,end_port);
            }
}

int net_resolv_fgd( char *fgd_host_check ) {

char *fgd_ip_check;

/* resolving the destination host, here fgd's host */   
   net_r = 0;
   if (verbose == 2) printf("     Resolving default DEAMON: %s ->", fgd_host_check);
   if (host_info = gethostbyname( fgd_host_check)) {
     bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
     strcpy((char *) fgd_ip_check, (char *) inet_ntoa(address.sin_addr));
     fgd_ip = fgd_ip_check;
     if (verbose == 0) {
            printf(" FGD: resolved\nFGD: running on HOST : %s", fgd_host_check);
            printf("   IP : %s\n", fgd_ip_check);
            strcpy( fgd_host, fgd_host_check);
//            return 0;
            }
   } else if ((address.sin_addr.s_addr = inet_addr( fgd_host)) == INADDR_NONE) {
            fprintf(stderr,"FGD:  Could not get %s host entry !\n", fgd_host_check);
            printf(" FGD: NOT resolved !!!\n");
            net_r = -1;
            return -1;
            // exit(1);
          } else if (verbose == 2) printf(" address valid\n");
   
   if ((base_port > end_port) || ((short)base_port < 0)) { 
     fprintf(stderr,"Bad port range : start=%d end=%d !\n",base_port,end_port);
     // exit(1);
        net_r = -2;
        return -2;
   } else if (verbose == 2) {
            printf("     Port range: %d to %d\n",base_port,end_port);
            }
   return 0;
}


void fgd_send_com( char *FGD_com, char *FGFS_host) {

   strcpy( buffp, "                ");
//   current_port = base_port;
   if (verbose == 2) printf("     Sending : %s\n", FGD_com);
//   while (current_port <= end_port) {
/*  fprintf(stderr,"Trying port: %d\n",current_port); */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
     {
	fprintf(stderr, "Error assigning master socket: %s\n",sys_errlist[errno]);
        /* must check how severe this really is */
	// exit(-1);
     } 

    address.sin_port = htons(current_port);
    if (verbose == 2) printf("     address.sin_port : %d\n",htons(address.sin_port));

    f_host_info = gethostbyname(src_host);

//printf ("src_host : %s", ntohs(f_host_info->h_addr));
    
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) == 0) {
/* FIXME:    make a single string instead of sending elements */

        fgd_len_msg = (int) sizeof(f_host_info->h_addr);
/* send length of sender-ip */
        write( sock, &fgd_len_msg,1);
/* send sender-ip */
        write( sock, f_host_info->h_addr, fgd_len_msg);
/* send commando */     
        write( sock, FGD_com, 1);
/* send length of dummy-string, for the moment with _WHO_ to execute commando 
   here: his length of ip  */
        f_host_info = gethostbyname(FGFS_host);
        fgd_len_msg = (int) sizeof(f_host_info->h_addr);
        write( sock, &fgd_len_msg,1);
/* send dummy-string, for the moment with _WHO_ to execute commando 
   here: his ip  */   
        write( sock, f_host_info->h_addr, fgd_len_msg);
/* END FIXME  */

/* Here we send subsequent data... */
        switch  ( (char) FGD_com[0] - 0x30) {
        case 5:  fgd_len_msg = strlen( net_callsign);
                 write( sock, &fgd_len_msg,1);
        /* send string, for the moment, here: callsign */   
                 write( sock, net_callsign, fgd_len_msg);
        /* Lon, Lat, Alt, Speed, Roll, Pitch, Yaw
           hope this sprintf call is not too expensive */
                 sprintf( fgd_txt, " %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f", 
//                          boss->latf, boss->lonf, boss->altf, boss->speedf,
//                          boss->rollf, boss->pitchf, boss->yawf);
/* 
   Must decide if it's better to send values "as are" or convert them for
   deamon, better is to let deamon make the job. Anyway this will depend on 
   speed loss/gain in network-area...
*/
                          get_latitude(), get_longitude(), get_altitude(),
                          get_speed(), get_roll()*RAD_TO_DEG,
                          get_pitch()*RAD_TO_DEG, get_heading());
                 write( sock, fgd_txt, 56);
                 break;

/* Here sending the previously calculated view.Mat4 by FGFS */
        case 17: if (verbose == 2) printf("Checkpoint\n");
                 sgCopyMat4(sgFGD_COORD, current_view.VIEW);

                 if (verbose == 2) {
                    printf("current_view\n");
                    fgd_print_Mat4( current_view.VIEW);
                    printf("FGD_COORD\n");
                    fgd_print_Mat4( sgFGD_COORD);
                 }
                 fgd_len_msg = strlen( net_callsign);
                 write( sock, &fgd_len_msg,1);
        /* send string, for the moment, here: callsign */   
                 write( sock, net_callsign, fgd_len_msg);
        /* MATRIX-variant of Lon, Lat etc...
           hope this sprintf call is not too expensive */
                 fgd_len_msg = sprintf( fgd_txt, " %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                          sgFGD_COORD[0][0], sgFGD_COORD[0][1], sgFGD_COORD[0][2], sgFGD_COORD[0][3],
                          sgFGD_COORD[1][0], sgFGD_COORD[1][1], sgFGD_COORD[1][2], sgFGD_COORD[1][3],
                          sgFGD_COORD[2][0], sgFGD_COORD[2][1], sgFGD_COORD[2][2], sgFGD_COORD[2][3],
                          sgFGD_COORD[3][0], sgFGD_COORD[3][1], sgFGD_COORD[3][2], sgFGD_COORD[3][3]);
                 fgd_txt[fgd_len_msg] = 0;
                 write( sock, fgd_txt, fgd_len_msg+1);
                 break;
        default: break;
        }
        

/* be verbose, this goes later into own (*void) */ 
        if (verbose == 2) printf("     Message : %s\n", FGD_com);
        switch (verbose) {
        case 0: // printf("%d\n",current_port);
              break;
        case 1: service_info = getservbyport(htons(current_port),"tcp");
              if (!service_info) {
              printf("%d -> service name unknown\n",current_port);
              } else {
              printf("%d -> %s\n",current_port,service_info->s_name);
              }
              break; 
        case 2: service_info = getservbyport(htons(current_port),"tcp");
              if (!service_info) {
              printf("     Port %d found. Service name unknown\n",current_port);
              } else {
              printf("     Port %d found. Service name: %s\n",current_port,service_info->s_name);
              }
              break; 
        } 
    }  else if (errno == 113) {
         fprintf(stderr,"No route to host !\n");
         /* must check this */
         // exit(1);
       } 
/*     fprintf(stderr,"Error %d connecting socket %d to port %d: %s\n",
                errno,sock,current_port,sys_errlist[errno]); */ 

//              service_info = getservbyport(htons(current_port),"tcp");
//              if (!service_info) {



/* The Receiving Part, fgd returns errormessages, succes, etc... */
                  do { 
                     fgd_status = recv( sock, (char *) buffp, 5, MSG_WAITALL);
                     if (verbose == 2) printf("     status %d\n", fgd_status);
                     }
//                  while ( (fgd_status != 5) && (fgd_status != 0) );
                  while ( (fgd_status == -1) || (fgd_status == -1) );
                  fgd_reply_len = (unsigned char) buffp[3] + (unsigned char) buffp[4] * 256;
                  if (verbose == 2) {
                      printf("     Got reply : %x %x %x  MSG length %d Bytes\n", 
                                   buffp[0], buffp[1], buffp[2], fgd_reply_len);
                  }
                      if (strncmp( buffp, "FGD", 3) == 0) {
                      switch ( (char) FGD_com[0] - 0x30) {
                      case  0: int abc;
                               abc = read( sock, fgd_name, fgd_reply_len);
                               if (verbose == 2) printf("readwert: %d", abc);
                               // fgd_name[buffp[3]] = 0;                      
                               printf("FGD: FlightGear-Deamon %s detected\n", fgd_name);
                               break;
                      case  1: read( sock, fgd_txt, fgd_reply_len);
                               printf("FGD: Registering Host %s\n", fgd_txt);
                               break;
                      case  2: printf("FGD: Showing registered Hosts at %s\n", fgd_host);
                               if ( fgd_reply_len != 5) {
/* FIXME: replace with SELECT to avoid broken pipes, known bug (-; */
                                  do { 
                                      fgd_status = recv( sock, fgd_txt, fgd_reply_len - 5, 
                                                      MSG_WAITALL);
//                                    printf("     status %d\n", fgd_status);
                                  }
//                                while ( (fgd_status != 5) && (fgd_status != 0) );
                                  while ( (fgd_status == -1) || (fgd_status == -1) );                               
//                                read( sock, fgd_txt, fgd_reply_len - 5);
                                  fgd_curpos = 1;
                                  for (fgd_cnt = 1; fgd_cnt < (fgd_txt[0]+1); fgd_cnt++) {
                                      fgd_ele_len = fgd_txt[fgd_curpos];
                                      bcopy( &fgd_txt[fgd_curpos], fgfs_host, fgd_ele_len);
                                      // fgfs_host[fgd_ele_len] = 0;
                                      fgd_curpos += fgd_ele_len + 1;
                                      if (verbose == 2) printf("     #%d  %s\n", fgd_cnt, fgfs_host);
                                  }
                               }
                               break;
                      case  5: printf("FGD: Receiving data from Host %s\n", FGFS_host);
                               read( sock, fgd_txt, buffp[3]);
                               fgd_txt[buffp[3]] = 0;
/* This works...
                               if (strcmp(fgd_txt, "UNKNOWN") == 0) {
                                   printf("FGD: Host not in list, sorry...\n");
                               }
                               else printf("FGD: Data from Host %s received\n", fgd_txt);
*/
/* This has problem with glibc-2.1
                               if (strcmp(fgd_txt, "UNKNOWN") == -1) {
                                   if (verbose == 2) printf("FGD: Data from Host %s received\n", fgd_txt);
                                   }
                                   else if (verbose == 2) printf("FGD: Host not in list, sorry...\n");
*/
                               break;
                      case 17: if (verbose == 2) printf("FGD: Receiving Mat4 data from Host %s\n", FGFS_host);
                               read( sock, fgd_txt, fgd_reply_len);
                               // fgd_txt[buffp[3]] = 0;
                               if (strcmp(fgd_txt, "UNKNOWN") == -1) {
                                   if (verbose == 2) printf("FGD: Mat4 Data from Host %s received\n", fgd_txt);
                                   }
                                   else printf("FGD: Host not in list, sorry...\n");
                               break;                               
                      case  6: printf("FGD: Sending data to Host %s\n", FGFS_host);
                               if (buffp[3] != 4) {
/* FIXME: replace with SELECT */
                  if (verbose == 2) printf("Noch %d bytes\n", (unsigned char) buffp[3]);
                  do { 
                    fgd_status = recv( sock, fgd_txt, (unsigned char) buffp[3]-4, MSG_PEEK);
                    if (verbose == 2) printf("Status %d\n", fgd_status);
                     }
                    while ( (fgd_status == 4) || (fgd_status == -1) );
//                  while ( (fgd_status == -1) || (fgd_status == -1) );                               
                                 read( sock, fgd_txt, buffp[3]-4);
                                 fgd_curpos = 2;
                                 fgd_ppl_old = fgd_ppl;
                                 fgd_ppl = fgd_txt[0];
                                 /* Check if list has changed (pilot joined/left) */
                                 if (fgd_ppl != fgd_ppl_old) {
                                   printf(" List changed!!!\n");
                                   for (fgd_cnt = 1; fgd_cnt <= abs(fgd_ppl - fgd_ppl_old); fgd_cnt++) {
                                     if (verbose == 2) printf(" Checkpoint\n");
                                     incoming = head->next;
                                     if ((fgd_ppl - fgd_ppl_old) > 0) list_insert("test\0");
                                     else {
                                        printf(" Clearing entry.\n");
                                        list_clear(incoming->ipadr);
                                     }
                                   }
                                 }
//                                 else {
                                   incoming = head->next;
                                   for (fgd_cnt = 1; fgd_cnt < (fgd_ppl+1); fgd_cnt++) {
                                 /* IP */
                                   fgd_ele_len = fgd_txt[fgd_curpos-1];
                                   bcopy( &fgd_txt[fgd_curpos], incoming->ipadr, fgd_ele_len);
                                   incoming->ipadr[fgd_ele_len] = 0;
                                   fgd_curpos = fgd_curpos + fgd_ele_len + 1;
                                 /* Pilot */
                                   fgd_ele_len = fgd_txt[fgd_curpos-1];
                                   bcopy( &fgd_txt[fgd_curpos], incoming->callsign, fgd_ele_len);
                                   incoming->callsign[fgd_ele_len] = 0;
                                   fgd_curpos = fgd_curpos + fgd_ele_len + 1;
                                  /* Lon, Lat...etc */
                                   if (verbose == 2) {
                                   printf("     #%d  %-16s %s\n", fgd_cnt, incoming->ipadr, incoming->callsign);
                                   printf(" curpos:%d\n", fgd_curpos);
                                   sscanf( &fgd_txt[fgd_curpos]," %7f %7f %7f %7f %7f %7f %7f",
                                           &incoming->latf, &incoming->lonf,
                                           &incoming->altf, &incoming->speedf, &incoming->rollf,
                                           &incoming->pitchf, &incoming->yawf);
                                   printf(" lat   :%7.3f\n lon   :%7.3f\n alt   :%7.3f\n speed :%7.3f\n roll  :%7.3f\n pitch :%7.3f\n yaw   :%7.3f\n",
                                            incoming->latf, incoming->lonf, incoming->altf, incoming->speedf,
                                            incoming->rollf, incoming->pitchf, incoming->yawf);                                   
                                   }         
                                   fgd_curpos += 56;
                                   incoming = incoming->next;
                                   } /* end for                 */
//                                 }   /* end else                */
                               }     /* end if "data available" */
/* Here reading the answer of completed command by fgd */
/*                               read( sock, fgd_txt, buffp[3]);
                               fgd_txt[buffp[3]] = 0;
                               if (strcmp(fgd_txt, "UNKNOWN") == -1) {
                                   if (verbose == 2) printf("FGD: Data to Host sent\n");
                                   }
                                   else printf("FGD: Host not in list, sorry...\n");
*/                                   
                               break;
                      case 18: if (verbose == 2) printf("FGD: Sending Mat4 data to Host %s\n", FGFS_host);
                               if (fgd_reply_len != 5) {
/* FIXME: replace with SELECT */
                                 if (verbose == 2) printf("Noch %d bytes\n", fgd_reply_len);
                                 do { 
                                     fgd_status = recv( sock, fgd_txt, fgd_reply_len - 5, MSG_WAITALL);
                                     if (verbose == 2) printf("Status %d\n", fgd_status);
                                 }
//                                 while ( (fgd_status == 4) || (fgd_status == -1) );
                                 while ( (fgd_status == -1) || (fgd_status == -1) );
//                                 read( sock, fgd_txt, fgd_reply_len - 5);
                                 fgd_curpos = 2;
                                 fgd_ppl_old = fgd_ppl;
                                 fgd_ppl = fgd_txt[0];
                                 /* Check if list has changed (pilot joined/left) */
                                 if (fgd_ppl != fgd_ppl_old) {
                                   printf(" List changed!!!\n");
                                   for (fgd_cnt = 1; fgd_cnt <= abs(fgd_ppl - fgd_ppl_old); fgd_cnt++) {
                                     if (verbose == 2) printf(" Checkpoint\n");
                                     incoming = head->next;
                                     if ((fgd_ppl - fgd_ppl_old) > 0) list_insert("test\0");
                                     else {
                                        printf(" Clearing entry.\n");
                                        list_clear(incoming->ipadr);
                                     }
                                   }
                                 }
//                                 else {
                                   incoming = head->next;
                                   for (fgd_cnt = 1; fgd_cnt < (fgd_ppl+1); fgd_cnt++) {
                                 /* IP */
                                   fgd_ele_len = fgd_txt[fgd_curpos-1];
                                   bcopy( &fgd_txt[fgd_curpos], incoming->ipadr, fgd_ele_len);
                                   incoming->ipadr[fgd_ele_len] = 0;
                                   fgd_curpos = fgd_curpos + fgd_ele_len + 1;
                                 /* Pilot */
                                   fgd_ele_len = fgd_txt[fgd_curpos-1];
                                   bcopy( &fgd_txt[fgd_curpos], incoming->callsign, fgd_ele_len);
                                   incoming->callsign[fgd_ele_len] = 0;
                                   fgd_curpos = fgd_curpos + fgd_ele_len + 1;
                                  /* Lon, Lat...etc */
                                   if (verbose == 2) {
                                     printf("     #%d  %-16s %s\n", fgd_cnt, incoming->ipadr, incoming->callsign);
                                     printf(" curpos:%d\n", fgd_curpos);
                                   }
                                   fgd_len_msg = strlen ( &fgd_txt[fgd_curpos]);
                                   sscanf( &fgd_txt[fgd_curpos]," %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                                           &incoming->sgFGD_COORD[0][0], &incoming->sgFGD_COORD[0][1], &incoming->sgFGD_COORD[0][2], &incoming->sgFGD_COORD[0][3],
                                           &incoming->sgFGD_COORD[1][0], &incoming->sgFGD_COORD[1][1], &incoming->sgFGD_COORD[1][2], &incoming->sgFGD_COORD[1][3],
                                           &incoming->sgFGD_COORD[2][0], &incoming->sgFGD_COORD[2][1], &incoming->sgFGD_COORD[2][2], &incoming->sgFGD_COORD[2][3],
                                           &incoming->sgFGD_COORD[3][0], &incoming->sgFGD_COORD[3][1], &incoming->sgFGD_COORD[3][2], &incoming->sgFGD_COORD[3][3]);
                                   
                                   if (verbose == 2) {
                                     printf("Incoming Mat4\n");
                                     fgd_print_Mat4( incoming->sgFGD_COORD );        
                                   }
                                   fgd_curpos += fgd_len_msg + 2;
                                   incoming = incoming->next;
                                   } /* end for                 */
//                                 }   /* end else                */
                               }     /* end if "data available" */
                               /* The first view-Mat4 is somebody else */
                               sgCopyMat4(sgFGD_VIEW, head->next->sgFGD_COORD);

/* Here reading the answer of completed command by fgd */
/*                               read( sock, fgd_txt, buffp[3]);
//                               fgd_txt[buffp[3]] = 0;
                               if (strcmp(fgd_txt, "UNKNOWN") == -1) {
                                   if (verbose == 2) printf("FGD: Mat4 Data to Host sent\n");
                                   }
                                   else printf("FGD: Host not in list, sorry...\n");
*/                                   
                               break;
                      case  8: printf("FGD: Unregistering Host %s\n", FGFS_host);
                               read( sock, fgd_txt, buffp[3]);
                               fgd_txt[buffp[3]] = 0;
                               test = head->next;
                               while (test != tail) {
                                  list_clear( test->ipadr );
                                  test = test->next;
                               }
                               fgd_ppl = 0;
/*  This does...
                               if (strcmp(fgd_txt, "UNKNOWN") == 0) {
                                   printf("FGD: Host not in list, sorry...\n");
                               }
                               else printf("FGD: Host %s unregistered\n", fgd_txt);
*/
/*  This does not work on glibc-2.1
                               if (strcmp(fgd_txt, "UNKNOWN") == -1) {
                                   printf("FGD: Host %s unregistered\n", fgd_txt);
                                   }
                               else printf("FGD: Host not in list, sorry...\n"); 
*/
                               break;                               
                      case  9: printf(" Shutdown FlightGear-Deamon %s .\n", fgd_name);
                               break;                               
                      default: break;
                      }
                  } else printf("     Huh?: no deamon present, yuk!!!\n");
//              }
       close(sock);
//       current_port++;
//   }

  if (verbose == 2) printf("fgd_com completed.\n");
}

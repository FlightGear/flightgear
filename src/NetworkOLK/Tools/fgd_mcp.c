/*************************************************************/
/* FGD_MCP.C by Oliver Delise                                */
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
/*             v0.1-beta      Jan 16 2000 libc5/glibc-2.0    */
/*                                        glibc-2.1 cleanups */
/*************************************************************/

#include <stdio.h>
#include "fgd.h"
#include <math.h>

//#define printf //

/* Netstuff */
int sock = -1;
int my_sock;
struct sockaddr_in address;
struct sockaddr_in my_address;
int result;
extern int errno;
int current_port  = 10000; 
u_short base_port = 10000;
u_short end_port  = 10000;
int verbose = 2;
struct hostent *host_info, *f_host_info;
struct servent *service_info;
struct utsname myname;

/* Program-stuff */
int i, j;
int fgd_len_msg = 1, fgd_status, fgd_ele_len, fgd_curpos, fgd_cnt, fgd_ppl,
    fgd_ppl_old, fgd_loss;
size_t anz;
char *fgd_job, *fgd_callsign, *fgd_name, *fgd_ip, *fgd_mcp_ip;
char *buffp, *src_host, *fgd_host, *fgfs_host, *fgfs_pilot, *fgd_txt;

/* List-stuff */

const int True  = 0;
const int False= -1;

struct list_ele {
   unsigned char ipadr[16], callsign[16];
   unsigned char lon[8], lat[8], alt[8], roll[8], pitch[8], yaw[8];
   float lonf, latf, altf, speedf, rollf, pitchf, yawf;   
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
   return(i);
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
   strcpy( fgd_host, "Johnny");
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
   printf("MCP: I'm running on HOST : %s   ", src_host);
   if (host_info = gethostbyname( src_host)) {
     bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
     strcpy((char *) fgd_mcp_ip, (char *) inet_ntoa(address.sin_addr));
     }
   printf("IP : %s\n", fgd_mcp_ip);   
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
            printf(" NOT resolved !!!\n");
            printf("MCP: Could not get %s host entry !\n", fgd_host);
            printf("MCP: Enter '10' for deamon IP or fqdn or alias. (your choice)\n");
//          exit(1);
          } else if (verbose == 2) printf(" address valid\n");
   
   if ((base_port > end_port) || ((short)base_port < 0)) { 
            printf("MCP: Bad port range : start=%d end=%d !\n");
            printf("MCP: Enter '10' for deamon IP/fqdn\n");     
     exit(1);
   } else if (verbose == 2) {
            printf("     Port range: %d to %d\n",base_port,end_port);
            }
}


void fgd_send_com( char *FGD_com, char *FGFS_host) {

   strcpy( buffp, "                ");
   current_port = base_port;
   printf("     Sending : %s\n", FGD_com);
   while (current_port <= end_port) {
/*  fprintf(stderr,"Trying port: %d\n",current_port); */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
     {
	fprintf(stderr, "Error assigning master socket: %s\n",sys_errlist[errno]);
	exit(-1);
     } 

    address.sin_port = htons(current_port);
    printf("     address.sin_port : %d\n",htons(address.sin_port));

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
        switch  (atoi((char *) FGD_com)) {
        case 5:  fgd_len_msg = strlen( fgd_callsign);
                 write( sock, &fgd_len_msg,1);
        /* send string, for the moment, here: callsign */   
                 write( sock, fgd_callsign, fgd_len_msg);
        /* Lon, Lat, Alt, Speed, Roll, Pitch, Yaw
           hope this sprintf call is not too expensive */
                 sprintf( fgd_txt, " %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f %7.3f", 
                          boss->latf, boss->lonf, boss->altf, boss->speedf,
                          boss->rollf, boss->pitchf, boss->yawf);
                 write( sock, fgd_txt, 56);
                 break;
        default: break;
        }
        

/* be verbose, this goes later into own (*void) */ 
        printf("     Message : %s\n", FGD_com);
        switch (verbose) {
        case 0: printf("%d\n",current_port);
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
         exit(1);
       } 
/*     fprintf(stderr,"Error %d connecting socket %d to port %d: %s\n",
                errno,sock,current_port,sys_errlist[errno]); */ 

//              service_info = getservbyport(htons(current_port),"tcp");
//              if (!service_info) {



/* The Receiving Part, fgd returns errormessages, succes, etc... */
                  do { 
                     fgd_status = recv( sock, (char *) buffp, 5, MSG_WAITALL);
                     printf("     status %d\n", fgd_status);
                     }
//                  while ( (fgd_status != 5) && (fgd_status != 0) );
                  while ( (fgd_status == -1) || (fgd_status == -1) );
                  if (verbose == 2) {
                      printf("     Got reply : %x %x %x\n", buffp[0], buffp[1], buffp[2]);
                      printf("     Got reply : %x\n", &buffp);
                  }
                  if (strncmp( buffp, "FGD", 3) == 0) {
                      switch (atoi((char *) FGD_com)) {
                      case  0: read( sock, fgd_name, buffp[3]);
                               fgd_name[buffp[3]] = 0;                      
                               printf("FGD: FlightGear-Deamon %s detected\n", fgd_name);
                               break;
                      case  1: printf("FGD: Registering Host %s\n", FGFS_host);
                               break;
                      case  2: printf("FGD: Showing registered Hosts at %s\n", fgd_host);
                               printf(" %d %d\n", buffp[3], buffp[4]);
                               if ( (buffp[3] + 256 * buffp[4]) != 5 ) {
/* FIXME: replace with SELECT to avoid broken pipes, known bug (-;       */
/*        but the transfer is calculated very accurately, null problemo  */
                  do { 
                      fgd_status = recv( sock, fgd_txt, buffp[3]-5, MSG_WAITALL);
//                    printf("     status %d\n", fgd_status);
                     }
//                  while ( (fgd_status != 4) && (fgd_status != 0) );
                  while ( (fgd_status == -1) || (fgd_status == -1) );                               
//                                 read( sock, fgd_txt, buffp[3]-5);
                                 fgd_curpos = 2;
                                 for (fgd_cnt = 1; fgd_cnt < (fgd_txt[0]+1); fgd_cnt++) {
                                   fgd_ele_len = fgd_txt[fgd_curpos-1];
                                   bcopy( &fgd_txt[fgd_curpos], fgfs_host, fgd_ele_len);
                                   fgfs_host[fgd_ele_len] = 0;
                                   fgd_curpos = fgd_curpos + fgd_ele_len + 1;
                                   printf("     #%d  %s\n", fgd_cnt, fgfs_host);
                                 }
                               }
                               break;
                      case  5: printf("FGD: Receiving data from Host %s\n", FGFS_host);
                               read( sock, fgd_txt, (unsigned char) buffp[3] + 256 * (unsigned char) buffp[4]);
                               fgd_txt[buffp[3]] = 0;
                               if (strcmp(fgd_txt, "UNKNOWN") == 0) {
                                   printf("FGD: Host not in list, sorry...\n");                               
                               }
                               else printf("FGD: Data from Host %s received\n", fgd_txt);
                               break;
                      case  6: printf("FGD: Sending data to Host %s\n", FGFS_host);
                               if (buffp[3] != 5) {
/* FIXME: replace with SELECT */
                  if (verbose == 2) printf("Noch %d bytes\n", (unsigned char) buffp[3] + 256 * (unsigned char) buffp[4]);
                  do { 
                    fgd_status = recv( sock, fgd_txt, (unsigned char) buffp[3]-5, MSG_PEEK);
                    if (verbose == 2) printf("Status %d\n", fgd_status);
                     }
                    while ( (fgd_status == 5) || (fgd_status == -1) );
//                  while ( (fgd_status == -1) || (fgd_status == -1) );                               
                                 read( sock, fgd_txt, buffp[3]-5);
                                 fgd_curpos = 2;
                                 fgd_ppl_old = fgd_ppl;
                                 fgd_ppl = fgd_txt[0];
                                 /* Check if list has changed (pilot joined/left) */
                                 if (fgd_ppl != fgd_ppl_old) {
                                   printf(" List changed!!!\n");
                                   for (fgd_cnt = 1; fgd_cnt <= abs(fgd_ppl - fgd_ppl_old); fgd_cnt++) {
                                     printf(" Checkpoint\n");
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
                                   printf("     #%d  %-16s %s\n", fgd_cnt, incoming->ipadr, incoming->callsign);
                                   printf(" curpos:%d\n", fgd_curpos);
                                   sscanf( &fgd_txt[fgd_curpos]," %7f %7f %7f %7f %7f %7f %7f",
                                           &incoming->latf, &incoming->lonf,
                                           &incoming->altf, &incoming->speedf, &incoming->rollf,
                                           &incoming->pitchf, &incoming->yawf);
                                   printf(" lat   :%7.3f\n lon   :%7.3f\n alt   :%7.3f\n speed :%7.3f\n roll  :%7.3f\n pitch :%7.3f\n yaw   :%7.3f\n",
                                            incoming->latf, incoming->lonf, incoming->altf, incoming->speedf,
                                            incoming->rollf, incoming->pitchf, incoming->yawf);                                   
                                   fgd_curpos += 56;
                                   incoming = incoming->next;
                                   } /* end for                 */
//                                 }   /* end else                */
                               }     /* end if "data available" */
/* Here reading the answer of completed command by fgd */
/*                               read( sock, fgd_txt, buffp[3]);
                               fgd_txt[buffp[3]] = 0;
                               if (strcmp(fgd_txt, "UNKNOWN") == -1) {
                                   printf("FGD: Data to Host sent\n");
                                   }
                                   else printf("FGD: Host not in list, sorry...\n");
*/                                   
                               break;
                      case  8: printf("FGD: Unregistering Host %s\n", FGFS_host);
                               read( sock, fgd_txt, buffp[3]);
                               fgd_txt[buffp[3]] = 0;
                               if (strcmp(fgd_txt, "UNKNOWN") == 0) {
                                   printf("FGD: Host not in list, sorry...\n");
                                   }
                                   else printf("FGD: Host %s unregistered\n", fgd_txt);
                               break;                               
                      case  9: printf(" Shutdown FlightGear-Deamon %s .\n", fgd_name);
                               break;                               
                      default: break;
                      }
                  } else printf("     Huh?: no deamon present, yuk!!!\n");
//              }
       close(sock);
       current_port++;
   }

  if (verbose == 2) printf("fgd_com completed.\n");
}


int main(int argc, char **argv) { 

   list_init();
   fgd_init();
   for ( ; (atoi( (char*) fgd_job)) != 99;){
   printf("MCP: ready...enter commando (42 help) ");
   fgets((char *) fgd_job, 5, stdin);
//   if (verbose == 2) printf("MCP: got %s %d\n", (char *) fgd_job, strlen((char *) fgd_job));
   if ( strcmp( fgd_job, "\n") > 0 ) switch( atoi((char*) fgd_job)) {
     case  0 : if ( strncmp( (char *) fgd_job, "0", 1) == 0 ){
                  printf("MCP: Scan for fgd\n");
                  fgd_send_com( "0", src_host);
               }
               break;
     case  1 : printf("MCP: Register to fgd\n");
               fgd_send_com( "1", src_host);
               break;     
     case  2 : printf("MCP: Show registered\n");
               fgd_send_com( "2", src_host);
               break;     
     case  3 : printf("MCP: Send MSG\n");
               break;
     case  4 : printf("MCP: Send MSG to ALL\n");
               break;     
     case  5 : printf("MCP: Push Data to fgd\n");
               fgd_send_com( "5", src_host);
               break;     
     case  6 : printf("MCP: Pop Data from fgd\n");
               fgd_send_com( "6", src_host);
               break;     
     case  8 : printf("MCP: Unregister from fgd\n");
               fgd_send_com( "8", src_host);
               break;     
     case  9 : printf("MCP: Shutdown fgd-deamon\n");
               fgd_send_com( "9", src_host);
               break;
     case 10 : printf("MCP: Choose default deamon HOST:\n");
               printf("     Deamon          Host            IP              Port\n");
               printf("     %-16s%-16s%-16s%-16d\n", fgd_name, fgd_host, fgd_ip, base_port);
               printf("\n     Enter new Host:[%s]  ", fgd_host);
               fgets((char *) fgd_txt, 32, stdin);
               if ( strlen(fgd_txt) != 1 ) {
                   strcpy(fgd_host, fgd_txt);
                   fgd_host[ strlen( fgd_txt) - 1] = 0;
                   if (host_info = gethostbyname( fgd_host)) {
                       bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
                       strcpy((char *) fgd_ip, (char *) inet_ntoa(address.sin_addr));
                       if (verbose == 2) {
                           printf("MCP: Resolved...FGD running on HOST : %s", fgd_host);
                           printf("   IP : %s\n", fgd_ip);
                       }
                   } else if ((address.sin_addr.s_addr = inet_addr( fgd_host)) == INADDR_NONE) {
                              fprintf(stderr,"MCP: Could not get %s host entry !\n", fgd_host);
                              printf("     NOT resolved !!!\n");
//                            exit(1);
                          } else if (verbose == 2) printf(" address valid\n");
               }
               break;
     case 11 : printf("MCP: Choose default deamon Port:\n");
               printf("     Deamon          Host            IP              Port\n");
               printf("     %-16s%-16s%-16s%-16d\n", fgd_name, fgd_host, fgd_ip, base_port);
               printf("     Enter new Port:[%d] ", base_port);
               fgets((char *) buffp, 16, stdin);
               current_port = atoi((char*) buffp);
               if (current_port < 1025) {
                   printf("MCP: Be fair please...Ports below 1024 are not a good choice\n");
                   current_port = base_port;
                   break;
               }
               if (current_port != 0) {
                   base_port = atoi((char*) buffp);
                   end_port = base_port;
               }
               break;               
     case 20 : printf("MCP: Current values:\n");
               printf("     Deamon          Host            IP              Port\n");
               printf("     %-16s%-16s%-16s%-16d\n", fgd_name, fgd_host, fgd_ip, base_port);
               printf("     -----------------------------------------------------\n");
               printf("     Callsign        Host            IP              Port\n");
               printf("     %-16s%-16s%-16s%-16d\n", fgd_callsign, src_host, fgd_mcp_ip, base_port);
               printf("                Lat     Lon     Alt     Speed    Roll    Pitch    Yaw\n");
               printf("     %-8s % 7.3f % 7.3f % 7.3f % 7.3f % 7.3f % 7.3f % 7.3f\n", fgd_callsign, boss->latf,
                            boss->lonf, boss->altf, boss->speedf, boss->rollf, boss->pitchf, boss->yawf);
               printf("     -----------------------------------------------------\n");                            
               printf("     Pilot list:\n");
               test = head->next;
               while (test != tail) {
               printf("     Callsign        Host\n");
               printf("     %-16s%-16s\n", test->callsign, test->ipadr);
               printf("                Lat     Lon     Alt     Speed    Roll    Pitch    Yaw\n");
               printf("     %-8s % 7.3f % 7.3f % 7.3f % 7.3f % 7.3f % 7.3f % 7.3f\n", test->callsign, test->latf,
                            test->lonf, test->altf, test->speedf, test->rollf, test->pitchf, test->yawf);
               test = test->next;
               }
               printf("     -----------------------------------------------------\n");
               
               break;               
     case 21 : printf("MCP: Enter your callsign, Pilot ");
               fgets((char *) fgd_callsign, 32, stdin);
               fgd_callsign[ strlen(fgd_callsign) - 1 ] = 0;
               break;
     case 42 : printf("MCP: Commands available:\n 0   Scan for fgd\n 1   Register\n");
               printf(" 2   Show registered\n 3   Send MSG\n 4   Send MSG to ALL\n");
               printf(" 5   Push Data to fgd\n 6   Pop Data from fgd\n");
               printf(" 8   Unregister from fgd\n 9   Shutdown fgd\n");
               printf("10   Set deamon HOST\n11   Set deamon PORT\n");
               printf("20   Show values\n21   Set own callsign\n");
//               printf("31   Set deamon PORT\n");
               printf("98   Stress test\n");
               printf("99   Quit Master Control Program (not recommended)\n");
               break;
     case 98 : printf("MCP: Stress test ");
               fgd_loss = 0;
               list_search(fgd_mcp_ip);
               other = act->next;
               printf("other-ip %s\n", other->ipadr);
               sleep(3);
               for ( j=1; j<10000; j++) {
                 boss->latf += 0.001;
                 fgd_send_com( "5", src_host);
                 fgd_send_com( "6", src_host);
                 printf("other lat:%7.3f  boss lat:%7.3f\n", other->latf, boss->latf);
                 if (fabs( (double) boss->latf - (double) other->latf ) > 0.001) {
                   printf("other lat:%7.3f  boss lat:%7.3f\n", other->latf, boss->latf);
                   fgd_loss++;
                 }
               }
               printf(" Packets lost: %d\n", fgd_loss);
               break;
     case 99 : printf("MCP: Good bye...\n");
               break;
     default:  printf("MCP: ???\n");
     }
   }
   // fgd_send_com( argv[5], argv[6]);
   free(buffp);
   free(fgd_job);     
   free(fgd_host);
   free(fgd_callsign);
   free(fgd_name);
   free(fgd_ip);
   free(fgd_mcp_ip);
   free(fgfs_host);
   free(fgfs_pilot);
   free(src_host);
   free(fgd_txt);
   exit(0);
}

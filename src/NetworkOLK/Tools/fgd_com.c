/*************************************************************/
/* FGD_COM.C by Oliver Delise                                */
/* Contact info:                                             */
/* e-mail: delise@mail-isis.de                               */
/* www: http://www.isis.de/members/odelise/progs/flightgear  */
/*                                                           */
/* Version 0.1-beta                                          */
/* The author of this program offers no waranty at all       */
/* about the correct execution of this software material.    */
/* Furthermore, the author can NOT be held responsible for   */
/* any physical or moral damage caused by the use of this    */
/* software.                                                 */
/*                                                           */
/* This is a standalone Tool to communicate with any         */
/* FlightGear-Deamon.                                        */
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
/*             v0.1-alpha     Nov 08 1999                    */
/*             v0.1-beta      Jan 16 2000 -> libc5/glibc-2.0 */
/*                            glibc2.1 cleanups              */
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


/* Netstuff */
int sock = -1;
int my_sock;
struct sockaddr_in address;
struct sockaddr_in my_address;
int result;
extern int errno;
int current_port = 0; 
u_short base_port = 0;
u_short end_port = 1024;
int verbose = 0;
struct hostent *host_info, *f_host_info;
struct servent *service_info;
struct utsname myname;

/* Program-stuff */
int i;
int fgd_len_msg = 1;
size_t anz;
char *buff;
char *src_host, *fgd_host, fgfs_host;
char *usage = "Usage:\n fgd_com [FGD host] [start port] [end port] [-v or -vv] [Commando] [FGFS host]\n";


void fgd_init(void){

   bzero((char *)&address, sizeof(address));
   address.sin_family = AF_INET;
/* determinating the source/sending host */
   if (uname(&myname) == 0) src_host = myname.nodename;
   printf("I'm running on HOST : %s\n", src_host);
/* resolving the destination host, here fgd's host */   
   if (verbose == 2) printf("Resolving: %s ->", fgd_host);
   if (host_info = gethostbyname( fgd_host)) {
     bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
     if (verbose == 2) printf(" resolved\n");
   } else if ((address.sin_addr.s_addr = inet_addr( fgd_host)) == INADDR_NONE) {
            fprintf(stderr,"Could not get %s host entry !\n", fgd_host);
            printf(" NOT resolved !!!\n");
            exit(1);
          } else if (verbose == 2) printf(" address valid\n");
   
   if ((base_port > end_port) || ((short)base_port < 0)) { 
     fprintf(stderr,"Bad port range : start=%d end=%d !\n");
     exit(1);
   } else if (verbose == 2) {
            printf("Port range: %d to %d\n",base_port,end_port);
            }
}


void fgd_send_com( char *FGD_com, char *FGFS_host) {
   current_port = base_port;
   printf("Sending : %s\n", FGD_com);
   while (current_port <= end_port) {
/*  fprintf(stderr,"Trying port: %d\n",current_port); */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
     {
	fprintf(stderr, "Error assigning master socket: %s\n",sys_errlist[errno]);
	exit(-1);
     } 

    address.sin_port = htons(current_port);
    printf("address.sin_port : %d\n",htons(address.sin_port));

    f_host_info = gethostbyname(src_host);

//printf ("src_host : %s", ntohs(f_host_info->h_addr));
    
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) == 0) {

//     write( sock, FGD_com, 1);

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

     printf(" Message : %s\n", FGD_com);
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
              printf("Port %d found. Service name unknown\n",current_port);
              } else {
              printf("Port %d found. Service name: %s\n",current_port,service_info->s_name);
              }
              break; 
     } 
    }  else if (errno == 113) {
         fprintf(stderr,"No route to host !\n");
         exit(1);
       } 
/*     fprintf(stderr,"Error %d connecting socket %d to port %d: %s\n",
     errno,sock,current_port,sys_errlist[errno]); */ 
     close(sock);
     current_port++;
   }

  if (verbose == 2) printf("fgd_com terminated.\n");
}


int main(int argc, char **argv) { 

   if (argc < 6) {
    fprintf(stderr, usage);
    exit(1);
   }
   printf("argc %d argv[5] %s\n",argc,argv[5]);
   switch (argc) {
     case 7: printf("fgd commando : %s\n",argv[5]);
             base_port = (u_short)atoi(argv[2]);
             end_port = (u_short)atoi(argv[3]);
             fgd_host = argv[1];
             verbose = 2;
             break;
     case 5: if (!strcmp(argv[4],"-v")) 
               verbose = 1;
             else if (!strcmp(argv[4],"-vv"))
                    verbose = 2;
               else { fprintf(stderr, usage);
                      exit(1); }

     case 4: base_port = (u_short)atoi(argv[2]);
             end_port = (u_short)atoi(argv[3]);
             break;
     default: fprintf(stderr, usage);
              exit(1);
              break;
   }
   fgd_init();
   fgd_send_com( argv[5], argv[6]);
   exit(0);
}

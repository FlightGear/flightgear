/***********************************************************/
/* FGD_SCAN.C by Oliver Delise                             */
/* Contact info:                                           */
/* e-mail: delise@rp-plus.de                               */
/* www: http://www.online-club.de/~olk/progs/mmx-emu/      */
/* ftp: http://www.online-club.de/~olk/progs/flightgear    */ 
/*                                                         */
/* Version 0.1pre-alpha                                    */
/* The author of this program offers no waranty at all     */
/* about the correct execution of this software material.  */
/* Furthermore, the author can NOT be held responsible for */
/* any physical or moral damage caused by the use of this  */
/* software.                                               */
/*                                                         */
/* This is a standalone Tool to scan for any FlightGear    */
/* Deamon.                                                 */
/* This is Open Source Software with many parts            */
/* shamelessly stolen from others...                       */
/*                                                         */
/* -> This program will scan for TCP port listening on a   */
/*    remote or local host inside the range you give to it.*/
/*    I offer no warranty over the accuracy though :)      */
/*    There are 3 verbose modes: No info, service info, and*/
/*    full info. No info is good of you only want the list */
/*    of the ports, no more info. The best mode is Full    */
/*    info, as you get error information,etc. The main     */
/*    output is STDOUT, and ALL the errors go to STDERR.   */
/*                                                         */
/*    History: v0.1pre-alpha: May 25 1999 -> First release */
/***********************************************************/


#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/utsname.h>

int i;
int sock = -1;
int my_sock;
struct sockaddr_in address;
struct sockaddr_in my_address;
int result;
extern char *sys_errlist[];
extern int errno;
int current_port  = 20000; 
u_short base_port = 20000;
u_short end_port  = 20100;
int verbose = 0;
struct hostent *host_info, *f_host_info;
struct servent *service_info;
struct utsname myname;

size_t anz;
char *buff;
char *src_host;

void port_scan( char FGD_com);
void fgd_scan();


int main(int argc, char **argv)
{ 
   if (argc < 4) {
    fprintf(stderr,"Usage: fgd_scan [host] [start port] [end port] <-v or -vv>\n");
    exit(1);
   }
   printf("argc %d argv[5] %s\n",argc,argv[5]);
   verbose = 0;
   switch (argc) {
/*     case 5: base_port = (u_short)atoi(argv[2]);
             end_port = (u_short)atoi(argv[3]);
             verbose = 2;
             src_host = argv[6];
             break; */
     case 5: if (!strcmp(argv[4],"-v")) 
               verbose = 1;
             else if (!strcmp(argv[4],"-vv"))
                    verbose = 2;
               else { fprintf(stderr,"Usage: fgd_scan [host] [start port] [end port] <-v or -vv>\n");
                      exit(1); }

     case 4: base_port = (u_short)atoi(argv[2]);
             end_port = (u_short)atoi(argv[3]);
             break;
     default: fprintf(stderr,"Usage: fgd_scan [host] [start port] [end port] <-v or -vv>\n");
              exit(1);
              break;
   }
   
   bzero((char *)&address, sizeof(address));
   address.sin_family = AF_INET;
/* determinating the source/sending host */
   if (uname(&myname) == 0) src_host = myname.nodename;
   printf("I'm running on HOST : %s\n", src_host);
/* resolving the destination host, here: fgd's host */   
   if (verbose == 2) printf("Resolving: %s ->",argv[1]);
   if (host_info = gethostbyname(argv[1])) {
     bcopy(host_info->h_addr, (char *)&address.sin_addr,host_info->h_length);
     if (verbose == 2) printf(" resolved\n");
   } else if ((address.sin_addr.s_addr = inet_addr(argv[1])) == INADDR_NONE) {
            fprintf(stderr,"Could not get %s host entry !\n",argv[1]);
            printf(" NOT resolved !!!\n");
            exit(1);
          } else if (verbose == 2) printf(" address valid\n");
   
   if ((base_port > end_port) || ((short)base_port < 0)) { 
     fprintf(stderr,"Bad port range : start=%d end=%d !\n");
     exit(1);
   } else if (verbose == 2) {
            printf("Port range: %d to %d\n",base_port,end_port);
     }
     
   fgd_scan();
   exit(0);
}

int fgd_len_msg = 1;


/******* HERE SCAN ROUTINE *******/

void fgd_scan() {
   current_port = base_port;
   printf("Scanning for fgd...\n");
   while (current_port <= end_port) {
     fprintf(stderr,"Trying port: %d\n",current_port); 
     sock = socket(PF_INET, SOCK_STREAM, 0);
     if (sock == -1) {
	fprintf(stderr, "Error assigning master socket: %s\n",sys_errlist[errno]);
	exit(-1);
     } 
     address.sin_port = htons(current_port);
     printf("address.sin_port : %d\n",htons(address.sin_port));
     f_host_info = gethostbyname(src_host);

/* printf ("src_host : %s", ntohs(f_host_info->h_addr)); */
    
    if (connect(sock, (struct sockaddr *)&address, sizeof(address)) == 0) {

/* we determine length of our ip */
     fgd_len_msg = (int) sizeof(f_host_info->h_addr);
/* first we send length of ip */
     write( sock, &fgd_len_msg,1);
/* then we send our ip */     
     write( sock, f_host_info->h_addr, fgd_len_msg);
/* we send the command, here 0 : we ask fgd to identify itself */
     write( sock, "0", 1);
     printf("verbose: %d", verbose);

     printf(" Inquiring FGD to identify itself\n");
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
                read( sock, &buff, 3);
                printf(" Got reply : %s\n", &buff);
                if (strncmp(&buff, "FGD", 3) == 0) {
                  printf(" FlightGear-Deamon detected\n");
                  break;
                }
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

  if (verbose == 2) printf("FGD scan finished !\n");
}


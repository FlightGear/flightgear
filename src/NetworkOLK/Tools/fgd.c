/***********************************************************/
/* FGD.C by Oliver Delise                                  */
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
/* This is a standalone Tool to communicate with any       */
/* FlightGear System and FGFS-Deamon.                      */
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


/* Net-stuff */
fd_set rset, allset;
int  maxfd, nready, retval;
struct timeval tv;
struct utsname myname;
char *fgd_host, *src_host;

/* List-stuff (doubly-Linked-list) */
#include <string.h>
#include <stdlib.h>

int i,j;
char *vb;
const int True  = 0;
const int False= -1;

struct list_ele {
   unsigned char ipadr[16], squak[16];
   float *lon, *lat, *alt, *roll, *pitch, *yaw;
   struct list_ele *next, *prev;
};

struct list_ele *head, *tail, *act, *test;  /*  fgd_msg; */

/*...Create head and tail of list */
void list_init( void) {
   head = (struct list_ele*) malloc(sizeof(struct list_ele));
   tail = (struct list_ele*) malloc(sizeof(struct list_ele));
   if (head == NULL || tail == NULL) { printf("Out of memory\n"); exit(1); }
/* fixme :Where is the "with foobar do command "
   head->ipadr = "127.0.0.0";
   head->squak = "None";
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

void list_insert( char newip[16]) {
struct list_ele *new_ele;

   new_ele = (struct list_ele*) malloc(sizeof(struct list_ele));
   if (new_ele == NULL) { printf("Out of memory\n"); exit(1); }
   strcpy(new_ele->ipadr, newip);
   list_search( newip);
   new_ele->prev = act;
   new_ele->next = act->next;
   act->next->prev = act->next = new_ele;
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

int list_not_in( char name[16]) {
   
   i = True;
   test = head->next;
   while ((test != tail) && (i==True)) {
     i = (strcmp(test->ipadr, name) ? True : False);
     test = test->next;
     printf("list_not_in : %d\n",i);
   }
   return(i);
}




int i;
int sock = -1;
int my_sock;
int fgd_com;
size_t anz;
char *buff;
struct { char *adr, *squak, *lon, *lat, *alt;} fgd_client;
int fgd_ele_len;

struct sockaddr_in address;
struct sockaddr_in my_address;
int result;
extern char *sys_errlist[];
extern int errno;
int current_port = 0; 
u_short base_port = 0;
u_short end_port = 1024;
int verbose = 0;
struct hostent *host_info;
struct servent *service_info;

void fgd_init(void);

int main(int argc, char **argv)
{ 
   if (argc < 3) {
    fprintf(stderr,"Usage: fgd [start port] [end port] <-v or -vv>\n");
    exit(1);
   }
   
   switch (argc) {
     case 4: if (!strcmp(argv[3],"-v")) 
               verbose = 1;
             else if (!strcmp(argv[3],"-vv"))
                    verbose = 2;
               else { fprintf(stderr,"Usage: fgd [start port] [end port] <-v or -vv>\n");
                      exit(1); }

     case 3: base_port = (u_short)atoi(argv[1]);
             end_port = (u_short)atoi(argv[2]);
             break;
     default: fprintf(stderr,"Usage: fgd [start port] [end port] <-v or -vv>\n");
              exit(1);
              break;
   }
   
   bzero((char *)&address, sizeof(address));
   address.sin_family = AF_INET;
   if (uname(&myname) == 0) fgd_host = myname.nodename;
   printf("I am running on HOST : %s\n", fgd_host);

   if (verbose == 2) printf("Resolving: %s ->",fgd_host);
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
            printf("Port range: %d to %d\n",base_port,end_port);
     }
   vb = (char) &verbose;
   printf("vb %s",&vb);
   exit(0);
   list_init();
   fgd_init();
   exit(0);
}

void fgd_init(void) {

struct { char *ip, *squak, *lon, *lat, *alt;} fg_id;


   current_port = base_port;
   while (current_port <= end_port) {
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
     {
	fprintf(stderr, "Error assigning master socket: %s\n",sys_errlist[errno]);
	exit(-1);
     } 

    address.sin_port = htons(current_port);
    printf("address.sin_port : %d\n",htons(address.sin_port));
    if (1 == 1) {
    bind(sock, (struct sockaddr *)&address, sizeof(address));
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
      my_sock = accept( sock, (struct sockaddr *)&address, sizeof(address));
      anz = 2;

/* reading length of senders' ip */
      fgd_ele_len =  0;
      buff = "";
      read( my_sock, &buff, 1);
      fgd_ele_len =  (int)(char) buff;
/* reading senders' ip */
      read( my_sock, &buff, fgd_ele_len);
      fgd_client.adr = inet_ntoa(buff);
      src_host = fgd_client.adr;
/* reading commando */
      read( my_sock, &buff, 1);
      printf(" Commando received : %s from Host : %s\n", &buff, src_host);
      fgd_com = (int) atoi(&buff);
      switch (fgd_com) {
      case 0:	printf(" fgd   : Identify\n");
                write( my_sock, "FGD", 3);                
                close(my_sock);
                break;      
      case 1:	printf(" fgd   : Register\n");
      /* reading length of FGFS_host ip */
                fgd_ele_len =  0;
                buff = "";
                read( my_sock, &buff, 1);
                fgd_ele_len =  (int)(char) buff;
      /* reading FGFS_host ip */
                read( my_sock, &buff, fgd_ele_len);
                fgd_client.adr = inet_ntoa(buff);
		if (list_not_in(fgd_client.adr) == True) list_insert(fgd_client.adr);
		printf("    IP : %s\n", fgd_client.adr);
		printf(" PILOT : %s\n", "OLK");
		printf("   LON : %s\n", "42.26");
		printf("   LAT : %s\n", "21.89");
		printf("   ALT : %s\n", "6000");
		close(my_sock);
      		break;
      case 2:	printf(" fgd   : Show Registered\n");
      		test = head->next;
      		while (test != tail) { 
      		   printf("    ip : %s\n", test->ipadr);
      		   test = test->next;
      		}
      		close(my_sock);
      		break;      		
      case 3:	printf(" fgd   : Send MSG\n");
                close(my_sock);
      		break;
      case 4:	printf(" fgd   : Send MSG to all\n");
      		close(my_sock);
      		break;      		
      case 5:	printf(" fgd   : Scan for fgd's\n");
      		close(my_sock);
      		break;      		      	
      case 6:	printf(" fgd   : Update\n");
      		close(my_sock);
      		break;
      case 8:	printf(" fgd   : Unregister\n");
		read( my_sock, &buff, 1);
		fgd_ele_len = (int) &buff;
		read( my_sock, &buff, fgd_ele_len);
		fgd_client.adr = inet_ntoa(buff);
		printf("    IP : %s\n", fgd_client.adr);
		printf("not_in (CASE) : %d\n", list_not_in(fgd_client.adr));
		
		if (list_not_in(fgd_client.adr) == -1) list_clear(fgd_client.adr);
                close(my_sock);
      		break;  
      case 9:	printf(" fgd   : Shutdown\n");
                close(my_sock);
                close(sock);
                exit(0);      		    		
      default:	printf(" fgd   : Illegal Command\n");
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


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/utsname.h>

/* this seems to be missing for glibc-2.0.x */
/* libc5 & glibc-2.1 do have them           */
#ifndef MSG_PEEK
int     MSG_PEEK        = 0x02;    /* Peek at incoming messages.  */
#define MSG_PEEK        MSG_PEEK
#endif
#ifndef MSG_WAITALL
int     MSG_WAITALL     = 0x100;   /* Wait for a full request.  */
#define MSG_WAITALL     MSG_WAITALL
#endif

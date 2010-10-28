
#ifndef __FG_MAIN_HXX
#define __FG_MAIN_HXX 1

void fgUpdateTimeDepCalcs();

int fgMainInit( int argc, char **argv );


extern int idle_state;

extern char *homedir;
extern char *hostname;
extern bool free_hostname;

#endif

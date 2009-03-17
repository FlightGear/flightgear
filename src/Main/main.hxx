
#ifndef __FG_MAIN_HXX
#define __FG_MAIN_HXX 1

void fgUpdateTimeDepCalcs();

bool fgMainInit( int argc, char **argv );


extern int idle_state;
extern long global_multi_loop;
extern double delta_time_sec;

extern char *homedir;
extern char *hostname;
extern bool free_hostname;

#endif

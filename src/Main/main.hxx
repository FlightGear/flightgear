
#ifndef __FG_MAIN_HXX
#define __FG_MAIN_HXX 1

class SGSky;

extern SGSky *thesky;

void fgLoadDCS (void);
void fgUpdateDCS (void);
void fgBuildRenderStates( void );
void fgInitVisuals( void );
void fgRenderFrame();
void fgUpdateTimeDepCalcs();
void fgInitTimeDepCalcs( void );
void fgReshape( int width, int height );
void fgLoadDCS(void);
void fgUpdateDCS (void);

bool fgMainInit( int argc, char **argv );


#endif

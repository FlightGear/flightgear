
#ifndef __FG_MAIN_HXX
#define __FG_MAIN_HXX 1

#include <simgear/screen/extensions.hxx>

class SGSky;
extern SGSky *thesky;

#define FG_ENABLE_MULTIPASS_CLOUDS 1

extern glPointParameterfProc glPointParameterfPtr;
extern glPointParameterfvProc glPointParameterfvPtr;
extern bool glPointParameterIsSupported;

void fgLoadDCS (void);
void fgUpdateDCS (void);
void fgBuildRenderStates( void );
void fgInitVisuals( void );
void fgRenderFrame();
void fgUpdateTimeDepCalcs();
void fgInitTimeDepCalcs( void );
void fgReshape( int width, int height );

bool fgMainInit( int argc, char **argv );


#endif


#ifndef __FG_RENDERER_HXX
#define __FG_RENDERER_HXX 1

#include <simgear/screen/extensions.hxx>
#include <simgear/scene/sky/sky.hxx>

#define FG_ENABLE_MULTIPASS_CLOUDS 1

class SGSky;
extern SGSky *thesky;

extern glPointParameterfProc glPointParameterfPtr;
extern glPointParameterfvProc glPointParameterfvPtr;
extern bool glPointParameterIsSupported;

void fgRenderFrame();
void fgReshape(int width, int height);

class FGRenderer {

public:
    FGRenderer() {}
    ~FGRenderer() {}

    void init();
    void update(double dt);
    void resize(int width, int height );

    void screendump();
    void build_states();
};

#endif

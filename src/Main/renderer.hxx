
#ifndef __FG_RENDERER_HXX
#define __FG_RENDERER_HXX 1

#include <simgear/screen/extensions.hxx>
#include <simgear/scene/util/SGPickCallback.hxx>

#include <osg/Camera>
#include <osgViewer/Viewer>

#include "FGManipulator.hxx"

#define FG_ENABLE_MULTIPASS_CLOUDS 1

class SGSky;
extern SGSky *thesky;

extern glPointParameterfProc glPointParameterfPtr;
extern glPointParameterfvProc glPointParameterfvPtr;
extern bool glPointParameterIsSupported;
extern bool glPointSpriteIsSupported;


class FGRenderer {

public:

    FGRenderer();
    ~FGRenderer();

    void splashinit();
    void init();

    static void resize(int width, int height );

    // calling update( refresh_camera_settings = false ) will not
    // touch window or camera settings.  This is useful for the tiled
    // renderer which needs to set the view frustum itself.
    static void update( bool refresh_camera_settings );
    inline static void update() { update( true ); }

    /** Set all the camera parameters at once. aspectRatio is height / width.
     */
    static void setCameraParameters(float vfov, float aspectRatio,
                                    float zNear, float zFar);
    /** Just pick into the scene and return the pick callbacks on the way ...
     */
    static bool pick( std::vector<SGSceneryPick>& pickList,
                      const osgGA::GUIEventAdapter* ea );

    /** Get and set the OSG Viewer object, if any.
     */
    osgViewer::Viewer* getViewer() { return viewer.get(); }
    const osgViewer::Viewer* getViewer() const { return viewer.get(); }
    void setViewer(osgViewer::Viewer* viewer) { this->viewer = viewer; }
    /** Get and set the manipulator object, if any.
     */
    flightgear::FGManipulator* getManipulator() { return manipulator.get(); }
    const flightgear::FGManipulator* getManipulator() const { return manipulator.get(); }
    void setManipulator(flightgear::FGManipulator* manipulator) {
        this->manipulator = manipulator;
    }

    /** Add a top level camera.
    */
    void addCamera(osg::Camera* camera, bool useSceneData);

protected:
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<flightgear::FGManipulator> manipulator;
};

bool fgDumpSceneGraphToFile(const char* filename);
bool fgDumpTerrainBranchToFile(const char* filename);

#endif

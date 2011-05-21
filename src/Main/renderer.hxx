
#ifndef __FG_RENDERER_HXX
#define __FG_RENDERER_HXX 1

#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/props/props.hxx>

#include <osg/ref_ptr>

namespace osg
{
class Camera;
class Group;
}

namespace osgGA
{
class GUIEventAdapter;
}

namespace osgShadow
{
class ShadowedScene;
}

namespace osgViewer
{
class Viewer;
}

namespace flightgear
{
class FGEventHandler;
}

class SGSky;
extern SGSky *thesky;

class FGRenderer {

public:

    FGRenderer();
    ~FGRenderer();

    void splashinit();
    void init();

    void resize(int width, int height );

    // calling update( refresh_camera_settings = false ) will not
    // touch window or camera settings.  This is useful for the tiled
    // renderer which needs to set the view frustum itself.
    void update( bool refresh_camera_settings);
    static void update();

    /** Just pick into the scene and return the pick callbacks on the way ...
     */
    bool pick( std::vector<SGSceneryPick>& pickList,
               const osgGA::GUIEventAdapter* ea );

    /** Get and set the OSG Viewer object, if any.
     */
    osgViewer::Viewer* getViewer() { return viewer.get(); }
    const osgViewer::Viewer* getViewer() const { return viewer.get(); }
    void setViewer(osgViewer::Viewer* viewer);
    /** Get and set the manipulator object, if any.
     */
    flightgear::FGEventHandler* getEventHandler() { return eventHandler.get(); }
    const flightgear::FGEventHandler* getEventHandler() const { return eventHandler.get(); }
    void setEventHandler(flightgear::FGEventHandler* manipulator);

    /** Add a top level camera.
    */
    void addCamera(osg::Camera* camera, bool useSceneData);

protected:
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<flightgear::FGEventHandler> eventHandler;
    SGPropertyNode_ptr _scenery_loaded,_scenery_override;
    SGPropertyNode_ptr _skyblend;
    SGPropertyNode_ptr _point_sprites, _enhanced_lighting, _distance_attenuation;
    SGPropertyNode_ptr _textures;
    SGPropertyNode_ptr _cloud_status, _visibility_m; 
    SGPropertyNode_ptr _xsize, _ysize;
    SGPropertyNode_ptr _panel_hotspots, _sim_delta_sec, _horizon_effect, _altitude_ft;
    SGPropertyNode_ptr _virtual_cockpit;
    bool _splash_screen_active;
};

bool fgDumpSceneGraphToFile(const char* filename);
bool fgDumpTerrainBranchToFile(const char* filename);

namespace flightgear
{
bool printVisibleSceneInfo(FGRenderer* renderer);
}

#endif

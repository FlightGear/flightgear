#ifndef __FG_RENDERER_HXX
#define __FG_RENDERER_HXX 1

#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/timestamp.hxx>

#include <osg/ref_ptr>
#include <osg/Matrix>
#include <osg/Vec2>
#include <osg/Vec3>

namespace osg
{
class Camera;
class Group;
class GraphicsContext;
class FrameStamp;
}

namespace osgGA
{
class GUIEventAdapter;
}

namespace osgViewer
{
class Viewer;
}

namespace flightgear
{
class FGEventHandler;
class CameraGroup;
class PUICamera;
}

class SGSky;
class SGUpdateVisitor;
class SplashScreen;
class QQuickDrawable;

typedef std::vector<SGSceneryPick> PickList;

class FGRenderer {

public:

    FGRenderer();
    ~FGRenderer();

    void preinit();
    void init();

    void setupView();

    void resize(int width, int height );

    void update();

    /** Just pick into the scene and return the pick callbacks on the way ...
     */
    PickList pick(const osg::Vec2& windowPos);

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

    void removeCamera(osg::Camera* camera);

    SGSky* getSky() const { return _sky; }

	void setPlanes( double zNear, double zFar );

protected:
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<flightgear::FGEventHandler> eventHandler;

    osg::ref_ptr<osg::FrameStamp> _frameStamp;
    osg::ref_ptr<SGUpdateVisitor> _updateVisitor;
    osg::ref_ptr<osg::Group> _viewerSceneRoot;
    osg::ref_ptr<osg::Group> _root;

    SGPropertyNode_ptr _scenery_loaded, _position_finalized;

    SGPropertyNode_ptr _splash_alpha;
    SGPropertyNode_ptr _enhanced_lighting;
    SGPropertyNode_ptr _textures;
    SGPropertyNode_ptr _cloud_status, _visibility_m;
    SGPropertyNode_ptr _xsize, _ysize;
    SGPropertyNode_ptr _panel_hotspots, _sim_delta_sec, _horizon_effect, _altitude_ft;
    SGPropertyNode_ptr _virtual_cockpit;
    SGTimeStamp _splash_time;
    SGSky* _sky;
    int MaximumTextureSize;

    typedef std::vector<SGPropertyChangeListener*> SGPropertyChangeListenerVec;
    SGPropertyChangeListenerVec _listeners;

    void addChangeListener(SGPropertyChangeListener* l, const char* path);

    void updateSky();

    void setupRoot();

    SplashScreen* _splash;
    QQuickDrawable* _quickDrawable = nullptr;
    flightgear::PUICamera* _puiCamera = nullptr;
};

bool fgDumpSceneGraphToFile(const char* filename);
bool fgDumpTerrainBranchToFile(const char* filename);

namespace flightgear
{
bool printVisibleSceneInfo(FGRenderer* renderer);
}

#endif

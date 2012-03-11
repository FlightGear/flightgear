
#ifndef __FG_RENDERER_HXX
#define __FG_RENDERER_HXX 1

#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/timestamp.hxx>

#include <osg/ref_ptr>
#include <osg/Matrix>

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
struct CameraInfo;
class CameraGroup;
}

class SGSky;

class FGRenderer {

public:

    FGRenderer();
    ~FGRenderer();

    void splashinit();
    void init();

    void setupView();

    void resize(int width, int height );

    void update();
  
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

    /** Add a camera to the group. The camera is added to the viewer
     * as a slave. See osgViewer::Viewer::addSlave.
     * @param flags properties of the camera; see CameraGroup::Flags
     * @param projection slave projection matrix
     * @param view slave view matrix
     * @param useMasterSceneData whether the camera displays the
     * viewer's scene data.
     * @return a CameraInfo object for the camera.
     */
	flightgear::CameraInfo* buildRenderingPipeline(flightgear::CameraGroup* cgroup, unsigned flags, osg::Camera* camera,
                                   const osg::Matrix& view,
                                   const osg::Matrix& projection,
                                   bool useMasterSceneData);

	/**
	 */
	flightgear::CameraInfo* buildClassicalPipeline(flightgear::CameraGroup* cgroup, unsigned flags, osg::Camera* camera,
                                   const osg::Matrix& view,
                                   const osg::Matrix& projection,
                                   bool useMasterSceneData);

	/**
	 */
	flightgear::CameraInfo* buildDeferredPipeline(flightgear::CameraGroup* cgroup, unsigned flags, osg::Camera* camera,
                                   const osg::Matrix& view,
                                   const osg::Matrix& projection);

    SGSky* getSky() const { return _sky; }
    
    /**
     * inform the renderer when the global (2D) panel is changed
     */
    void panelChanged();
protected:
    osg::ref_ptr<osgViewer::Viewer> viewer;
    osg::ref_ptr<flightgear::FGEventHandler> eventHandler;
    SGPropertyNode_ptr _scenery_loaded,_scenery_override;
    SGPropertyNode_ptr _skyblend, _splash_alpha;
    SGPropertyNode_ptr _point_sprites, _enhanced_lighting, _distance_attenuation;
    SGPropertyNode_ptr _textures;
    SGPropertyNode_ptr _cloud_status, _visibility_m; 
    SGPropertyNode_ptr _xsize, _ysize;
    SGPropertyNode_ptr _panel_hotspots, _sim_delta_sec, _horizon_effect, _altitude_ft;
    SGPropertyNode_ptr _virtual_cockpit;
    SGTimeStamp _splash_time;
    SGSky* _sky;
	bool _classicalRenderer;
};

bool fgDumpSceneGraphToFile(const char* filename);
bool fgDumpTerrainBranchToFile(const char* filename);

namespace flightgear
{
bool printVisibleSceneInfo(FGRenderer* renderer);
}

#endif

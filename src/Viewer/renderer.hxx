
#ifndef __FG_RENDERER_HXX
#define __FG_RENDERER_HXX 1

#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/props/props.hxx>
#include <simgear/timing/timestamp.hxx>

#include <osg/ref_ptr>
#include <osg/Matrix>
#include <osg/Vec3>

#include "renderingpipeline.hxx"

namespace osg
{
class Camera;
class Group;
class GraphicsContext;
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

    void removeCamera(osg::Camera* camera);
  
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
								   osg::GraphicsContext* gc,
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
                                   const osg::Matrix& view, const osg::Matrix& projection, osg::GraphicsContext* gc);

    void updateShadowCamera(const flightgear::CameraInfo* info, const osg::Vec3d& position);
    void updateShadowMapSize(int mapSize);
    void enableShadows(bool enabled);
    void updateCascadeFar(int index, float far_m);
    void updateCascadeNumber(size_t num);

    SGSky* getSky() const { return _sky; }

	void setPlanes( double zNear, double zFar );

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
    std::string _renderer;
    int _shadowMapSize;
    size_t _numCascades;
    float _cascadeFar[4];
    bool _useColorForDepth;

    flightgear::CameraInfo* buildCameraFromRenderingPipeline(FGRenderingPipeline* rpipe, flightgear::CameraGroup* cgroup, unsigned flags, osg::Camera* camera,
                                        const osg::Matrix& view, const osg::Matrix& projection, osg::GraphicsContext* gc);

    void buildBuffers(FGRenderingPipeline* rpipe, flightgear::CameraInfo* info);
    void buildStage(flightgear::CameraInfo* info, FGRenderingPipeline::Stage* stage, flightgear::CameraGroup* cgroup, osg::Camera* mainCamera, const osg::Matrix& view, const osg::Matrix& projection, osg::GraphicsContext* gc);
    osg::Node* buildPass(flightgear::CameraInfo* info, FGRenderingPipeline::Pass* pass);
    osg::Node* buildLightingSkyCloudsPass(FGRenderingPipeline::Pass* pass);
    osg::Node* buildLightingLightsPass(flightgear::CameraInfo* info, FGRenderingPipeline::Pass* pass);

    osg::Camera* buildDeferredGeometryCamera( flightgear::CameraInfo* info, osg::GraphicsContext* gc, const std::string& name, const FGRenderingPipeline::AttachmentList& attachments );
    osg::Camera* buildDeferredShadowCamera( flightgear::CameraInfo* info, osg::GraphicsContext* gc, const std::string& name, const FGRenderingPipeline::AttachmentList& attachments );
    osg::Camera* buildDeferredLightingCamera( flightgear::CameraInfo* info, osg::GraphicsContext* gc, const FGRenderingPipeline::Stage* stage );
    osg::Camera* buildDeferredFullscreenCamera( flightgear::CameraInfo* info, osg::GraphicsContext* gc, const FGRenderingPipeline::Stage* stage );
    osg::Camera* buildDeferredFullscreenCamera( flightgear::CameraInfo* info, const FGRenderingPipeline::Pass* pass );
    void buildDeferredDisplayCamera( osg::Camera* camera, flightgear::CameraInfo* info, const FGRenderingPipeline::Stage* stage, osg::GraphicsContext* gc );

    void updateShadowCascade(const flightgear::CameraInfo* info, osg::Camera* camera, osg::Group* grp, int idx, double left, double right, double bottom, double top, double zNear, double f1, double f2);
    osg::Vec3 getSunDirection() const;
    osg::ref_ptr<osg::Uniform> _ambientFactor;
    osg::ref_ptr<osg::Uniform> _sunDiffuse;
    osg::ref_ptr<osg::Uniform> _sunSpecular;
    osg::ref_ptr<osg::Uniform> _sunDirection;
    osg::ref_ptr<osg::Uniform> _planes;
    osg::ref_ptr<osg::Uniform> _fogColor;
    osg::ref_ptr<osg::Uniform> _fogDensity;
    osg::ref_ptr<osg::Uniform> _shadowNumber;
    osg::ref_ptr<osg::Uniform> _shadowDistances;
    osg::ref_ptr<osg::Uniform> _depthInColor;

    osg::ref_ptr<FGRenderingPipeline> _pipeline;
};

bool fgDumpSceneGraphToFile(const char* filename);
bool fgDumpTerrainBranchToFile(const char* filename);

namespace flightgear
{
bool printVisibleSceneInfo(FGRenderer* renderer);
}

#endif

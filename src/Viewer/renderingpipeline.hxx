
#ifndef __FG_RENDERINGPIPELINE_HXX
#define __FG_RENDERINGPIPELINE_HXX 1

#include <osg/ref_ptr>
#include <string>

namespace simgear
{
class SGReaderWriterOptions;
}
namespace flightgear
{
struct CameraInfo;
class CameraGroup;
}

class FGRenderingPipeline;
namespace flightgear {
    FGRenderingPipeline* makeRenderingPipeline(const std::string& name,
                   const simgear::SGReaderWriterOptions* options);
}

class FGRenderingPipeline : public osg::Referenced {
public:
    struct Buffer : public osg::Referenced {
        Buffer(SGPropertyNode* prop);

        std::string name;
        GLint internalFormat;
        GLenum sourceFormat;
        GLenum sourceType;
        int width;
        int height;
        float scaleFactor;
        GLenum wrapMode;
		bool shadowComparison;
		//GLenum shadowTextureMode;
		//osg::Vec4 borderColor;
    };

    struct Pass : public osg::Referenced {
        Pass(SGPropertyNode* prop);

        std::string name;
        std::string type;
    };

    struct Stage : public osg::Referenced {
        Stage(SGPropertyNode* prop);

        std::string name;
        std::string type;

        std::vector<osg::ref_ptr<Pass> > passes;
    };
    FGRenderingPipeline();

    flightgear::CameraInfo* buildCamera(flightgear::CameraGroup* cgroup,
                                        unsigned flags,
                                        osg::Camera* camera,
                                        const osg::Matrix& view,
                                        const osg::Matrix& projection,
                                        osg::GraphicsContext* gc);

private:
    std::vector<osg::ref_ptr<Buffer> > buffers;
    std::vector<osg::ref_ptr<Stage> > stages;

    void buildBuffers(flightgear::CameraInfo* info);
    void buildStage(flightgear::CameraInfo* info,
                    Stage* stage,
                    flightgear::CameraGroup* cgroup,
                    osg::Camera* camera,
                    const osg::Matrix& view,
                    const osg::Matrix& projection,
                    osg::GraphicsContext* gc);
    void buildMainCamera(flightgear::CameraInfo* info,
                    Stage* stage,
                    flightgear::CameraGroup* cgroup,
                    osg::Camera* camera,
                    const osg::Matrix& view,
                    const osg::Matrix& projection,
                    osg::GraphicsContext* gc);

    friend FGRenderingPipeline* flightgear::makeRenderingPipeline(const std::string& name,
                   const simgear::SGReaderWriterOptions* options);
};

#endif

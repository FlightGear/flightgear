
#ifndef __FG_RENDERINGPIPELINE_HXX
#define __FG_RENDERINGPIPELINE_HXX 1

#include <osg/ref_ptr>
#include <osg/Camera>
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

    struct Attachment : public osg::Referenced {
        Attachment(SGPropertyNode* prop);
        Attachment(osg::Camera::BufferComponent c, const std::string& b ) : component(c), buffer(b) {}

        osg::Camera::BufferComponent component;
        std::string buffer;
    };
    typedef std::vector<osg::ref_ptr<Attachment> > AttachmentList;

    struct Stage : public osg::Referenced {
        Stage(SGPropertyNode* prop);

        std::string name;
        std::string type;
        int orderNum;
        std::string effect;
        bool needsDuDv;
        float scaleFactor;

        std::vector<osg::ref_ptr<Pass> > passes;
        AttachmentList attachments;
    };
    FGRenderingPipeline();

    std::vector<osg::ref_ptr<Buffer> > buffers;
    std::vector<osg::ref_ptr<Stage> > stages;

    friend FGRenderingPipeline* flightgear::makeRenderingPipeline(const std::string& name,
                   const simgear::SGReaderWriterOptions* options);
};

#endif


#ifndef __FG_RENDERINGPIPELINE_HXX
#define __FG_RENDERINGPIPELINE_HXX 1

#include <osg/ref_ptr>
#include <osg/Camera>
#include <string>

#include <simgear/structure/SGExpression.hxx>

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
    class Conditionable : public osg::Referenced {
    public:
        Conditionable() : _alwaysValid(true) {}
        void parseCondition(SGPropertyNode* prop);
        bool getAlwaysValid() const { return _alwaysValid; }
        void setAlwaysValid(bool val) { _alwaysValid = val; }
        void setValidExpression(SGExpressionb* exp);
        bool valid();
    protected:
        bool _alwaysValid;
        SGSharedPtr<SGExpressionb> _validExpression;
    };
    struct Buffer : public Conditionable {
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

    struct Pass : public Conditionable {
        Pass(SGPropertyNode* prop);

        std::string name;
        std::string type;
        int orderNum;
        std::string effect;
        std::string debugProperty;
    };

    struct Attachment : public Conditionable {
        Attachment(SGPropertyNode* prop);
        Attachment(osg::Camera::BufferComponent c, const std::string& b ) : component(c), buffer(b) {}

        osg::Camera::BufferComponent component;
        std::string buffer;
    };
    typedef std::vector<osg::ref_ptr<Attachment> > AttachmentList;

    struct Stage : public Pass {
        Stage(SGPropertyNode* prop);

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

namespace flightgear {

class PipelinePredParser : public simgear::expression::ExpressionParser
{
public:
protected:
};
}

#endif

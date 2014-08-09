#ifndef FG_PANELNODE_HXX
#define FG_PANELNODE_HXX

#include <osg/Vec3>
#include <osg/Matrix>
#include <osg/Drawable>
#include <osg/Version>

#include <memory>
#include <simgear/structure/SGSharedPtr.hxx>
#include <simgear/props/props.hxx>

class FGPanel;
class SGPropertyNode;

// PanelNode defines an OSG drawable wrapping the 2D panel drawing code

class FGPanelNode : public osg::Drawable
{
public:  
    FGPanelNode(SGPropertyNode* props);
    virtual ~FGPanelNode();

    // OSGFIXME
    virtual osg::Object* cloneType() const { return 0; }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return 0; }        

    FGPanel* getPanel() { return _panel; }

    virtual void drawImplementation(osg::RenderInfo& renderInfo) const
    { drawImplementation(*renderInfo.getState()); }
  
    void drawImplementation(osg::State& state) const;

    virtual osg::BoundingBox
#if OSG_VERSION_LESS_THAN(3,3,2)
    computeBound()
#else
    computeBoundingBox()
#endif
    const;


    /** Return true, FGPanelNode does support accept(PrimitiveFunctor&). */
    virtual bool supports(const osg::PrimitiveFunctor&) const { return true; }
  
    virtual void accept(osg::PrimitiveFunctor& functor) const;
  
    static osg::Node* load(SGPropertyNode *n);
    static osg::Node* create2DPanelNode();
  
    osg::Matrix transformMatrix() const;
  
    void setPanelPath(const std::string& panel);
    void lazyLoad();
  
    /**
      * is visible in 2D mode or not?
      */
    bool isVisible2d() const;
private:
    FGPanelNode(); // for 2D panels
    
    void commonInit();
    void initWithPanel();
    
    bool panelMouseClickCommand(const SGPropertyNode * arg);
    
    const bool _is2d;
    SGSharedPtr<FGPanel> _panel;
    std::string _panelPath;
  
    bool _resizeToViewport;
    bool _depthTest;
  
    // Panel corner coordinates
    osg::Vec3 _bottomLeft, _topLeft, _bottomRight;

    int _xmin, _ymin, _xmax, _ymax;
  
    // The matrix that results, which transforms 2D x/y panel
    // coordinates into 3D coordinates of the panel quadrilateral.
    osg::Matrix _xform;  
  
    SGPropertyChangeListener* _listener;
    std::auto_ptr<SGPropertyChangeListener> _pathListener;
    
    /// should the 2D panel auto-hide when the view orientation changes
    mutable SGPropertyNode_ptr _autoHide2d;
    
    /// should the 2D panel be hidden in views other than the default (view 0)
    mutable SGPropertyNode_ptr _hideNonDefaultViews;
};


#endif // FG_PANELNODE_HXX

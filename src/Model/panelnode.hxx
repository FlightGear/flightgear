#ifndef FG_PANELNODE_HXX
#define FG_PANELNODE_HXX

#include <osg/Vec3>
#include <osg/Matrix>
#include <osg/Drawable>
#include <simgear/structure/SGSharedPtr.hxx>

class FGPanel;
class SGPropertyNode;

// PanelNode defines an OSG drawable wrapping the 2D panel drawing code

class FGPanelNode : public osg::Drawable
{
public:
    FGPanelNode(FGPanel* panel);
  
    FGPanelNode(SGPropertyNode* props);
    virtual ~FGPanelNode();

    // OSGFIXME
    virtual osg::Object* cloneType() const { return 0; }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return 0; }        

    FGPanel* getPanel() { return _panel; }

    virtual void drawImplementation(osg::RenderInfo& renderInfo) const
    { drawImplementation(*renderInfo.getState()); }
  
    void drawImplementation(osg::State& state) const;
    virtual osg::BoundingBox computeBound() const;

    /** Return true, FGPanelNode does support accept(PrimitiveFunctor&). */
    virtual bool supports(const osg::PrimitiveFunctor&) const { return true; }
  
    virtual void accept(osg::PrimitiveFunctor& functor) const;
  
    static osg::Node* load(SGPropertyNode *n);
    static osg::Node* createNode(FGPanel* panel);
  
    osg::Matrix transformMatrix() const;
  
private:
    void initWithPanel();
    
    SGSharedPtr<FGPanel> _panel;
  
    bool _resizeToViewport;

    // Panel corner coordinates
    osg::Vec3 _bottomLeft, _topLeft, _bottomRight;

    int _xmin, _ymin, _xmax, _ymax;
  
    // The matrix that results, which transforms 2D x/y panel
    // coordinates into 3D coordinates of the panel quadrilateral.
    osg::Matrix _xform;  
};


#endif // FG_PANELNODE_HXX

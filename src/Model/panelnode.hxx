#ifndef FG_PANELNODE_HXX
#define FG_PANELNODE_HXX

#include <osg/Vec3>
#include <osg/Matrix>
#include <osg/Drawable>

class FGPanel;
class SGPropertyNode;

// PanelNode defines an SSG leaf object that draws a FGPanel object
// into the scene graph.  Note that this is an incomplete SSG object,
// many methods, mostly involved with modelling and runtime
// inspection, are unimplemented.

// Static mouse handler for all FGPanelNodes.  Very clumsy; this
// should really be done through our container (an aircraft model,
// typically).
bool fgHandle3DPanelMouseEvent(int button, int updown, int x, int y);
void fgUpdate3DPanels();

class FGPanelNode : public osg::Drawable // OSGFIXME 
{
public:
    FGPanelNode(SGPropertyNode* props);
    virtual ~FGPanelNode();

    // OSGFIXME
    virtual osg::Object* cloneType() const { return 0; }
    virtual osg::Object* clone(const osg::CopyOp& copyop) const { return 0; }        

    bool doMouseAction(int button, int updown, int x, int y);

    FGPanel* getPanel() { return _panel; }

    virtual void drawImplementation(osg::State& state) const;
    virtual osg::BoundingBox computeBound() const;

private:
    FGPanel* _panel;

    // Panel corner coordinates
    osg::Vec3 _bottomLeft, _topLeft, _bottomRight;

    // The input range expected in the panel definition.  These x/y
    // coordinates will map to the right/top sides.
    float _xmax, _ymax;
    
    // The matrix that results, which transforms 2D x/y panel
    // coordinates into 3D coordinates of the panel quadrilateral.
    osg::Matrix _xform;

    // The matrix transformation state that was active the last time
    // we were rendered.  Used by the mouse code to compute
    // intersections.
    osg::Matrix _lastModelview;
    osg::Matrix _lastProjection;
    int   _lastViewport[4];
};


#endif // FG_PANELNODE_HXX

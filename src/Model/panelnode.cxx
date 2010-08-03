#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>

#include <vector>
#include <algorithm>

#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include "panelnode.hxx"

using std::vector;


// Static (!) handling for all 3D panels in the program.
// OSGFIXME: Put the panel as different elements in the scenegraph.
// Then just pick in that scenegraph.
vector<FGPanelNode*> all_3d_panels;
bool fgHandle3DPanelMouseEvent( int button, int updown, int x, int y )
{
    for ( unsigned int i = 0; i < all_3d_panels.size(); i++ ) {
        if ( all_3d_panels[i]->doMouseAction(button, updown, x, y) ) {
            return true;
        }
    }
    return false;
}

void fgUpdate3DPanels()
{
    for ( unsigned int i = 0; i < all_3d_panels.size(); i++ ) {
        all_3d_panels[i]->getPanel()->updateMouseDelay();
    }
}

FGPanelNode::FGPanelNode(SGPropertyNode* props)
{
    int i;

    // Make an FGPanel object.  But *don't* call init() or bind() on
    // it -- those methods touch static state.
    const char *path = props->getStringValue("path");
    _panel = fgReadPanel(path);
    if (!_panel)
        throw sg_io_exception(string("Failed to load panel ") + path);

    // Never mind.  We *have* to call init to make sure the static
    // state is initialized (it's not, if there aren't any 2D
    // panels).  This is a memory leak and should be fixed!`
    // FIXME
    _panel->init();

    _panel->setDepthTest( props->getBoolValue("depth-test") );

    // Read out the pixel-space info
    _xmax = _panel->getWidth();
    _ymax = _panel->getHeight();

    // And the corner points
    SGPropertyNode* pt = props->getChild("bottom-left");
    _bottomLeft[0] = pt->getFloatValue("x-m");
    _bottomLeft[1] = pt->getFloatValue("y-m");
    _bottomLeft[2] = pt->getFloatValue("z-m");

    pt = props->getChild("top-left");
    _topLeft[0] = pt->getFloatValue("x-m");
    _topLeft[1] = pt->getFloatValue("y-m");
    _topLeft[2] = pt->getFloatValue("z-m");

    pt = props->getChild("bottom-right");
    _bottomRight[0] = pt->getFloatValue("x-m");
    _bottomRight[1] = pt->getFloatValue("y-m");
    _bottomRight[2] = pt->getFloatValue("z-m");

    // Now generate our transformation matrix.  For shorthand, use
    // "a", "b", and "c" as our corners and "m" as the matrix. The
    // vector u goes from a to b, v from a to c, and w is a
    // perpendicular cross product.
    osg::Vec3 a = _bottomLeft;
    osg::Vec3 b = _bottomRight;
    osg::Vec3 c = _topLeft;
    osg::Vec3 u = b - a;
    osg::Vec3 v = c - a;
    osg::Vec3 w = u^v;

    osg::Matrix& m = _xform;
    // Now generate a trivial basis transformation matrix.  If we want
    // to map the three unit vectors to three arbitrary vectors U, V,
    // and W, then those just become the columns of the 3x3 matrix.
    m(0,0) = u[0]; m(1,0) = v[0]; m(2,0) = w[0]; m(3,0) = a[0];//    |Ux Vx Wx|
    m(0,1) = u[1]; m(1,1) = v[1]; m(2,1) = w[1]; m(3,1) = a[1];//m = |Uy Vy Wy|
    m(0,2) = u[2]; m(1,2) = v[2]; m(2,2) = w[2]; m(3,2) = a[2];//    |Uz Vz Wz|
    m(0,3) = 0;    m(1,3) = 0;    m(2,3) = 0;    m(3,3) = 1;

    // The above matrix maps the unit (!) square to the panel
    // rectangle.  Postmultiply scaling factors that match the
    // pixel-space size of the panel.
    for(i=0; i<4; ++i) {
        m(0,i) *= 1.0/_xmax;
        m(1,i) *= 1.0/_ymax;
    }

    _lastViewport[0] = 0;
    _lastViewport[1] = 0;
    _lastViewport[2] = 0;
    _lastViewport[3] = 0;

    dirtyBound();

    // All done.  Add us to the list
    all_3d_panels.push_back(this);

    setUseDisplayList(false);
    getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
}

FGPanelNode::~FGPanelNode()
{
    vector<FGPanelNode*>::iterator i =
      find(all_3d_panels.begin(), all_3d_panels.end(), this);
    if (i != all_3d_panels.end()) {
        all_3d_panels.erase(i);
    }
    delete _panel;
}

void
FGPanelNode::drawImplementation(osg::State& state) const
{
  osg::ref_ptr<osg::RefMatrix> mv = new osg::RefMatrix;
  mv->set(_xform*state.getModelViewMatrix());
  state.applyModelViewMatrix(mv.get());
  
  // Grab the matrix state, so that we can get back from screen
  // coordinates to panel coordinates when the user clicks the
  // mouse.
  // OSGFIXME: we don't need that when we can really pick
  _lastModelview = state.getModelViewMatrix();
  _lastProjection = state.getProjectionMatrix();

  const osg::Viewport* vp = state.getCurrentViewport();
  _lastViewport[0] = vp->x();
  _lastViewport[1] = vp->y();
  _lastViewport[2] = vp->width();
  _lastViewport[3] = vp->height();
  
  _panel->draw(state);
}

osg::BoundingBox
FGPanelNode::computeBound() const
{
    osg::BoundingBox bb;
    bb.expandBy(_bottomLeft);
    bb.expandBy(_bottomRight);
    bb.expandBy(_topLeft);
    return bb;
}

bool FGPanelNode::doMouseAction(int button, int updown, int x, int y)
{
    if (_lastViewport[2] == 0 || _lastViewport[3] == 0) {
        // we haven't been drawn yet, presumably
        return false;
    }

    // Covert the screen coordinates to viewport coordinates in the
    // range [0:1], then transform to OpenGL "post projection" coords
    // in [-1:1].  Remember the difference in Y direction!
    float vx = (x + 0.5 - _lastViewport[0]) / _lastViewport[2];
    float vy = (y + 0.5 - _lastViewport[1]) / _lastViewport[3];
    vx = 2*vx - 1;
    vy = 1 - 2*vy;

    // Make two vectors in post-projection coordinates at the given
    // screen, one in the near field and one in the far field.
    osg::Vec3 a, b;
    a[0] = b[0] = vx;
    a[1] = b[1] = vy;
    a[2] =  0.75; // "Near" Z value
    b[2] = -0.75; // "Far" Z value

    // Run both vectors "backwards" through the OpenGL matrix
    // transformation.  Remember to w-normalize the vectors!
    osg::Matrix m = _lastModelview*_lastProjection;
    m = osg::Matrix::inverse(m);

    a = m.preMult(a);
    b = m.preMult(b);

    // And find their intersection on the z=0 plane.  The resulting X
    // and Y coordinates are the hit location in panel coordinates.
    float dxdz = (b[0] - a[0]) / (b[2] - a[2]);
    float dydz = (b[1] - a[1]) / (b[2] - a[2]);
    int panelX = (int)(a[0] - a[2]*dxdz + 0.5);
    int panelY = (int)(a[1] - a[2]*dydz + 0.5);

    return _panel->doLocalMouseAction(button, updown, panelX, panelY);
}


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "panelnode.hxx"

#include <vector>
#include <algorithm>

#include <osg/Geode>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include <Main/fg_os.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>


using std::vector;

class FGPanelPickCallback : public SGPickCallback {
public:
  FGPanelPickCallback(FGPanelNode* p) :
    panel(p)
  {}
  
  virtual bool buttonPressed(int b, const Info& info)
  {    
    button = b;
  // convert to panel coordinates
    osg::Matrixd m = osg::Matrixd::inverse(panel->transformMatrix());
    picked = toOsg(info.local) * m;
    SG_LOG( SG_INSTR, SG_DEBUG, "panel pick: " << toSG(picked) );
  
  // send to the panel
    return panel->getPanel()->doLocalMouseAction(button, MOUSE_BUTTON_DOWN, 
                                          picked.x(), picked.y());
  }
  
  virtual void update(double /* dt */)
  {
    panel->getPanel()->updateMouseDelay();
  }
  
  virtual void buttonReleased(void)
  {
    panel->getPanel()->doLocalMouseAction(button, MOUSE_BUTTON_UP, 
                                          picked.x(), picked.y());
  }
  
private:
  FGPanelNode* panel;
  int button;
  osg::Vec3 picked;
};

FGPanelNode::FGPanelNode(SGPropertyNode* props) :
  _resizeToViewport(false)
{  
    // Make an FGPanel object.  But *don't* call init() or bind() on
    // it -- those methods touch static state.
    const char *path = props->getStringValue("path");
    _panel = fgReadPanel(path);
    if (!_panel)
        throw sg_io_exception(string("Failed to load panel ") + path);

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
  
    _panel->setDepthTest( props->getBoolValue("depth-test") );
  
    initWithPanel();
}

FGPanelNode::FGPanelNode(FGPanel* p) :
  _panel(p),
  _resizeToViewport(true)
{
    initWithPanel();
}

void FGPanelNode::initWithPanel()
{
    int i;

    // Never mind.  We *have* to call init to make sure the static
    // state is initialized (it's not, if there aren't any 2D
    // panels).  This is a memory leak and should be fixed!`
    // FIXME
    _panel->init();

    // Read out the pixel-space info
    float panelWidth = _panel->getWidth();
    float panelHeight = _panel->getHeight();

    _panel->getLogicalExtent(_xmin, _ymin, _xmax, _ymax);
  
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
        m(0,i) *= 1.0/panelWidth;
        m(1,i) *= 1.0/panelHeight;
    }

    dirtyBound();
    setUseDisplayList(false);
    setDataVariance(Object::DYNAMIC);

    getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
}

FGPanelNode::~FGPanelNode()
{
    delete _panel;
}

osg::Matrix FGPanelNode::transformMatrix() const
{
  if (!_resizeToViewport) {
    return _xform;
  }
  
  double s = _panel->getAspectScale();
  osg::Matrix m = osg::Matrix::scale(s, s, 1.0);
  m *= osg::Matrix::translate(_panel->getXOffset(), _panel->getYOffset(), 0.0);

  return m;
}

void
FGPanelNode::drawImplementation(osg::State& state) const
{  
  osg::ref_ptr<osg::RefMatrix> mv = new osg::RefMatrix;
  mv->set(transformMatrix() * state.getModelViewMatrix());
  state.applyModelViewMatrix(mv.get());
  
  _panel->draw(state);
}

osg::BoundingBox
FGPanelNode::computeBound() const
{

  osg::Vec3 coords[3];
  osg::Matrix m(transformMatrix());
  coords[0] = m.preMult(osg::Vec3(_xmin,_ymin,0));
  coords[1] = m.preMult(osg::Vec3(_xmax,_ymin,0));
  coords[2] = m.preMult(osg::Vec3(_xmin,_ymax,0));

  osg::BoundingBox bb;
  bb.expandBy(coords[0]);
  bb.expandBy(coords[1]);
  bb.expandBy(coords[2]);
  return bb;
}

void FGPanelNode::accept(osg::PrimitiveFunctor& functor) const
{
  osg::Vec3 coords[4];
  osg::Matrix m(transformMatrix());
  
  coords[0] = m.preMult(osg::Vec3(_xmin,_ymin,0));
  coords[1] = m.preMult(osg::Vec3(_xmax,_ymin,0));
  coords[2] = m.preMult(osg::Vec3(_xmax, _ymax, 0));
  coords[3] = m.preMult(osg::Vec3(_xmin,_ymax,0));

  functor.setVertexArray(4, coords);
  functor.drawArrays( GL_QUADS, 0, 4);
}

static osg::Node* createGeode(FGPanelNode* panel)
{
    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(panel);
    
    geode->setNodeMask(SG_NODEMASK_PICK_BIT | SG_NODEMASK_2DPANEL_BIT);
    
    SGSceneUserData* userData;
    userData = SGSceneUserData::getOrCreateSceneUserData(geode);
    userData->setPickCallback(new FGPanelPickCallback(panel));
    return geode;
}

osg::Node* FGPanelNode::createNode(FGPanel* p)
{
    FGPanelNode* drawable = new FGPanelNode(p);
    return createGeode(drawable);
}

osg::Node* FGPanelNode::load(SGPropertyNode *n)
{
  FGPanelNode* drawable = new FGPanelNode(n);
  return createGeode(drawable);
}

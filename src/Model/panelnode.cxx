#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "panelnode.hxx"

#include <vector>
#include <algorithm>

#include <osg/Geode>
#include <osg/Switch>
#include <osg/BlendFunc>

#include <simgear/compiler.h>
#include <simgear/structure/exception.hxx>
#include <simgear/debug/logstream.hxx>

#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/scene/util/SGPickCallback.hxx>
#include <simgear/scene/util/SGSceneUserData.hxx>
#include <simgear/scene/util/SGNodeMasks.hxx>

#include <GUI/FlightGear_pu.h>
#include <Main/fg_os.hxx>
#include <Cockpit/panel.hxx>
#include <Cockpit/panel_io.hxx>
#include "Viewer/view.hxx"
#include "Viewer/viewmgr.hxx"

using std::vector;

class PanelTransformListener : public SGPropertyChangeListener
{
public:
    PanelTransformListener(FGPanelNode* pn) : _panelNode(pn) {}
    
    virtual void valueChanged (SGPropertyNode * node)
    {
        _panelNode->dirtyBound();
    }
private:
    FGPanelNode* _panelNode;
};

class PanelPathListener : public SGPropertyChangeListener
{
public:
    PanelPathListener(FGPanelNode* pn) : _panelNode(pn) {}
    
    virtual void valueChanged (SGPropertyNode * node)
    {
        _panelNode->setPanelPath(node->getStringValue());
    }
private:
    FGPanelNode* _panelNode;
};

class FGPanelPickCallback : public SGPickCallback {
public:
  FGPanelPickCallback(FGPanelNode* p) :
    panel(p)
  {}
  
  virtual bool buttonPressed( int b,
                              const osgGA::GUIEventAdapter&,
                              const Info& info )
  {
      if (!panel->getPanel()) {
          return false;
      }
      
    button = b;
  // convert to panel coordinates
    osg::Matrixd m = osg::Matrixd::inverse(panel->transformMatrix());
    picked = toOsg(info.local) * m;
    SG_LOG( SG_INSTR, SG_DEBUG, "panel pick: " << toSG(picked) );
  
  // send to the panel
    return panel->getPanel()->doLocalMouseAction(button, MOUSE_BUTTON_DOWN, 
                                          picked.x(), picked.y());
  }
  
  virtual void update(double dt, int keyModState)
  {
    panel->getPanel()->updateMouseDelay(dt);
  }
  
  virtual void buttonReleased( int,
                               const osgGA::GUIEventAdapter&,
                               const Info* )
  {
    panel->getPanel()->doLocalMouseAction(button, MOUSE_BUTTON_UP, 
                                          picked.x(), picked.y());
  }
  
private:
  FGPanelNode* panel;
  int button;
  osg::Vec3 picked;
};

class FGPanelSwitchCallback : public osg::NodeCallback {
public:
  FGPanelSwitchCallback(FGPanelNode* pn) : 
    panel(pn),
    visProp(fgGetNode("/sim/panel/visibility"))
  {
  }
  
  virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
  {
    assert(dynamic_cast<osg::Switch*>(node));
    osg::Switch* sw = static_cast<osg::Switch*>(node);
    
    if (!visProp->getBoolValue()) {
      sw->setValue(0, false);
      return;
    }
  
    
    panel->lazyLoad(); // isVisible check needs the panel loaded for auto-hide flag
    bool enabled = panel->isVisible2d();
    sw->setValue(0, enabled);
    if (!enabled)
      return;
    
    traverse(node, nv);
  }
  
private:
  FGPanelNode* panel;
  SGPropertyNode_ptr visProp;
};


FGPanelNode::FGPanelNode(SGPropertyNode* props) :
    _is2d(false),
    _resizeToViewport(false),
    _listener(NULL)
{  
  commonInit();
  _panelPath = props->getStringValue("path");

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
  
  _depthTest = props->getBoolValue("depth-test");
}

FGPanelNode::FGPanelNode() :
    _is2d(true),
  _resizeToViewport(true),
  _depthTest(false)
{
    globals->get_commands()->addCommand("panel-mouse-click", this, &FGPanelNode::panelMouseClickCommand);

    SGPropertyNode* pathNode = fgGetNode("/sim/panel/path");
    _pathListener.reset(new PanelPathListener(this));
    pathNode->addChangeListener(_pathListener.get());
    setPanelPath(pathNode->getStringValue());
    
    // for a 2D panel, various options adjust the transformation
    // matrix. We need to pass this data on to OSG or its bounding box
    // will be stale, and picking will break.
    // http://code.google.com/p/flightgear-bugs/issues/detail?id=864
    _listener = new PanelTransformListener(this);
    fgGetNode("/sim/panel/x-offset", true)->addChangeListener(_listener);
    fgGetNode("/sim/panel/y-offset", true)->addChangeListener(_listener);
    fgGetNode("/sim/startup/xsize", true)->addChangeListener(_listener);
    fgGetNode("/sim/startup/ysize", true)->addChangeListener(_listener);
    
    commonInit();
}

FGPanelNode::~FGPanelNode()
{
    if (_is2d) {
        globals->get_commands()->removeCommand("panel-mouse-click");
        SGPropertyNode* pathNode = fgGetNode("/sim/panel/path");
        pathNode->removeChangeListener(_pathListener.get());
    }
    
    if (_listener) {
        fgGetNode("/sim/panel/x-offset", true)->removeChangeListener(_listener);
        fgGetNode("/sim/panel/y-offset", true)->removeChangeListener(_listener);
        fgGetNode("/sim/startup/xsize", true)->removeChangeListener(_listener);
        fgGetNode("/sim/startup/ysize", true)->removeChangeListener(_listener);
        delete _listener;
    }
}

void FGPanelNode::setPanelPath(const std::string& panel)
{
  if (panel == _panelPath) {
    return;
  }
  
  _panelPath = panel;
  if (_panel) {
    _panel.clear();
  }
}

void FGPanelNode::lazyLoad()
{
  if (!_panelPath.empty() && !_panel) {
    _panel = fgReadPanel(_panelPath);
    if (!_panel) {
      SG_LOG(SG_COCKPIT, SG_WARN, "failed to read panel from:" << _panelPath);
      _panelPath = std::string(); // don't keep trying to read
      return;
    }
    
    _panel->setDepthTest(_depthTest);
    initWithPanel();
  }
}

void FGPanelNode::commonInit()
{
  setUseDisplayList(false);
  setDataVariance(Object::DYNAMIC);
  getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
  getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
}

void FGPanelNode::initWithPanel()
{
  int i;

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
    if ((u.length2() == 0.0) || (b.length2() == 0.0)) {
        m.makeIdentity();
    } else {
        // Now generate a trivial basis transformation matrix.  If we want
        // to map the three unit vectors to three arbitrary vectors U, V,
        // and W, then those just become the columns of the 3x3 matrix.
        m(0,0) = u[0]; m(1,0) = v[0]; m(2,0) = w[0]; m(3,0) = a[0];//    |Ux Vx Wx|
        m(0,1) = u[1]; m(1,1) = v[1]; m(2,1) = w[1]; m(3,1) = a[1];//m = |Uy Vy Wy|
        m(0,2) = u[2]; m(1,2) = v[2]; m(2,2) = w[2]; m(3,2) = a[2];//    |Uz Vz Wz|
        m(0,3) = 0;    m(1,3) = 0;    m(2,3) = 0;    m(3,3) = 1;
    }

  // The above matrix maps the unit (!) square to the panel
  // rectangle.  Postmultiply scaling factors that match the
  // pixel-space size of the panel.
  for(i=0; i<4; ++i) {
      m(0,i) *= 1.0/panelWidth;
      m(1,i) *= 1.0/panelHeight;
  }

  dirtyBound();
}

osg::Matrix FGPanelNode::transformMatrix() const
{
  if (!_panel) {
    return osg::Matrix();
  }

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
  if (!_panel) {
    return;
  }
  
  osg::ref_ptr<osg::RefMatrix> mv = new osg::RefMatrix;
  mv->set(transformMatrix() * state.getModelViewMatrix());
  state.applyModelViewMatrix(mv.get());
  
  _panel->draw(state);
}

osg::BoundingBox FGPanelNode::computeBoundingBox() const
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

bool FGPanelNode::isVisible2d() const
{
  if (!_panel) {
    return false;
  }
  
  if (!_hideNonDefaultViews) {
    _hideNonDefaultViews = fgGetNode("/sim/panel/hide-nonzero-view", true);
  }
  
  if (_hideNonDefaultViews->getBoolValue()) {
      if (globals->get_viewmgr()->getCurrentViewIndex() != 0) {
          return false;
      }
  }
  
  if (!_autoHide2d) {
    _autoHide2d = fgGetNode("/sim/panel/hide-nonzero-heading-offset", true);
  }
  
  if (_panel->getAutohide() && _autoHide2d->getBoolValue()) {
    if (!globals->get_current_view()) {
      return false;
    }
    
    return globals->get_current_view()->getHeadingOffset_deg() == 0;
  }
  
  return true;
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

osg::Node* FGPanelNode::create2DPanelNode()
{
  FGPanelNode* drawable = new FGPanelNode;
    
  osg::Switch* ps = new osg::Switch;
  osg::StateSet* stateSet = ps->getOrCreateStateSet();
  stateSet->setRenderBinDetails(1000, "RenderBin");
  ps->addChild(createGeode(drawable));
  
  // speed optimization?
  stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
  stateSet->setAttribute(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
  stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
  stateSet->setMode(GL_FOG, osg::StateAttribute::OFF);
  stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
  
  ps->setUpdateCallback(new FGPanelSwitchCallback(drawable));
  return ps;
}

osg::Node* FGPanelNode::load(SGPropertyNode *n)
{
  FGPanelNode* drawable = new FGPanelNode(n);
  drawable->lazyLoad(); // force load now for 2.5D panels
  return createGeode(drawable);
}

/**
 * Built-in command: pass a mouse click to the panel.
 *
 * button: the mouse button number, zero-based.
 * is-down: true if the button is down, false if it is up.
 * x-pos: the x position of the mouse click.
 * y-pos: the y position of the mouse click.
 */
bool
FGPanelNode::panelMouseClickCommand(const SGPropertyNode * arg, SGPropertyNode * root)
{
    return _panel->doMouseAction(arg->getIntValue("button"),
                         arg->getBoolValue("is-down") ? PU_DOWN : PU_UP,
                         arg->getIntValue("x-pos"),
                         arg->getIntValue("y-pos"));
}

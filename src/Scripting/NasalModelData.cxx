
#include "NasalModelData.hxx"
#include "NasalSys.hxx"
#include <Main/fg_props.hxx>
#include <Main/globals.hxx>

#include <osg/Transform>
#include <osg/observer_ptr>

#include <simgear/math/SGMath.hxx>
#include <simgear/nasal/cppbind/Ghost.hxx>
#include <simgear/scene/util/OsgDebug.hxx>
#include <simgear/scene/util/OsgMath.hxx>
#include <simgear/debug/logstream.hxx>

#include <algorithm>
#include <cstring> // for strlen

// FGNasalModelData class.  If sgLoad3DModel() is called with a pointer to
// such a class, then it lets modelLoaded() run the <load> script, and the
// destructor the <unload> script. The latter happens when the model branch
// is removed from the scene graph.

typedef osg::ref_ptr<osg::Node> NodeRef;
typedef nasal::Ghost<NodeRef> NasalNode;

/**
 * Get position (lat, lon, elevation) and orientation (heading, pitch, roll) of
 * model.
 */
static naRef f_node_getPose( const osg::Node& node,
                             const nasal::CallContext& ctx )
{
  osg::NodePathList parent_paths = node.getParentalNodePaths();
  for( osg::NodePathList::const_iterator path = parent_paths.begin();
                                         path != parent_paths.end();
                                       ++path )
  {
    osg::Matrix local_to_world = osg::computeLocalToWorld(*path);
    if( !local_to_world.valid() )
      continue;

    SGGeod coord = SGGeod::fromCart( toSG(local_to_world.getTrans()) );
    if( !coord.isValid() )
      continue;

    osg::Matrix local_frame = makeZUpFrameRelative(coord),
                inv_local;
    inv_local.invert_4x3(local_frame);
    local_to_world.postMult(inv_local);

    SGQuatd rotate = toSG(local_to_world.getRotate());
    double hdg, pitch, roll;
    rotate.getEulerDeg(hdg, pitch, roll);

    nasal::Hash pose(ctx.to_nasal(coord), ctx.c_ctx());
    pose.set("heading", hdg);
    pose.set("pitch", pitch);
    pose.set("roll", roll);
    return pose.get_naRef();
  }

  return naNil();
}

//------------------------------------------------------------------------------
FGNasalModelData::FGNasalModelData( SGPropertyNode *root,
                                    const std::string& path,
                                    SGPropertyNode *prop,
                                    SGPropertyNode* load,
                                    SGPropertyNode* unload,
                                    osg::Node* branch ):
  _path(path),
  _root(root), _prop(prop),
  _load(load), _unload(unload),
  _branch(branch)
{
  const std::lock_guard<std::mutex> lock(FGNasalModelData::_loaded_models_mutex);
  _module_id = _max_module_id++;
  _loaded_models.push_back(this);

  SG_LOG
  (
    SG_NASAL,
    SG_INFO,
    "New model with attached script(s) "
    "(branch = " << branch << ","
    " path = " << simgear::getNodePathString(branch) << ")"
  );
}

//------------------------------------------------------------------------------
FGNasalModelData::~FGNasalModelData()
{
  const std::lock_guard<std::mutex> lock(FGNasalModelData::_loaded_models_mutex);
  _loaded_models.remove(this);

  SG_LOG
  (
    SG_NASAL,
    SG_INFO,
    "Removed model with script(s) (branch = " << _branch.get() << ")"
  );
}

//------------------------------------------------------------------------------
void FGNasalModelData::load()
{
  std::stringstream m;
  m << "__model" << _module_id;
  _module = m.str();

  SG_LOG(SG_NASAL, SG_DEBUG, "Loading nasal module " << _module.c_str());

  const std::string s = _load ? _load->getStringValue() : "";
  auto nasalSys = globals->get_subsystem<FGNasalSys>();

  // Add _module_id to script local hash to allow placing canvasses on objects
  // inside the model.
  nasal::Hash module = nasalSys->getGlobals().createHash(_module);
  module.set("_module_id", _module_id);

  NasalNode::init("osg.Node")
      .method("getPose", &f_node_getPose);
  module.set("_model", _branch);

  naRef arg[2];
  arg[0] = nasalSys->propNodeGhost(_root);
  arg[1] = nasalSys->propNodeGhost(_prop);
  nasalSys->createModule(_module.c_str(), _path.c_str(), s.c_str(), s.length(),
                         _root, 2, arg);
}

//------------------------------------------------------------------------------
void FGNasalModelData::unload()
{
    if (_module.empty())
        return;

    auto nasalSys = globals->get_subsystem<FGNasalSys>();
    if(!nasalSys) {
        SG_LOG(SG_NASAL, SG_WARN, "Trying to run an <unload> script "
               "without Nasal subsystem present.");
        return;
    }

    SG_LOG(SG_NASAL, SG_DEBUG, "Unloading nasal module " << _module.c_str());

    if (_unload)
    {
        const std::string s = _unload->getStringValue();
        nasalSys->createModule(_module.c_str(), _module.c_str(), s.c_str(), s.length(), _root);
    }

    nasalSys->deleteModule(_module.c_str());
}

//------------------------------------------------------------------------------
osg::Node* FGNasalModelData::getNode()
{
  return _branch.get();
}

//------------------------------------------------------------------------------
FGNasalModelData* FGNasalModelData::getByModuleId(unsigned int id)
{
  const std::lock_guard<std::mutex> lock(FGNasalModelData::_loaded_models_mutex);
  FGNasalModelDataList::iterator it = std::find_if
  (
    _loaded_models.begin(),
    _loaded_models.end(),
    [id] (const FGNasalModelData* const data) {
        return data->_module_id == id; });

  if( it != _loaded_models.end() )
    return *it;

  return 0;
}

//------------------------------------------------------------------------------
FGNasalModelDataProxy::~FGNasalModelDataProxy()
{
    if (globals) {
      auto nasalSys = globals->get_subsystem<FGNasalSys>();

      // when necessary, register Nasal module to be destroyed/unloaded
      // in the main thread.
      if ((_data.valid())&&(nasalSys))
          nasalSys->registerToUnload(_data);
    }
}

//------------------------------------------------------------------------------
void FGNasalModelDataProxy::modelLoaded( const std::string& path,
                                         SGPropertyNode *prop,
                                         osg::Node *branch )
{
    if( fgGetBool("/sim/disable-embedded-nasal") )
        return;

    if(!prop)
        return;
    
    SGPropertyNode *nasal = prop->getNode("nasal");
    if(!nasal)
        return;
    
    auto nasalSys = globals->get_subsystem<FGNasalSys>();
    if(!nasalSys)
    {
        SG_LOG
        (
          SG_NASAL,
          SG_WARN,
          "Can not load model script(s) (Nasal subsystem not available)."
        );
        return;
    }

    SGPropertyNode* load   = nasal->getNode("load");
    SGPropertyNode* unload = nasal->getNode("unload");
    
    if ((!load) && (!unload))
        return;

    _data = new FGNasalModelData(_root, path, prop, load, unload, branch);
    
    // register Nasal module to be created and loaded in the main thread.
    nasalSys->registerToLoad(_data);
}

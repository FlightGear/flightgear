
#include "NasalModelData.hxx"
#include "NasalSys.hxx"
#include <Main/globals.hxx>

#include <simgear/debug/logstream.hxx>

#include <boost/bind.hpp>

#include <algorithm>
#include <cstring> // for strlen

// FGNasalModelData class.  If sgLoad3DModel() is called with a pointer to
// such a class, then it lets modelLoaded() run the <load> script, and the
// destructor the <unload> script. The latter happens when the model branch
// is removed from the scene graph.

unsigned int FGNasalModelData::_max_module_id = 0;
FGNasalModelDataList FGNasalModelData::_loaded_models;

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
  _branch(branch),
  _module_id( _max_module_id++ )
{
  _loaded_models.push_back(this);
}

//------------------------------------------------------------------------------
FGNasalModelData::~FGNasalModelData()
{
  _loaded_models.remove(this);
}

//------------------------------------------------------------------------------
void FGNasalModelData::load()
{
    std::stringstream m;
    m << "__model" << _module_id;
    _module = m.str();
    
    SG_LOG(SG_NASAL, SG_DEBUG, "Loading nasal module " << _module.c_str());
    
    const char *s = _load ? _load->getStringValue() : "";
    FGNasalSys* nasalSys = (FGNasalSys*) globals->get_subsystem("nasal");
    
    // Add _module_id to script local hash to allow placing canvasses on objects
    // inside the model.
    nasalSys->getGlobals().createHash(_module).set("_module_id", _module_id);

    naRef arg[2];
    arg[0] = nasalSys->propNodeGhost(_root);
    arg[1] = nasalSys->propNodeGhost(_prop);
    nasalSys->createModule(_module.c_str(), _path.c_str(), s, strlen(s),
                           _root, 2, arg);
}

//------------------------------------------------------------------------------
void FGNasalModelData::unload()
{
    if (_module.empty())
        return;

    FGNasalSys* nasalSys = (FGNasalSys*) globals->get_subsystem("nasal");
    if(!nasalSys) {
        SG_LOG(SG_NASAL, SG_WARN, "Trying to run an <unload> script "
               "without Nasal subsystem present.");
        return;
    }

    SG_LOG(SG_NASAL, SG_DEBUG, "Unloading nasal module " << _module.c_str());

    if (_unload)
    {
        const char *s = _unload->getStringValue();
        nasalSys->createModule(_module.c_str(), _module.c_str(), s, strlen(s), _root);
    }

    nasalSys->deleteModule(_module.c_str());
}

//------------------------------------------------------------------------------
osg::Node* FGNasalModelData::getNode()
{
  return _branch;
}

//------------------------------------------------------------------------------
FGNasalModelData* FGNasalModelData::getByModuleId(unsigned int id)
{
  FGNasalModelDataList::iterator it = std::find_if
  (
    _loaded_models.begin(),
    _loaded_models.end(),
    boost::bind(&FGNasalModelData::_module_id, _1) == id
  );

  if( it != _loaded_models.end() )
    return *it;

  return 0;
}

//------------------------------------------------------------------------------
FGNasalModelDataProxy::~FGNasalModelDataProxy()
{
    FGNasalSys* nasalSys = (FGNasalSys*) globals->get_subsystem("nasal");
    // when necessary, register Nasal module to be destroyed/unloaded
    // in the main thread.
    if ((_data.valid())&&(nasalSys))
        nasalSys->registerToUnload(_data);
}

//------------------------------------------------------------------------------
void FGNasalModelDataProxy::modelLoaded( const std::string& path,
                                         SGPropertyNode *prop,
                                         osg::Node *branch )
{
    FGNasalSys* nasalSys = (FGNasalSys*) globals->get_subsystem("nasal");
    if(!nasalSys) {
        SG_LOG(SG_NASAL, SG_WARN, "Trying to run a <load> script "
               "without Nasal subsystem present.");
        return;
    }
    
    if(!prop)
        return;
    
    SGPropertyNode *nasal = prop->getNode("nasal");
    if(!nasal)
        return;
    
    SGPropertyNode* load   = nasal->getNode("load");
    SGPropertyNode* unload = nasal->getNode("unload");
    
    if ((!load) && (!unload))
        return;
    
    _data = new FGNasalModelData(_root, path, prop, load, unload, branch);
    
    // register Nasal module to be created and loaded in the main thread.
    nasalSys->registerToLoad(_data);
}

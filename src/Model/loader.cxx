// loader.cxx - implement SSG model and texture loaders.

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <simgear/compiler.h>
#include <simgear/props/props.hxx>

#include "loader.hxx"
#include "model.hxx"



////////////////////////////////////////////////////////////////////////
// Implementation of FGSSGLoader.
////////////////////////////////////////////////////////////////////////

FGSSGLoader::FGSSGLoader ()
{
    // no op
}

FGSSGLoader::~FGSSGLoader ()
{
    std::map<string, ssgBase *>::iterator it = _table.begin();
    while (it != _table.end()) {
        it->second->deRef();
        _table.erase(it);
    }
}

void
FGSSGLoader::flush ()
{
    std::map<string, ssgBase *>::iterator it = _table.begin();
    while (it != _table.end()) {
        ssgBase * item = it->second;
                                // If there is only one reference, it's
                                // ours; no one else is using the item.
        if (item->getRef() == 1) {
            item->deRef();
            _table.erase(it);
        }
        it++;
    }
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGModelLoader.
////////////////////////////////////////////////////////////////////////

FGModelLoader::FGModelLoader ()
{
}

FGModelLoader::~FGModelLoader ()
{
}

ssgEntity *
FGModelLoader::load_model( const string &fg_root,
                           const string &path,
                           SGPropertyNode *prop_root,
                           double sim_time_sec )
{
                                // FIXME: normalize path to
                                // avoid duplicates.
    std::map<string, ssgBase *>::iterator it = _table.find(path);
    if (it == _table.end()) {
        _table[path] = fgLoad3DModel( fg_root, path, prop_root, sim_time_sec );
        it = _table.find(path);
        it->second->ref();      // add one reference to keep it around
    }
    return (ssgEntity *)it->second;
}



////////////////////////////////////////////////////////////////////////
// Implementation of FGTextureLoader.
////////////////////////////////////////////////////////////////////////

FGTextureLoader::FGTextureLoader ()
{
}

FGTextureLoader::~FGTextureLoader ()
{
}

ssgTexture *
FGTextureLoader::load_texture( const string &fg_root, const string &path )
{
    std::map<string, ssgBase *>::iterator it = _table.find(path);
    if (it == _table.end()) {
        _table[path] = new ssgTexture((char *)path.c_str()); // FIXME wrapu/v
        it = _table.find(path);
        it->second->ref();      // add one reference to keep it around
    }
    return (ssgTexture *)it->second;
}


// end of loader.cxx

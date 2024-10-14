// Copyright (C) 2013  James Turner
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef NASAL_MODEL_DATA_HXX
#define NASAL_MODEL_DATA_HXX

#include <simgear/nasal/nasal.h>
#include <simgear/scene/model/modellib.hxx>

class FGNasalModelData;
typedef SGSharedPtr<FGNasalModelData> FGNasalModelDataRef;
typedef std::list<FGNasalModelData*> FGNasalModelDataList;

/** Nasal model data container.
 * load and unload methods must be run in main thread (not thread-safe). */
class FGNasalModelData : public SGReferenced
{
public:
    /** Constructor to be run in an arbitrary thread. */
    FGNasalModelData( SGPropertyNode *root,
                      const std::string& path,
                      SGPropertyNode *prop,
                      SGPropertyNode* load,
                      SGPropertyNode* unload,
                      osg::Node* branch );
    
    ~FGNasalModelData();

    /** Load hook. Always call from inside the main loop. */
    void load();
    
    /** Unload hook. Always call from inside the main loop. */
    void unload();
    
    /**
     * Get osg scenegraph node of model
     */
    osg::Node* getNode();

    /**
     * Get FGNasalModelData for model with the given module id. Every scenery
     * model containing a nasal load or unload tag gets assigned a module id
     * automatically.
     *
     * @param id    Module id
     * @return model data or NULL if does not exists
     */
    static FGNasalModelData* getByModuleId(unsigned int id);

private:
    inline static std::mutex _loaded_models_mutex; // mutex protecting the _loaded_models
    inline static unsigned int _max_module_id = 0;
    inline static FGNasalModelDataList _loaded_models;

    std::string _module, _path;
    SGPropertyNode_ptr _root, _prop;
    SGConstPropertyNode_ptr _load, _unload;
    osg::ref_ptr<osg::Node> _branch;
    unsigned int _module_id;
};

/** Thread-safe proxy for FGNasalModelData.
 * modelLoaded/destroy methods only register the requested
 * operation. Actual (un)loading of Nasal module is deferred
 * and done in the main loop. */
class FGNasalModelDataProxy : public simgear::SGModelData
{
public:
    FGNasalModelDataProxy(SGPropertyNode *root = 0) :
    _root(root), _data(0)
    {
    }
    
    ~FGNasalModelDataProxy();
    
    void modelLoaded( const std::string& path,
                      SGPropertyNode *prop,
                      osg::Node *branch );
    
    virtual FGNasalModelDataProxy* clone() const { return new FGNasalModelDataProxy(_root); }

    ErrorContext getErrorContext() const override
    {
        return {}; // return nothing for now, not yet clear if this proxy needs it
    }

protected:
    SGPropertyNode_ptr _root;
    FGNasalModelDataRef _data;
};

#endif // of NASAL_MODEL_DATA_HXX

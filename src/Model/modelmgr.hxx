// model-mgr.hxx - manage user-specified 3D models.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __MODELMGR_HXX
#define __MODELMGR_HXX 1

#include <vector>
#include <memory>

#include <simgear/compiler.h>    // for SG_USING_STD
#include <simgear/structure/subsystem_mgr.hxx>

#include <Scripting/NasalModelData.hxx>

// Don't pull in headers, since we don't need them here.
class SGPropertyNode;
class SGModelPlacement;


/**
 * Manage a list of user-specified models.
 */
class FGModelMgr : public SGSubsystem
{
public:

    /**
     * A dynamically-placed model using properties.
     *
     * The model manager uses the property nodes to update the model's
     * position and orientation; any of the property node pointers may
     * be set to zero to avoid update.  Normally, a caller should
     * load the model by instantiating SGModelPlacement with the path
     * to the model or its XML wrapper, then assign any relevant
     * property node pointers.
     *
     * @see SGModelPlacement
     * @see FGModelMgr#add_instance
     */
    struct Instance
    {
        virtual ~Instance ();
        SGModelPlacement * model = nullptr;
        SGPropertyNode_ptr node;
        SGPropertyNode_ptr lon_deg_node;
        SGPropertyNode_ptr lat_deg_node;
        SGPropertyNode_ptr elev_ft_node;
        SGPropertyNode_ptr roll_deg_node;
        SGPropertyNode_ptr pitch_deg_node;
        SGPropertyNode_ptr heading_deg_node;
        SGPropertyNode_ptr loaded_node;
        bool shadow = false;

        bool checkLoaded() const;
    };

    FGModelMgr ();
    virtual ~FGModelMgr ();

    // Subsystem API.
    void bind() override;
    void init() override;
    void shutdown() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "model-manager"; }

    virtual void add_model (SGPropertyNode * node);

    /**
     * Add an instance of a dynamic model to the manager.
     *
     * NOTE: pointer ownership is transferred to the model manager!
     *
     * The caller is responsible for setting up the Instance structure
     * as required.  The model manager will continuously update the
     * location and orientation of the model based on the current
     * values of the properties.
     */
    virtual void add_instance (Instance * instance);


    /**
     * Remove an instance of a dynamic model from the manager.
     *
     * NOTE: the manager will delete the instance as well.
     */
    virtual void remove_instance (Instance * instance);


     /**
     * Finds an instance in the model manager from a given node path in the property tree.
     * A possible path could be "models/model[0]"
     *
     * NOTE: the manager will delete the instance as well.
     */
    Instance* findInstanceByNodePath(const std::string& nodePath) const;

private:
    /**
     * Listener class that adds models at runtime.
     */
    class Listener : public SGPropertyChangeListener
    {
    public:
        Listener(FGModelMgr *mgr) : _mgr(mgr) {}
        virtual void childAdded (SGPropertyNode * parent, SGPropertyNode * child);
        virtual void childRemoved (SGPropertyNode * parent, SGPropertyNode * child);

    private:
        FGModelMgr * _mgr;
    };

    SGPropertyNode_ptr _models;
    std::unique_ptr<Listener> _listener;

    std::vector<Instance *> _instances;
};

#endif // __MODELMGR_HXX

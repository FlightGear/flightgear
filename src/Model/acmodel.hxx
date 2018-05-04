// acmodel.hxx - manage a 3D aircraft model.
// Written by David Megginson, started 2002.
//
// This file is in the Public Domain, and comes with no warranty.

#ifndef __ACMODEL_HXX
#define __ACMODEL_HXX 1

#include <osg/ref_ptr>
#include <osg/Group>
#include <osg/Switch>

#include <memory>
#include <simgear/structure/subsystem_mgr.hxx>    // for SGSubsystem


// Don't pull in the headers, since we don't need them here.
class SGModelPlacement;
class FGFX;

class FGAircraftModel : public SGSubsystem
{
public:
    FGAircraftModel ();
    virtual ~FGAircraftModel ();

    // Subsystem API.
    void bind() override;
    void init() override;
    void reinit() override;
    void shutdown() override;
    void unbind() override;
    void update(double dt) override;

    // Subsystem identification.
    static const char* staticSubsystemClassId() { return "aircraft-model"; }

    virtual SGModelPlacement * get3DModel() { return _aircraft.get(); }
    virtual SGVec3d& getVelocity() { return _velocity; }

private:
    void deinit ();

    std::unique_ptr<SGModelPlacement> _aircraft;
    std::unique_ptr<SGModelPlacement> _interior;

    SGVec3d _velocity;
    SGSharedPtr<FGFX>  _fx;

    SGPropertyNode_ptr _speed_n;
    SGPropertyNode_ptr _speed_e;
    SGPropertyNode_ptr _speed_d;
};

#endif // __ACMODEL_HXX

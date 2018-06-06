#ifndef _FG_PRIVATE_ACCESSOR_FDM_HXX
#define _FG_PRIVATE_ACCESSOR_FDM_HXX

#include <map>
#include <vector>

#include <simgear/math/SGMath.hxx>
#include <simgear/structure/SGSharedPtr.hxx>

// Forward declarations for src/FDM/AIWake.
class AIWakeData;
class AIWakeGroup;
class AircraftMesh;
class WakeMesh;
class AeroElement;
typedef SGSharedPtr<AeroElement> AeroElement_ptr;


namespace FGTestApi {
namespace PrivateAccessor {
namespace FDM {

class Accessor
{
public:
    // Access variables from src/FDM/AIWake/AIWakeGroup.hxx.
    WakeMesh* read_FDM_AIWake_AIWakeGroup_aiWakeData(AIWakeGroup* instance, int i);

    // Access variables from src/FDM/AIWake/AircraftMesh.hxx.
    const std::vector<SGVec3d> read_FDM_AIWake_AircraftMesh_collPt(AircraftMesh* instance) const;
    const std::vector<SGVec3d> read_FDM_AIWake_AircraftMesh_midPt(AircraftMesh* instance) const;

    // Access variables from src/FDM/AIWake/WakeMesh.hxx.
    const std::vector<AeroElement_ptr> read_FDM_AIWake_WakeMesh_elements(WakeMesh* instance) const;
    int read_FDM_AIWake_WakeMesh_nelm(WakeMesh* instance) const;
    double **read_FDM_AIWake_WakeMesh_Gamma(WakeMesh* instance) const;
};

} // End of namespace FDM.
} // End of namespace PrivateAccessor.
} // End of namespace FGTestApi.

#endif // _FG_PRIVATE_ACCESSOR_FDM_HXX

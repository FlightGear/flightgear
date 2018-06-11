#include "PrivateAccessorFDM.hxx"

#include <FDM/AIWake/AIWakeGroup.hxx>
#include <FDM/AIWake/AircraftMesh.hxx>
#include <FDM/AIWake/WakeMesh.hxx>
#include <FDM/YASim/Atmosphere.hpp>



// Access variables from src/FDM/AIWake/AIWakeGroup.hxx.
WakeMesh*
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_AIWake_AIWakeGroup_aiWakeData(AIWakeGroup* instance, int i)
{
    return instance->_aiWakeData[i].mesh;
}


// Access variables from src/FDM/AIWake/AircraftMesh.hxx.
const std::vector<SGVec3d>
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_AIWake_AircraftMesh_collPt(AircraftMesh* instance) const
{
    return instance->collPt;
}

const std::vector<SGVec3d>
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_AIWake_AircraftMesh_midPt(AircraftMesh* instance) const
{
    return instance->midPt;
}


// Access variables from src/FDM/AIWake/WakeMesh.hxx.
const std::vector<AeroElement_ptr>
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_AIWake_WakeMesh_elements(WakeMesh* instance) const
{
   return instance->elements;
}

int
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_AIWake_WakeMesh_nelm(WakeMesh* instance) const
{
   return instance->nelm;
}

double **
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_AIWake_WakeMesh_Gamma(WakeMesh* instance) const
{
   return instance->Gamma;
}


// Access variables from src/FDM/YASim/Atmosphere.hxx.
float
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_YASim_Atmosphere_numColumns(std::unique_ptr<yasim::Atmosphere> &instance) const
{
    return instance->numColumns;
}

float
FGTestApi::PrivateAccessor::FDM::Accessor::read_FDM_YASim_Atmosphere_data(std::unique_ptr<yasim::Atmosphere> &instance, int i, int j) const
{
    return instance->data[i][j];
}

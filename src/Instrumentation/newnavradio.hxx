

#ifndef _FG_INSTRUMENTATION_NAVRADIO_HXX
#define _FG_INSTRUMENTATION_NAVRADIO_HXX

#include <simgear/props/props.hxx>
#include <simgear/structure/subsystem_mgr.hxx>

namespace Instrumentation {
class NavRadio : public SGSubsystem
{
public:
  static SGSubsystem * createInstance( SGPropertyNode_ptr rootNode );
};

}

#endif // _FG_INSTRUMENTATION_NAVRADIO_HXX
